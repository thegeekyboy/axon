/*
 * scylla2r.cpp — comprehensive example and test for axon::database::scylla
 *               and axon::resultset
 *
 * Tests covered:
 *   1.  Lifecycle               (connect, ping, version, close)
 *   2.  DDL                     (CREATE TABLE, DROP TABLE)
 *   3.  DML — literal SQL       (INSERT without bind)
 *   4.  DML — operator<<        (INSERT with << binding)
 *   5.  DML — variadic          (INSERT with variadic execute<T...>)
 *   6.  SELECT — all types      (bigint, int, smallint, tinyint,
 *                                float, double, boolean, text, blob)
 *   7.  Cursor navigation       (next(), get<T> by position)
 *   8.  get<T> by column name   (get("col", val))
 *   9.  operator>>              (stream extraction)
 *   10. resultset metadata    (count(), name(n), type(n), rows())
 *   11. to_json()
 *   12. operator<< print
 *   13. NULL handling           (get() returns false, output unchanged)
 *   14. Paginated fetch         (batch_size=2)
 *   15. done() after exhaustion (safe no-op)
 *   16. Boolean column
 *   17. BLOB round-trip
 *   18. tableinfo               (column_exists, colcnt)
 *
 * Build:
 *   g++ -std=c++17 -I ../include -L . -laxon -lscylla-cpp-driver \
 *       -o scylla2r ../examples/scylla2r.cpp
 *
 * Run:
 *   AXON_HOSTNAME=127.0.0.1 AXON_USERNAME=cassandra \
 *   AXON_PASSWORD=cassandra AXON_KEYSPACE=test ./scylla2r
 *
 * Prerequisite CQL:
 *   CREATE KEYSPACE IF NOT EXISTS test
 *     WITH replication = {'class':'SimpleStrategy','replication_factor':1};
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>

#include <axon.h>
#include <axon/util.h>
#include <axon/scylla.h>

static int g_pass = 0, g_fail = 0;

static void check(bool ok, const std::string &label)
{
    if (ok) { ++g_pass; std::cout << "  PASS  " << label << "\n"; }
    else    { ++g_fail; std::cout << "  FAIL  " << label << "\n"; }
}

static const std::string TBL = "scylla2r_test";

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[], char *env[])
{
    axon::timer ctm(__PRETTY_FUNCTION__);

    std::string hostname, username, password, keyspace;

    for (int i = 0; env[i]; i++)
    {
        auto p = axon::util::split(env[i], '=');
        if (p[0] == "AXON_HOSTNAME") hostname = p[1];
        if (p[0] == "AXON_USERNAME") username = p[1];
        if (p[0] == "AXON_PASSWORD") password = p[1];
        if (p[0] == "AXON_KEYSPACE") keyspace = p[1];
    }

    if (hostname.empty() || keyspace.empty())
    {
        std::cerr << "Set AXON_HOSTNAME, AXON_USERNAME, AXON_PASSWORD, AXON_KEYSPACE\n";
        return 1;
    }

    std::cout << "\n=== axon::database::scylla + resultset test suite ===\n";
    std::cout << "host: " << hostname << "  keyspace: " << keyspace << "\n\n";

    try {

        // ================================================================
        // 1. Lifecycle
        // ================================================================
        std::cout << "[1] Database lifecycle\n";

        axon::database::scylla db;
        db[AXON_DATABASE_HOSTNAME] = hostname;
        db[AXON_DATABASE_USERNAME] = username;
        db[AXON_DATABASE_PASSWORD] = password;
        db[AXON_DATABASE_KEYSPACE] = keyspace;
        db.connect();
        check(true, "connect()");
        check(db.ping(), "ping()");

        std::string ver = db.version();
        check(!ver.empty(), "version() non-empty: " + ver);

        // Use base class reference — tests polymorphic path
        axon::database::connector &cdb = db;

        // ================================================================
        // 2. DDL
        // ================================================================
        std::cout << "\n[2] DDL\n";

        cdb.execute("DROP TABLE IF EXISTS " + keyspace + "." + TBL);
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // schema propagation

        cdb.execute(
            "CREATE TABLE " + keyspace + "." + TBL + " ("
            "  id        bigint  PRIMARY KEY,"
            "  name      text,"
            "  score     double,"
            "  flags     int,"
            "  active    boolean,"
            "  rating    float,"
            "  level     smallint,"
            "  priority  tinyint,"
            "  payload   blob,"
            "  notes     text"
            ")"
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        check(true, "CREATE TABLE");

        // ================================================================
        // 3. DML — literal SQL (no bind)
        // ================================================================
        std::cout << "\n[3] DML — literal SQL\n";

        cdb.execute("INSERT INTO " + keyspace + "." + TBL +
                    " (id, name, score, flags, active, rating, level, priority)"
                    " VALUES (1, 'Alice', 95.5, 1, true, 4.8, 3, 1)");
        check(true, "INSERT Alice (literal)");

        cdb.execute("INSERT INTO " + keyspace + "." + TBL +
                    " (id, name, score, flags, active, rating, level, priority)"
                    " VALUES (2, 'Bob', 72.0, 0, false, 3.5, 2, 0)");
        check(true, "INSERT Bob (literal)");

        // Row with NULL score/notes for NULL handling test
        cdb.execute("INSERT INTO " + keyspace + "." + TBL +
                    " (id, name, flags, active)"
                    " VALUES (6, 'Frank', 0, false)");
        check(true, "INSERT Frank with NULL score/notes");

        // ================================================================
        // 4. DML — operator<< binding
        // ================================================================
        std::cout << "\n[4] DML via operator<<\n";

        // Named parameter style (:name) is converted to ? by replace()
        std::string n3 = "Carol";
        cdb << (int64_t) 3 << n3 << 88.8 << (int32_t) 1;
        cdb.execute("INSERT INTO " + keyspace + "." + TBL +
                    " (id, name, score, flags, active, rating, level, priority)"
                    " VALUES (?, ?, ?, ?, true, 4.2, 2, 1)");
        check(true, "INSERT Carol via operator<<");

        // ================================================================
        // 5. DML — variadic execute<T...>
        // ================================================================
        std::cout << "\n[5] DML via variadic execute<T...>\n";

        cdb.execute(
            "INSERT INTO " + keyspace + "." + TBL +
            " (id, name, score, flags, active, rating, level, priority)"
            " VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            (int64_t) 4, std::string("Dave"),
            61.1, (int32_t) 0,
            false,
            2.9f, (int16_t) 1, (int8_t) 0
        );
        check(true, "INSERT Dave via variadic execute");

        cdb.execute(
            "INSERT INTO " + keyspace + "." + TBL +
            " (id, name, score, flags, active, rating, level, priority)"
            " VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            (int64_t) 5, std::string("Eve"),
            55.0, (int32_t) 1,
            true,
            3.1f, (int16_t) 2, (int8_t) 1
        );
        check(true, "INSERT Eve via variadic execute");

        // ================================================================
        // 6 & 7. SELECT all types — cursor navigation and get<T>()
        // ================================================================
        std::cout << "\n[6+7] SELECT all types — cursor and get<T>()\n";

        cdb.query("SELECT id, name, score, flags, active, rating, level, priority "
                  "FROM " + keyspace + "." + TBL);
        {
            axon::resultset rc(cdb);
            int rows = 0;
            while (rc.next())
            {
                int64_t id;    rc.get(0, id);
                std::string n; rc.get(1, n);
                double score;  bool has_score = rc.get(2, score);

                check(id > 0,     "row " + std::to_string(rows) + ": id > 0");
                check(!n.empty(), "row " + std::to_string(rows) + ": name non-empty");
                rows++;
            }
            check(rows >= 5, "row count >= 5 (got " + std::to_string(rows) + ")");
            rc.done();
        }

        // ================================================================
        // 8. get<T> by column name
        // ================================================================
        std::cout << "\n[8] get<T> by column name\n";

        cdb.query("SELECT id, name, score FROM " + keyspace + "." + TBL +
                  " WHERE id = 1");
        {
            axon::resultset rc(cdb);
            check(rc.next(), "next() true for id=1");
            std::string aname; double ascore;
            rc.get("name",  aname);
            rc.get("score", ascore);
            check(aname  == "Alice", "get(name)  == Alice");
            check(ascore > 95.0,     "get(score) > 95.0");
            rc.done();
        }

        // ================================================================
        // 9. operator>>
        // ================================================================
        std::cout << "\n[9] operator>>\n";

        cdb.query("SELECT id, name, score FROM " + keyspace + "." + TBL +
                  " WHERE id = 2");
        {
            axon::resultset rc(cdb);
            check(rc.next(), "next() true for id=2");
            int64_t rid; std::string rname; double rscore;
            rc >> rid >> rname >> rscore;
            check(rid   == 2,     ">> id == 2");
            check(rname == "Bob", ">> name == Bob");
            check(rscore > 71.0,  ">> score > 71.0");
            rc.done();
        }

        // ================================================================
        // 10. resultset metadata
        // ================================================================
        std::cout << "\n[10] resultset metadata\n";

        cdb.query("SELECT id, name, score, flags, active, rating, level, priority "
                  "FROM " + keyspace + "." + TBL + " LIMIT 1");
        {
            axon::resultset rc(cdb);
            rc.next();
            check(rc.count() == 8,                            "count() == 8");
            check(rc.name(0) == "id",                         "name(0) == id");
            check(rc.name(1) == "name",                       "name(1) == name");
            check(rc.type(0) == axon::column_type::int64_t,   "type(0) == int64_t");
            check(rc.type(1) == axon::column_type::string_t,  "type(1) == string_t");
            check(rc.type(2) == axon::column_type::double_t,  "type(2) == double_t");
            check(rc.type(4) == axon::column_type::bool_t,    "type(4) == bool_t");
            check(rc.rows()  == 1,                            "rows() == 1");
            rc.done();
        }

        // ================================================================
        // 11. to_json()
        // ================================================================
        std::cout << "\n[11] to_json()\n";

        cdb.query("SELECT id, name, score FROM " + keyspace + "." + TBL +
                  " WHERE id = 2");
        {
            axon::resultset rc(cdb);
            rc.next();
            std::string json = rc.to_json();
            check(json.find("\"name\"") != std::string::npos, "to_json() has name key");
            check(json.find("\"Bob\"")  != std::string::npos, "to_json() has Bob value");
            std::cout << "         JSON: " << json << "\n";
            rc.done();
        }

        // ================================================================
        // 12. operator<< print
        // ================================================================
        std::cout << "\n[12] operator<<\n";

        cdb.query("SELECT id, name, score FROM " + keyspace + "." + TBL +
                  " WHERE id = 1");
        {
            axon::resultset rc(cdb);
            rc.next();
            std::cout << "         row: " << rc << "\n";
            check(true, "operator<< did not throw");
            rc.done();
        }

        // ================================================================
        // 13. NULL handling
        // ================================================================
        std::cout << "\n[13] NULL handling\n";

        cdb.query("SELECT score, notes FROM " + keyspace + "." + TBL +
                  " WHERE id = 6");
        {
            axon::resultset rc(cdb);
            check(rc.next(), "next() true for Frank (id=6)");

            double      score_out = 999.0;
            std::string notes_out = "sentinel";

            bool has_score = rc.get(0, score_out);
            bool has_notes = rc.get(1, notes_out);

            check(!has_score,                  "score NULL: get() returns false");
            check(score_out == 999.0,          "score NULL: output unchanged");
            check(!has_notes,                  "notes NULL: get() returns false");
            check(notes_out == "sentinel",     "notes NULL: output unchanged");
            rc.done();
        }

        // ================================================================
        // 14. Paginated fetch — batch_size=2
        // ================================================================
        std::cout << "\n[14] Paginated fetch (batch_size=2)\n";

        cdb.query("SELECT id, name FROM " + keyspace + "." + TBL);
        {
            axon::resultset rc(cdb, 2);
            int rows = 0;
            while (rc.next()) rows++;
            check(rows >= 5, "paginated: >= 5 rows (got " + std::to_string(rows) + ")");
            rc.done();
        }

        // ================================================================
        // 15. done() after natural exhaustion
        // ================================================================
        std::cout << "\n[15] done() after natural exhaustion\n";

        cdb.query("SELECT id FROM " + keyspace + "." + TBL + " LIMIT 2");
        {
            axon::resultset rc(cdb);
            while (rc.next()) { }
            rc.done();
            check(true, "done() after exhaustion — no throw");
        }

        // ================================================================
        // 16. Boolean column
        // ================================================================
        std::cout << "\n[16] Boolean column\n";

        cdb.query("SELECT id, active FROM " + keyspace + "." + TBL +
                  " WHERE id = 1");
        {
            axon::resultset rc(cdb);
            rc.next();
            bool active;
            rc.get("active", active);
            check(active == true, "active == true for Alice (id=1)");
            rc.done();
        }

        cdb.query("SELECT id, active FROM " + keyspace + "." + TBL +
                  " WHERE id = 2");
        {
            axon::resultset rc(cdb);
            rc.next();
            bool active;
            rc.get("active", active);
            check(active == false, "active == false for Bob (id=2)");
            rc.done();
        }

        // ================================================================
        // 17. BLOB round-trip
        // ================================================================
        std::cout << "\n[17] BLOB round-trip\n";

        // CQL hex literal for blob
        cdb.execute("UPDATE " + keyspace + "." + TBL +
                    " SET payload = 0xDEADBEEFCAFE WHERE id = 1");
        check(true, "UPDATE with BLOB literal");

        cdb.query("SELECT payload FROM " + keyspace + "." + TBL +
                  " WHERE id = 1");
        {
            axon::resultset rc(cdb);
            rc.next();
            std::vector<uint8_t> blob;
            bool has_blob = rc.get(0, blob);
            check(has_blob,           "BLOB: get() returns true");
            check(blob.size() == 6,   "BLOB: size == 6 bytes");
            check(blob[0] == 0xDE && blob[1] == 0xAD,
                  "BLOB: first two bytes == 0xDE 0xAD");
            rc.done();
        }

        // ================================================================
        // 18. tableinfo
        // ================================================================
        std::cout << "\n[18] tableinfo\n";

        auto info = db.describe(keyspace,TBL);
        check(info->column_exists("id"),      "column_exists(id)");
        check(info->column_exists("name"),    "column_exists(name)");
        check(info->column_exists("score"),   "column_exists(score)");
        check(!info->column_exists("ghost"),  "column_exists(ghost) == false");
        check(info->size() >= 8,            "size() >= 8");

        // ================================================================
        // Cleanup
        // ================================================================
        std::cout << "\n[cleanup]\n";

        cdb.execute("DROP TABLE IF EXISTS " + keyspace + "." + TBL);
        check(true, "DROP TABLE");

        db.close();
        check(true, "close()");

    } catch (axon::exception &e) {
        std::cerr << "\nEXCEPTION: " << e.what() << "\n";
        ++g_fail;
    }

    std::cout << "\n=== Results: " << g_pass << " passed, "
              << g_fail << " failed ===\n";
    std::cerr << "runtime: " << ctm.now() / 1000.0 << " ms\n";

    return g_fail == 0 ? 0 : 1;
}