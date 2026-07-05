/*
 * sqlite2r.cpp — comprehensive example and test for axon::database2r::sqlite
 *                and axon::recordset2r
 *
 * Tests covered:
 *   1.  Database lifecycle  (connect, ping, version, close)
 *   2.  DDL                 (CREATE TABLE, DROP TABLE)
 *   3.  DML — execute()     (INSERT via positional << binding)
 *   4.  DML — template      (INSERT via variadic query<T...> binding)
 *   5.  Transactions        (BEGIN / END)
 *   6.  SELECT — all types  (INTEGER, REAL, TEXT, BLOB, NULL)
 *   7.  recordset2r cursor  (next(), get<T> by position, get<T> by name)
 *   8.  recordset2r >> op   (stream extraction operator)
 *   9.  recordset2r metadata (count(), name(n), type(n), rows(), source())
 *   10. recordset2r to_json  (serialise current row)
 *   11. recordset2r print    (operator<< on ostream)
 *   12. NULL handling        (get() returns false, field not modified)
 *   13. Bind via operator<<  (db << value style)
 *   14. Paginated fetch      (recordset2r constructed with small batch_size)
 *   15. done() idempotency   (safe to call after natural exhaustion)
 *   16. UPDATE / DELETE      (DML with bind variables)
 *
 * Build (from the axon build directory):
 *   g++ -std=c++17 -I ../include -L . -laxon -lsqlite3 -o sqlite2r ../examples/sqlite2r.cpp
 *
 * Run:
 *   ./sqlite2r /tmp/test_axon.db
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>

#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>

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

int main(int argc, char *argv[])
{
	axon::timer ctm(__PRETTY_FUNCTION__);

	std::string dbpath = (argc > 1) ? argv[1] : "/tmp/test_axon2r.db";

	std::cout << "\n=== axon::database2r::sqlite + recordset2r test suite ===\n";
	std::cout << "database file: " << dbpath << "\n\n";

	try {

		// ================================================================
		// 1. Database lifecycle
		// ================================================================
		std::cout << "[1] Database lifecycle\n";

		axon::database2r::sqlite db;
		db[AXON_DATABASE2R_FILEPATH] = dbpath;
		db.connect();
		check(true, "connect()");

		check(db.ping(), "ping()");

		std::string ver = db.version();
		check(!ver.empty(), "version() non-empty: " + ver);

		// ================================================================
		// 2. DDL — CREATE TABLE
		// ================================================================
		std::cout << "\n[2] DDL\n";

		db.execute("DROP TABLE IF EXISTS TBL_TEST");
		db.execute(
			"CREATE TABLE TBL_TEST ("
			"  ID       INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  NAME     TEXT    NOT NULL,"
			"  SCORE    REAL,"
			"  FLAGS    INTEGER,"
			"  PAYLOAD  BLOB,"
			"  NOTES    TEXT"
			")"
		);
		check(true, "CREATE TABLE");

		// ================================================================
		// 3. DML — execute() with operator<< binding
		// ================================================================
		std::cout << "\n[3] DML via operator<<\n";

		std::string name1 = "Alice";
		db << name1;
		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES (:name, 95.5, 1)");
		check(true, "INSERT Alice via operator<<");

		std::string name2 = "Bob";
		db << name2;
		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES (:name, 72.0, 0)");
		check(true, "INSERT Bob via operator<<");

		// ================================================================
		// 4. DML — variadic template execute<T...>()
		// ================================================================
		std::cout << "\n[4] DML via variadic execute<T...>\n";

		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES (:name, :score, :flags)",
			std::string("Carol"), 88.8, 1);
		check(true, "INSERT Carol via variadic execute");

		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES (:name, :score, :flags)",
			std::string("Dave"), 61.1, 0);
		check(true, "INSERT Dave via variadic execute");

		// ================================================================
		// 5. Transactions
		// ================================================================
		std::cout << "\n[5] Transactions\n";

		db.transaction(axon::database2r::transaction::BEGIN);
		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES ('Eve', 55.0, 1)");
		db.execute("INSERT INTO TBL_TEST (NAME, SCORE, FLAGS) VALUES ('Frank', 44.0, 0)");
		db.transaction(axon::database2r::transaction::END);
		check(true, "transaction BEGIN/END with two inserts");

		// ================================================================
		// 6 & 7. SELECT all columns — cursor navigation and typed get()
		// ================================================================
		std::cout << "\n[6+7] SELECT — cursor navigation and get<T>()\n";

		db.query("SELECT ID, NAME, SCORE, FLAGS, PAYLOAD, NOTES FROM TBL_TEST ORDER BY ID");
		axon::recordset2r rc(db);

		int row = 0;
		while (rc.next())
		{
			int64_t id;    rc.get(0, id);
			std::string n; rc.get(1, n);
			double score;  bool has_score = rc.get(2, score);
			int64_t flags; rc.get(3, flags);

			check(id > 0,       "row " + std::to_string(row) + ": ID > 0");
			check(!n.empty(),   "row " + std::to_string(row) + ": NAME non-empty");
			check(has_score,    "row " + std::to_string(row) + ": SCORE not null");
			row++;
		}
		check(row == 6, "total rows == 6 (got " + std::to_string(row) + ")");
		rc.done();

		// ================================================================
		// 8. recordset2r >> stream extraction operator
		// ================================================================
		std::cout << "\n[8] operator>>\n";

		db.query("SELECT ID, NAME, SCORE FROM TBL_TEST WHERE NAME = :name", std::string("Alice"));
		axon::recordset2r rc2(db);

		check(rc2.next(), "next() returns true for Alice");
		int64_t aid; std::string aname; double ascore;
		rc2 >> aid >> aname >> ascore;
		check(aname == "Alice", ">> extracted name == Alice");
		check(ascore > 95.0,    ">> extracted score > 95.0");
		rc2.done();

		// ================================================================
		// 9. recordset2r metadata
		// ================================================================
		std::cout << "\n[9] recordset2r metadata\n";

		db.query("SELECT ID, NAME, SCORE, FLAGS, PAYLOAD, NOTES FROM TBL_TEST LIMIT 1");
		axon::recordset2r rc3(db);
		rc3.next();

		check(rc3.count() == 6,                            "count() == 6");
		check(rc3.name(0) == "ID",                         "name(0) == ID");
		check(rc3.name(1) == "NAME",                       "name(1) == NAME");
		check(rc3.type(0) == axon::column_type::int64_t,   "type(0) == int64_t");
		check(rc3.type(1) == axon::column_type::string_t,  "type(1) == string_t");
		check(rc3.type(2) == axon::column_type::double_t,  "type(2) == double_t");
		check(rc3.rows()  == 1,                            "rows() == 1 after one next()");
		rc3.done();

		// ================================================================
		// 10. recordset2r get() by column name
		// ================================================================
		std::cout << "\n[10] get<T> by column name\n";

		db.query("SELECT ID, NAME, SCORE FROM TBL_TEST WHERE NAME = :name", std::string("Carol"));
		axon::recordset2r rc4(db);
		rc4.next();

		std::string cname; double cscore;
		rc4.get("NAME",  cname);
		rc4.get("SCORE", cscore);
		check(cname  == "Carol", "get(NAME)  == Carol");
		check(cscore > 88.0,     "get(SCORE) > 88.0");
		rc4.done();

		// ================================================================
		// 11. recordset2r to_json() and operator<<
		// ================================================================
		std::cout << "\n[11] to_json() and operator<<\n";

		db.query("SELECT ID, NAME, SCORE FROM TBL_TEST WHERE NAME = :name", std::string("Bob"));
		axon::recordset2r rc5(db);
		rc5.next();

		std::string json = rc5.to_json();
		check(json.find("\"NAME\"") != std::string::npos, "to_json() contains NAME key");
		check(json.find("\"Bob\"")  != std::string::npos, "to_json() contains Bob value");

		std::cout << "         row as JSON: " << json << "\n";
		std::cout << "         row via <<:  " << rc5  << "\n";
		check(true, "operator<< did not throw");
		rc5.done();

		// ================================================================
		// 12. NULL handling
		// ================================================================
		std::cout << "\n[12] NULL handling\n";

		// PAYLOAD and NOTES were never set — they should be NULL.
		db.query("SELECT PAYLOAD, NOTES FROM TBL_TEST WHERE NAME = :name", std::string("Alice"));
		axon::recordset2r rc6(db);
		rc6.next();

		std::vector<uint8_t> payload_out = {0xFF};   // sentinel — should stay unchanged
		std::string notes_out = "sentinel";

		bool payload_has_val = rc6.get(0, payload_out);
		bool notes_has_val   = rc6.get(1, notes_out);

		check(!payload_has_val,          "BLOB NULL: get() returns false");
		check(payload_out[0] == 0xFF,    "BLOB NULL: output buffer unchanged");
		check(!notes_has_val,            "TEXT NULL: get() returns false");
		check(notes_out == "sentinel",   "TEXT NULL: output buffer unchanged");
		rc6.done();

		// ================================================================
		// 13. BLOB column — insert and read back
		// ================================================================
		std::cout << "\n[13] BLOB round-trip\n";

		const uint8_t blob_data[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
		std::vector<uint8_t> blob_vec(blob_data, blob_data + sizeof(blob_data));
		axon::database2r::bind blob_bind = blob_vec;

		db << blob_bind;
		db.execute("UPDATE TBL_TEST SET PAYLOAD = :blob WHERE NAME = 'Alice'");
		check(true, "UPDATE with BLOB bind");

		db.query("SELECT PAYLOAD FROM TBL_TEST WHERE NAME = :name", std::string("Alice"));
		axon::recordset2r rc7(db);
		rc7.next();

		std::vector<uint8_t> blob_out;
		bool has_blob = rc7.get(0, blob_out);
		check(has_blob,                          "BLOB: get() returns true");
		check(blob_out.size() == sizeof(blob_data), "BLOB: size matches");
		check(std::memcmp(blob_out.data(), blob_data, sizeof(blob_data)) == 0, "BLOB: content matches");
		rc7.done();

		// ================================================================
		// 14. Paginated fetch — small batch_size forces multiple fetch()
		//     calls from recordset2r::next() internall
		// ================================================================
		std::cout << "\n[14] Paginated fetch (batch_size=2)\n";

		db.query("SELECT ID, NAME FROM TBL_TEST ORDER BY ID");
		axon::recordset2r rc8(db, 2);   // batch 2 rows at a time

		int page_rows = 0;
		while (rc8.next())
		{
			int64_t pid; std::string pname;
			rc8.get(0, pid);
			rc8.get(1, pname);
			page_rows++;
		}
		check(page_rows == 6, "paginated fetch: total rows == 6 (got " + std::to_string(page_rows) + ")");
		rc8.done();

		// ================================================================
		// 15. done() idempotency after natural exhaustion
		// ================================================================
		std::cout << "\n[15] done() after natural exhaustion\n";

		db.query("SELECT ID FROM TBL_TEST LIMIT 2");
		axon::recordset2r rc9(db);
		while (rc9.next()) { }      // drain all rows
		rc9.done();                 // should not throw
		check(true, "done() after natural exhaustion does not throw");

		// ================================================================
		// 16. UPDATE with bind variable, verify with SELECT
		// ================================================================
		std::cout << "\n[16] UPDATE and DELETE with bind variables\n";

		db.execute("UPDATE TBL_TEST SET SCORE = :score, NOTES = :notes WHERE NAME = :name",
			99.9, std::string("Updated"), std::string("Alice"));
		check(true, "UPDATE via variadic execute");

		db.query("SELECT SCORE, NOTES FROM TBL_TEST WHERE NAME = :name", std::string("Alice"));
		axon::recordset2r rc10(db);
		rc10.next();
		double new_score; std::string new_notes;
		rc10.get("SCORE", new_score);
		rc10.get("NOTES", new_notes);
		check(new_score  > 99.0,        "UPDATE: SCORE updated to 99.9");
		check(new_notes == "Updated",   "UPDATE: NOTES updated");
		rc10.done();

		db.execute("DELETE FROM TBL_TEST WHERE NAME = :name", std::string("Frank"));
		check(true, "DELETE via variadic execute");

		db.query("SELECT COUNT(*) FROM TBL_TEST");
		axon::recordset2r rc11(db);
		rc11.next();
		int64_t remaining;
		rc11.get(0, remaining);
		check(remaining == 5, "DELETE: row count is 5 (got " + std::to_string(remaining) + ")");
		rc11.done();

		// ================================================================
		// Cleanup
		// ================================================================
		std::cout << "\n[cleanup]\n";

		db.execute("DROP TABLE TBL_TEST");
		check(true, "DROP TABLE");

		db.close();
		check(true, "close()");

	} catch (axon::exception &e) {
		std::cerr << "\nEXCEPTION: " << e.what() << "\n";
		++g_fail;
	}

	std::cout << "\n=== Results: " << g_pass << " passed, " << g_fail << " failed ===\n";
	std::cerr << "runtime: " << ctm.now() / 1000.0 << " ms\n";

	return g_fail == 0 ? 0 : 1;
}