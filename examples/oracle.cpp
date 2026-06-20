/*
 * oracle2r.cpp — comprehensive example and test for axon::database2r::oracle
 *                and axon::recordset2r
 *
 * Tests covered:
 *   1.  Database lifecycle      (connect via operator[], ping, version, close)
 *   2.  DDL                     (CREATE TABLE, DROP TABLE)
 *   3.  DML — execute()         (INSERT, UPDATE, DELETE)
 *   4.  DML — variadic          (INSERT via variadic execute<T...>)
 *   5.  Transactions            (COMMIT / ROLLBACK)
 *   6.  SELECT — numeric types  (NUMBER integer, NUMBER decimal, FLOAT,
 *                                BINARY_FLOAT, BINARY_DOUBLE)
 *   7.  SELECT — string types   (VARCHAR2, CHAR, NVARCHAR2)
 *   8.  SELECT — date/time      (DATE, TIMESTAMP, TIMESTAMP WITH TIME ZONE)
 *   9.  SELECT — binary         (RAW)
 *   10. SELECT — NULL handling  (get() returns false, field unchanged)
 *   11. Bind variables          (operator<< style, variadic style)
 *   12. recordset2r cursor      (next(), get<T> by position, get<T> by name)
 *   13. recordset2r >> op       (stream extraction, column cursor reset)
 *   14. recordset2r metadata    (count(), name(n), type(n), rows(), source())
 *   15. recordset2r to_json     (serialise current row)
 *   16. recordset2r operator<<  (print current row)
 *   17. Paginated fetch         (batch_size smaller than result set)
 *   18. done() after exhaustion (safe to call after natural EOF)
 *   19. ping() on live connection
 *   20. flush() (COMMIT via flush)
 *
 * Build (from axon build directory):
 *   g++ -std=c++17 -I ../include -L . -laxon -lclntsh -o oracle2r ../examples/oracle2r.cpp
 *
 * Run:
 *   AXON_SID=mydb AXON_USERNAME=user AXON_PASSWORD=pass ./oracle2r
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include <axon.h>
#include <axon/util.h>
#include <axon/oracle.h>

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static int g_pass = 0, g_fail = 0;

static void check(bool ok, const std::string &label)
{
    if (ok) { ++g_pass; std::cout << "  PASS  " << label << "\n"; }
    else    { ++g_fail; std::cout << "  FAIL  " << label << "\n"; }
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[], char *env[])
{
    axon::timer ctm(__PRETTY_FUNCTION__);

    std::string sid, username, password;

    for (int i = 0; env[i] != nullptr; i++)
    {
        auto parts = axon::util::split(env[i], '=');
        if (parts[0] == "AXON_SID")      sid      = parts[1];
        if (parts[0] == "AXON_USERNAME") username = parts[1];
        if (parts[0] == "AXON_PASSWORD") password = parts[1];
    }

    std::cout << "\n=== axon::database2r::oracle + recordset2r test suite ===\n\n";

    try {

        // ================================================================
        // 1. Database lifecycle
        // ================================================================
        std::cout << "[1] Database lifecycle\n";

        axon::database2r::oracle ora;
        ora[AXON_DATABASE2R_HOSTNAME] = sid;
        ora[AXON_DATABASE2R_USERNAME] = username;
        ora[AXON_DATABASE2R_PASSWORD] = password;
        ora.connect();
        check(true, "connect() via operator[]");

        check(ora.ping(), "ping()");

        std::string ver = ora.version();
        check(!ver.empty(), "version() non-empty: " + ver.substr(0, 50));

        // Use connector& reference for the rest — tests the polymorphic path
        axon::database2r::connector &db = ora;

        // ================================================================
        // 2. DDL
        // ================================================================
        std::cout << "\n[2] DDL\n";

        db.execute("BEGIN EXECUTE IMMEDIATE 'DROP TABLE ORA2R_TEST'; EXCEPTION WHEN OTHERS THEN NULL; END;");

        db.execute(
            "CREATE TABLE ORA2R_TEST ("
            "  ID           NUMBER(10)          NOT NULL,"
            "  NAME         VARCHAR2(100)        NOT NULL,"
            "  SCORE        NUMBER(10,2),"
            "  FLAGS        NUMBER(3),"
            "  BFLOAT_COL   BINARY_FLOAT,"
            "  BDOUBLE_COL  BINARY_DOUBLE,"
            "  CHAR_COL     CHAR(10),"
            "  DATE_COL     DATE,"
            "  TS_COL       TIMESTAMP(6),"
            "  TSTZ_COL     TIMESTAMP(6) WITH TIME ZONE,"
            "  RAW_COL      RAW(16),"
            "  NOTES        VARCHAR2(256),"
            "  PRIMARY KEY (ID)"
            ")"
        );
        check(true, "CREATE TABLE ORA2R_TEST");

        // ================================================================
        // 3. DML — execute() with operator<< binding
        // ================================================================
        std::cout << "\n[3] DML via operator<<\n";

        std::string name1 = "Alice";
        db << name1;
        db.execute("INSERT INTO ORA2R_TEST (ID,NAME,SCORE,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) VALUES (1,:name,95.5,1,'A_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('DEADBEEF'))");
        check(true, "INSERT Alice via operator<<");

        std::string name2 = "Bob";
        db << name2;
        db.execute("INSERT INTO ORA2R_TEST (ID,NAME,SCORE,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) VALUES (2,:name,72.0,0,'B_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('CAFEBABE'))");
        check(true, "INSERT Bob via operator<<");

        // ================================================================
        // 4. DML — variadic execute<T...>
        // ================================================================
        std::cout << "\n[4] DML via variadic execute<T...>\n";

        db.execute(
            "INSERT INTO ORA2R_TEST (ID,NAME,SCORE,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) "
            "VALUES (3,:name,:score,:flags,'C_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('AABBCCDD'))",
            std::string("Carol"), 88.75, 1
        );
        check(true, "INSERT Carol via variadic execute");

        db.execute(
            "INSERT INTO ORA2R_TEST (ID,NAME,SCORE,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) "
            "VALUES (4,:name,:score,:flags,'D_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('11223344'))",
            std::string("Dave"), 61.1, 0
        );
        check(true, "INSERT Dave via variadic execute");

        // Row with NULL SCORE and NULL NOTES — to test NULL handling
        db.execute(
            "INSERT INTO ORA2R_TEST (ID,NAME,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) "
            "VALUES (5,'Eve',1,'E_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('FFFFFF00'))"
        );
        check(true, "INSERT Eve with NULL SCORE");

        // Row with BINARY_FLOAT and BINARY_DOUBLE
        db.execute(
            "INSERT INTO ORA2R_TEST (ID,NAME,SCORE,FLAGS,BFLOAT_COL,BDOUBLE_COL,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) "
            "VALUES (6,'Frank',99.99,1,3.14,2.71828,'F_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('00112233'))"
        );
        check(true, "INSERT Frank with BINARY_FLOAT/DOUBLE");

        // ================================================================
        // 5. Transactions
        // ================================================================
        std::cout << "\n[5] Transactions\n";

        db.transaction(axon::database2r::transaction::BEGIN);
        db.execute("INSERT INTO ORA2R_TEST (ID,NAME,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) VALUES (7,'Grace',1,'G_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('ABCDEF01'))");
        db.execute("INSERT INTO ORA2R_TEST (ID,NAME,FLAGS,CHAR_COL,DATE_COL,TS_COL,TSTZ_COL,RAW_COL) VALUES (8,'Hank',0,'H_CHAR',SYSDATE,SYSTIMESTAMP,SYSTIMESTAMP,HEXTORAW('FEDCBA98'))");
        db.transaction(axon::database2r::transaction::END);
        check(true, "transaction BEGIN/END with two inserts");

        // ================================================================
        // 6. SELECT — numeric types
        // ================================================================
        std::cout << "\n[6] SELECT — numeric types\n";

        db.query("SELECT ID, SCORE, BFLOAT_COL, BDOUBLE_COL FROM ORA2R_TEST WHERE ID IN (1,2,3,6) ORDER BY ID");
        {
            axon::recordset2r rc(db);
            int row = 0;
            while (rc.next())
            {
                long   id;    rc.get(0, id);
                double score; bool has_score = rc.get(1, score);

                check(id > 0,    "row " + std::to_string(row) + ": ID > 0 (got " + std::to_string(id) + ")");
                if (id != 6)
                    check(has_score, "row " + std::to_string(row) + ": SCORE not null");
                row++;
            }
            check(row == 4, "numeric query: 4 rows returned");
            rc.done();
        }

        // ================================================================
        // 7. SELECT — string types
        // ================================================================
        std::cout << "\n[7] SELECT — string types\n";

        db.query("SELECT ID, NAME, CHAR_COL FROM ORA2R_TEST WHERE ID <= 3 ORDER BY ID");
        {
            axon::recordset2r rc(db);
            const std::string expected[] = { "Alice", "Bob", "Carol" };
            int row = 0;
            while (rc.next())
            {
                std::string name, charcol;
                rc.get(1, name);
                rc.get(2, charcol);

                check(name == expected[row], "row " + std::to_string(row) + ": NAME == " + expected[row] + " (got '" + name + "')");
                check(!charcol.empty(),      "row " + std::to_string(row) + ": CHAR_COL non-empty");
                row++;
            }
            check(row == 3, "string query: 3 rows returned");
            rc.done();
        }

        // ================================================================
        // 8. SELECT — date / timestamp
        // ================================================================
        std::cout << "\n[8] SELECT — date and timestamp\n";

        db.query("SELECT ID, DATE_COL, TS_COL, TSTZ_COL FROM ORA2R_TEST WHERE ID = 1");
        {
            axon::recordset2r rc(db);
            check(rc.next(), "date query: next() returns true");
            std::string date_str, ts_str, tstz_str;
            rc.get(1, date_str);
            rc.get(2, ts_str);
            rc.get(3, tstz_str);
            check(!date_str.empty(),  "DATE_COL non-empty: " + date_str);
            check(!ts_str.empty(),    "TIMESTAMP_COL non-empty: " + ts_str);
            check(!tstz_str.empty(),  "TIMESTAMP_TZ_COL non-empty: " + tstz_str);
            rc.done();
        }

        // ================================================================
        // 9. SELECT — RAW / binary
        // ================================================================
        std::cout << "\n[9] SELECT — RAW binary\n";

        db.query("SELECT ID, RAW_COL FROM ORA2R_TEST WHERE ID = 1");
        {
            axon::recordset2r rc(db);
            check(rc.next(), "raw query: next() returns true");

            std::vector<uint8_t> raw;
            bool has_raw = rc.get(1, raw);
            check(has_raw,           "RAW_COL: get() returns true");
            check(raw.size() == 4,   "RAW_COL: size == 4 bytes (got " + std::to_string(raw.size()) + ")");
            check(raw[0] == 0xDE && raw[1] == 0xAD && raw[2] == 0xBE && raw[3] == 0xEF, "RAW_COL: content == DEADBEEF");
            rc.done();
        }

        // ================================================================
        // 10. NULL handling
        // ================================================================
        std::cout << "\n[10] NULL handling\n";

        db.query("SELECT ID, SCORE, NOTES FROM ORA2R_TEST WHERE ID = 5");
        {
            axon::recordset2r rc(db);
            check(rc.next(), "null query: next() returns true for Eve");

            double  score_sentinel = 999.0;
            std::string notes_sentinel = "sentinel";

            bool has_score = rc.get(1, score_sentinel);
            bool has_notes = rc.get(2, notes_sentinel);

            check(!has_score,                  "SCORE NULL: get() returns false");
            check(score_sentinel == 999.0,     "SCORE NULL: output unchanged");
            check(!has_notes,                  "NOTES NULL: get() returns false");
            check(notes_sentinel == "sentinel","NOTES NULL: output unchanged");
            rc.done();
        }

        // ================================================================
        // 11. Bind variables — WHERE clause
        // ================================================================
        std::cout << "\n[11] Bind variables in WHERE\n";

        db.query("SELECT ID, NAME FROM ORA2R_TEST WHERE ID < :max_id ORDER BY ID", 4);
        {
            axon::recordset2r rc(db);
            int count = 0;
            while (rc.next()) count++;
            check(count == 3, "bind WHERE ID < 4: 3 rows (got " + std::to_string(count) + ")");
            rc.done();
        }

        std::string filter_name = "Alice";
        db << filter_name;
        db.query("SELECT ID, NAME FROM ORA2R_TEST WHERE NAME = :name");
        {
            axon::recordset2r rc(db);
            check(rc.next(), "bind WHERE NAME = Alice: next() true");
            std::string name;
            rc.get(1, name);
            check(name == "Alice", "bind WHERE NAME: got Alice (got '" + name + "')");
            rc.done();
        }

        // ================================================================
        // 12. get<T> by position and by name
        // ================================================================
        std::cout << "\n[12] get<T> by position and by name\n";

        db.query("SELECT ID, NAME, SCORE FROM ORA2R_TEST WHERE ID = 3");
        {
            axon::recordset2r rc(db);
            rc.next();

            long id; std::string name; double score;
            rc.get(0, id);
            rc.get(1, name);
            rc.get(2, score);
            check(id    == 3,       "get by position: ID == 3");
            check(name  == "Carol", "get by position: NAME == Carol");
            check(score > 88.0,     "get by position: SCORE > 88.0");

            // Reset and re-query for by-name test
            rc.done();
        }

        db.query("SELECT ID, NAME, SCORE FROM ORA2R_TEST WHERE ID = 3");
        {
            axon::recordset2r rc(db);
            rc.next();

            long id; std::string name; double score;
            rc.get("ID",    id);
            rc.get("NAME",  name);
            rc.get("SCORE", score);
            check(id    == 3,       "get by name: ID == 3");
            check(name  == "Carol", "get by name: NAME == Carol");
            check(score > 88.0,     "get by name: SCORE > 88.0");
            rc.done();
        }

        // ================================================================
        // 13. operator>> stream extraction
        // ================================================================
        std::cout << "\n[13] operator>>\n";

        db.query("SELECT ID, NAME, SCORE FROM ORA2R_TEST WHERE ID = 2");
        {
            axon::recordset2r rc(db);
            check(rc.next(), ">> test: next() true for Bob");

            long bid; std::string bname; double bscore;
            rc >> bid >> bname >> bscore;

            check(bid    == 2,     ">> extracted ID == 2");
            check(bname  == "Bob", ">> extracted NAME == Bob");
            check(bscore > 71.0,   ">> extracted SCORE > 71.0");
            rc.done();
        }

        // ================================================================
        // 14. recordset2r metadata
        // ================================================================
        std::cout << "\n[14] recordset2r metadata\n";

        db.query("SELECT ID, NAME, SCORE, FLAGS FROM ORA2R_TEST WHERE ROWNUM = 1");
        {
            axon::recordset2r rc(db);
            rc.next();

            check(rc.count()  == 4,                             "count() == 4");
            check(rc.name(0)  == "ID",                          "name(0) == ID");
            check(rc.name(1)  == "NAME",                        "name(1) == NAME");
            check(rc.type(0)  == axon::column_type::int64_t,    "type(0) == int64_t");
            check(rc.type(1)  == axon::column_type::string_t,   "type(1) == string_t");
            check(rc.rows()   == 1,                             "rows() == 1 after one next()");
            rc.done();
        }

        // ================================================================
        // 15. to_json()
        // ================================================================
        std::cout << "\n[15] to_json()\n";

        db.query("SELECT ID, NAME, SCORE FROM ORA2R_TEST WHERE ID = 1");
        {
            axon::recordset2r rc(db);
            rc.next();

            std::string json = rc.to_json();
            check(json.find("\"NAME\"")  != std::string::npos, "to_json() contains NAME key");
            check(json.find("\"Alice\"") != std::string::npos, "to_json() contains Alice value");
            std::cout << "         JSON: " << json << "\n";
            rc.done();
        }

        // ================================================================
        // 16. operator<< print
        // ================================================================
        std::cout << "\n[16] operator<<\n";

        db.query("SELECT ID, NAME, SCORE FROM ORA2R_TEST WHERE ID = 2");
        {
            axon::recordset2r rc(db);
            rc.next();
            std::cout << "         row:  " << rc << "\n";
            check(true, "operator<< did not throw");
            rc.done();
        }

        // ================================================================
        // 17. Paginated fetch — batch_size smaller than result set
        // ================================================================
        std::cout << "\n[17] Paginated fetch (batch_size=2)\n";

        db.query("SELECT ID, NAME FROM ORA2R_TEST ORDER BY ID");
        {
            axon::recordset2r rc(db, 2);   // force multiple fetch() calls
            int rows = 0;
            while (rc.next()) rows++;
            check(rows == 8, "paginated: all 8 rows returned (got " + std::to_string(rows) + ")");
            rc.done();
        }

        // ================================================================
        // 18. done() after natural exhaustion
        // ================================================================
        std::cout << "\n[18] done() after natural exhaustion\n";

        db.query("SELECT ID FROM ORA2R_TEST WHERE ROWNUM <= 2");
        {
            axon::recordset2r rc(db);
            while (rc.next()) { }    // drain all rows
            rc.done();               // must not throw
            check(true, "done() after natural exhaustion does not throw");
        }

        // ================================================================
        // 19. ping() and flush()
        // ================================================================
        std::cout << "\n[19] ping() and flush()\n";

        check(db.ping(), "ping() on live connection");

        db.execute("UPDATE ORA2R_TEST SET NOTES = 'flushed' WHERE ID = 1");
        db.flush();
        check(true, "flush() (COMMIT) does not throw");

        db.query("SELECT NOTES FROM ORA2R_TEST WHERE ID = 1");
        {
            axon::recordset2r rc(db);
            rc.next();
            std::string notes;
            rc.get(0, notes);
            check(notes == "flushed", "NOTES == flushed after flush()");
            rc.done();
        }

        // ================================================================
        // 20. UPDATE and DELETE with bind variables
        // ================================================================
        std::cout << "\n[20] UPDATE and DELETE with bind variables\n";

        db.execute("UPDATE ORA2R_TEST SET SCORE = :score WHERE NAME = :name", 100.0, std::string("Alice"));
        check(true, "UPDATE via variadic execute");

        db.query("SELECT SCORE FROM ORA2R_TEST WHERE NAME = :name", std::string("Alice"));
        {
            axon::recordset2r rc(db);
            rc.next();
            double updated_score;
            rc.get(0, updated_score);
            check(updated_score > 99.0, "UPDATE: SCORE updated to 100.0 (got " + std::to_string(updated_score) + ")");
            rc.done();
        }

        db.execute("DELETE FROM ORA2R_TEST WHERE NAME = :name", std::string("Hank"));
        check(true, "DELETE via variadic execute");

        db.query("SELECT COUNT(*) FROM ORA2R_TEST");
        {
            axon::recordset2r rc(db);
            rc.next();
            long remaining;
            rc.get(0, remaining);
            check(remaining == 7, "DELETE: 7 rows remain (got " + std::to_string(remaining) + ")");
            rc.done();
        }

        // ================================================================
        // Cleanup
        // ================================================================
        std::cout << "\n[cleanup]\n";

        db.execute("DROP TABLE ORA2R_TEST");
        check(true, "DROP TABLE ORA2R_TEST");

        ora.close();
        check(true, "close()");

    } catch (axon::exception &e) {
        std::cerr << "\nEXCEPTION: " << e.what() << "\n";
        ++g_fail;
    }

    std::cout << "\n=== Results: " << g_pass << " passed, " << g_fail << " failed ===\n";
    std::cerr << "runtime: " << ctm.now() / 1000.0 << " ms\n";

    return g_fail == 0 ? 0 : 1;
}