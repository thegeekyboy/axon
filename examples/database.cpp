#include <iostream>
#include <string>
#include <memory>

#include <axon.h>
#include <axon/util.h>
// #include <axon/sqlite.h>
#include <axon/oracle.h>
// #include <axon/scylladb.h>

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
	axon::timer ctm(__PRETTY_FUNCTION__);
	std::string hostname, username, password, keyspace, sid;

	for(int i=0;env[i]!=NULL;i++)
	{
    	auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_HOSTNAME")
			hostname = parts[1];
		else if (parts[0] == "AXON_KEYSPACE")
			keyspace = parts[1];
		else if (parts[0] == "AXON_SID")
			sid = parts[1];
	}

	try {

		axon::database2r::oracle ora;
		ora.connect(sid, username, password);
		axon::database2r::connector &db = ora;

		// axon::database2r::scylladb scylla;
		// scylla[AXON_DATABASE_KEYSPACE] = keyspace;
		// scylla.connect(hostname, username, password);
		// axon::database2r::interface &db = scylla;

		// axon::database2r::sqlite lite;
		// lite.connect("/tmp/test.db", username, password);
		// axon::database2r::interface &db = lite;

		db.ping();
		std::cout<<db.version()<<std::endl;

		// if (argc <= 2) return 0;
		// int ival = atoi(argv[1]);
		// std::string sval = argv[2];

		// long blah = 1122;
		// char bs[23] = "this is a bs";

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database2r::bind l = slist;
		axon::database2r::bind i = 1100, f = 15.1, s = (text*) "BD", addr = (char*)"%BD%";

		// ANSI SQL Tests ///////////////////////
		// db.execute("CREATE TABLE TBL_EMPLOYEE_INFO (EID INT, NAME VARCHAR2(256), DESIGNATION VARCHAR2(256), UNIT VARCHAR2(32), JOINING_DATE DATE, GENDER VARCHAR2(16), BL BLOB, RA RAW(512), PRIMARY KEY (EID))");
		// db.execute("INSERT INTO TBL_EMPLOYEE_INFO(EID,NAME,DESIGNATION,UNIT,JOINING_DATE,GENDER) VALUES(1, 'SOME EMPLOYEE', 'CTO', 'TECHNOLOGY', '12-JAN-2020', 'Male')");
		// db.execute("INSERT INTO TBL_EMPLOYEE_INFO(EID,NAME,DESIGNATION,UNIT,JOINING_DATE,GENDER) VALUES(2, 'OTHER EMPLOYEE', 'CHRO', 'HUMAN RESOURCE', '1-SEP-2010', 'Female')");

		// db<<ival<<sval;
		// db.execute("INSERT INTO TBL_EMPLOYEE_INFO (EID, NAME) VALUES (:yx, :pp)", ival, sval);

		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO");
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE UNIT = :uname", sval);
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID = :eid", ival);

		// db.query("SELECT ROWID, A.* FROM TBL_EMPLOYEE_INFO A");
		
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID < :eid");
		// db<<ival;

		// db.query("select * from TBL_EMPLOYEE_INFO where unit like :unit and eid < :eid");
		// db<<sval<<ival;
		// ANSI End ///////////////////////////////

		// ORACLE Test //
		db.execute("CREATE TABLE ALL_TYPES_DEMO(ID_COL NUMBER(10) PRIMARY KEY, NUM_SCALE_COL NUMBER(10, 2), FLOAT_COL FLOAT(63), BINARY_FLOAT_COL BINARY_FLOAT, BINARY_DOUBLE_COL BINARY_DOUBLE, VARCHAR2_COL VARCHAR2(100), NVARCHAR2_COL NVARCHAR2(100), CHAR_COL CHAR(10), NCHAR_COL NCHAR(10), DATE_COL DATE, TIMESTAMP_COL TIMESTAMP(6), TIMESTAMP_TZ_COL TIMESTAMP(6) WITH TIME ZONE, TIMESTAMP_LTZ_COL TIMESTAMP(6) WITH LOCAL TIME ZONE, INTERVAL_YM_COL INTERVAL YEAR(2) TO MONTH, INTERVAL_DS_COL INTERVAL DAY(3) TO SECOND(2), RAW_COL RAW(16), BLOB_COL BLOB, CLOB_COL CLOB, NCLOB_COL NCLOB)");
		db.execute("INSERT INTO all_types_demo VALUES (1, 100.50, 1.23, 1.2, 3.456, 'Alice', 'Alpha', 'A', 'X', TO_DATE('2026-01-01', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-01-01 10:00:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-01-01 10:00:00 -05:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-01-01 10:00:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '1-2' YEAR TO MONTH, INTERVAL '5 10:00:00' DAY TO SECOND, HEXTORAW('AA'), TO_BLOB(HEXTORAW('7F')), 'Text 1', 'NText 1')");
		db.execute("INSERT INTO all_types_demo VALUES (2, -250.75, -5.67, -2.4, -9.876, 'Bob', 'Beta', 'B', 'Y', TO_DATE('2026-02-15', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-02-15 11:30:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-02-15 11:30:00 +01:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-02-15 11:30:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '0-6' YEAR TO MONTH, INTERVAL '1 02:30:15' DAY TO SECOND, HEXTORAW('BB'), TO_BLOB(HEXTORAW('8F')), 'Text 2', 'NText 2')");
		db.execute("INSERT INTO all_types_demo VALUES (3, 9999.99, 12345.67, 123.45, 987.654, 'Charlie', 'Gamma', 'C', 'Z', TO_DATE('2026-03-20', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-03-20 14:15:22', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-03-20 14:15:22 +00:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-03-20 14:15:22', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '10-0' YEAR TO MONTH, INTERVAL '12 00:00:00' DAY TO SECOND, HEXTORAW('CC'), TO_BLOB(HEXTORAW('9F')), 'Text 3', 'NText 3')");
		db.execute("INSERT INTO all_types_demo VALUES (4, 0.00, 0.0, 0.0, 0.0, 'David', 'Delta', 'D', 'W', TO_DATE('2026-04-05', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-04-05 08:00:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-04-05 08:00:00 -08:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-04-05 08:00:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '2-11' YEAR TO MONTH, INTERVAL '0 05:22:10' DAY TO SECOND, HEXTORAW('00'), TO_BLOB(HEXTORAW('00')), 'Text 4', 'NText 4')");
		db.execute("INSERT INTO all_types_demo VALUES (5, 55.22, 9.81, 3.14, 2.718, 'Eve', 'Epsilon', 'E', 'V', SYSDATE, SYSTIMESTAMP, SYSTIMESTAMP, SYSTIMESTAMP, INTERVAL '5-4' YEAR TO MONTH, INTERVAL '10 12:00:00' DAY TO SECOND, HEXTORAW('FF'), TO_BLOB(HEXTORAW('FF')), 'Text 5', 'NText 5')");
		db.execute("INSERT INTO all_types_demo VALUES (6, 12.34, 1.11, 2.22, 3.333, 'Frank', 'Zeta', 'F', 'U', TO_DATE('2026-05-12', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-05-12 19:45:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-05-12 19:45:00 +05:30', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-05-12 19:45:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '0-1' YEAR TO MONTH, INTERVAL '0 00:00:45' DAY TO SECOND, HEXTORAW('01'), TO_BLOB(HEXTORAW('11')), 'Long character content 6', 'Wide character content 6')");
		db.execute("INSERT INTO all_types_demo VALUES (7, 88.00, 2.34, 4.56, 7.89, 'Grace', 'Eta', 'G', 'T', TO_DATE('2026-06-30', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-06-30 23:59:59.999', 'YYYY-MM-DD HH24:MI:SS.FF'), TO_TIMESTAMP_TZ('2026-06-30 23:59:59.999 +02:00', 'YYYY-MM-DD HH24:MI:SS.FF TZH:TZM'), TO_TIMESTAMP('2026-06-30 23:59:59.999', 'YYYY-MM-DD HH24:MI:SS.FF'), INTERVAL '15-6' YEAR TO MONTH, INTERVAL '2 18:30:00' DAY TO SECOND, HEXTORAW('02'), TO_BLOB(HEXTORAW('22')), 'Text 7', 'NText 7')");
		db.execute("INSERT INTO all_types_demo VALUES (8, 4500.67, 100.01, 50.5, 20.22, 'Hank', 'Theta', 'H', 'S', TO_DATE('2028-02-29', 'YYYY-MM-DD'), TO_TIMESTAMP('2028-02-29 12:00:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2028-02-29 12:00:00 -04:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2028-02-29 12:00:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '3-0' YEAR TO MONTH, INTERVAL '4 04:04:04' DAY TO SECOND, HEXTORAW('A1'), TO_BLOB(HEXTORAW('A2')), 'Text 8', 'NText 8')");
		db.execute("INSERT INTO all_types_demo VALUES (9, 0.01, 0.001, 0.1, 0.01, 'Ivy', 'Iota', 'I', 'R', TO_DATE('2026-07-04', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-07-04 06:15:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-07-04 06:15:00 -07:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-07-04 06:15:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '4-5' YEAR TO MONTH, INTERVAL '1 01:01:01' DAY TO SECOND, HEXTORAW('B1'), TO_BLOB(HEXTORAW('B2')), 'O''Connor Text 9', 'NText 9')");
		db.execute("INSERT INTO all_types_demo VALUES (10, 75.50, 88.88, 9.9, 99.99, 'Jack', 'Kappa', 'J', 'Q', TO_DATE('2026-08-19', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-08-19 03:00:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-08-19 03:00:00 +09:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-08-19 03:00:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '25-0' YEAR TO MONTH, INTERVAL '31 00:00:00' DAY TO SECOND, HEXTORAW('C1'), TO_BLOB(HEXTORAW('C2')), 'Text 10', 'NText 10')");
		db.execute("INSERT INTO all_types_demo VALUES (11, 10.10, 20.20, 30.3, 40.4, 'Kevin', 'Lambda', 'K', 'P', TO_DATE('2026-09-11', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-09-11 09:11:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-09-11 09:11:00 -06:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-09-11 09:11:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '0-0' YEAR TO MONTH, INTERVAL '0 09:11:00' DAY TO SECOND, HEXTORAW('D1'), TO_BLOB(HEXTORAW('D2')), 'Text 11', 'NText 11')");
		db.execute("INSERT INTO all_types_demo VALUES (12, 1212.12, 12.12, 12.1, 12.12, 'Leo', 'Mu', 'L', 'O', TO_DATE('2026-12-12', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-12-12 12:12:12', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-12-12 12:12:12 +12:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-12-12 12:12:12', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '12-2' YEAR TO MONTH, INTERVAL '12 12:12:12' DAY TO SECOND, HEXTORAW('E1'), TO_BLOB(HEXTORAW('E2')), 'Text 12', 'NText 12')");
		db.execute("INSERT INTO all_types_demo VALUES (13, 199.99, 1999.9, 19.9, 199.9, 'Mallory', 'Nu', 'M', 'N', TO_DATE('2099-12-31', 'YYYY-MM-DD'), TO_TIMESTAMP('2099-12-31 23:59:59', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2099-12-31 23:59:59 +00:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2099-12-31 23:59:59', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '75-10' YEAR TO MONTH, INTERVAL '19 23:59:59' DAY TO SECOND, HEXTORAW('F1'), TO_BLOB(HEXTORAW('F2')), 'Text 13', 'NText 13')");
		db.execute("INSERT INTO all_types_demo VALUES (14, 5.05, 55.55, 5.5, 55.55, 'Nina', 'Xi', 'N', 'M', TO_DATE('2026-05-05', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-05-05 05:05:05', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-05-05 05:05:05 -05:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-05-05 05:05:05', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '5-5' YEAR TO MONTH, INTERVAL '5 05:05:05' DAY TO SECOND, HEXTORAW('05'), TO_BLOB(HEXTORAW('05')), 'Text 14', 'NText 14')");
		db.execute("INSERT INTO all_types_demo VALUES (15, 99999999.99, 999999.9, 9999.9, 99999.9, 'Oscar', 'Omicron', 'O', 'L', TO_DATE('2026-10-10', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-10-10 10:10:10', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-10-10 10:10:10 +10:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-10-10 10:10:10', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '99-11' YEAR TO MONTH, INTERVAL '25 23:59:59' DAY TO SECOND, HEXTORAW('99'), TO_BLOB(HEXTORAW('99')), 'Text 15', 'NText 15')");
		db.execute("INSERT INTO all_types_demo VALUES (16, 1.01, 2.02, 3.03, 4.04, 'Peggy', 'Pi', 'P', 'K', TO_DATE('2026-04-01', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-04-01 01:01:01', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-04-01 01:01:01 -01:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-04-01 01:01:01', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '0-2' YEAR TO MONTH, INTERVAL '0 01:01:01' DAY TO SECOND, HEXTORAW('16'), TO_BLOB(HEXTORAW('16')), 'Text 16', 'NText 16')");
		db.execute("INSERT INTO all_types_demo VALUES (17, 700.00, 77.77, 7.7, 77.77, 'Quinn', 'Rho', 'Q', 'J', TO_DATE('2026-07-07', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-07-07 07:07:07', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-07-07 07:07:07 +07:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-07-07 07:07:07', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '7-7' YEAR TO MONTH, INTERVAL '7 07:07:07' DAY TO SECOND, HEXTORAW('17'), TO_BLOB(HEXTORAW('17')), NULL, 'NText 17')");
		db.execute("INSERT INTO all_types_demo VALUES (18, 18.18, 181.8, 1.8, 18.18, 'Sybil', 'Sigma', 'R', 'I', TO_DATE('2026-08-18', 'YYYY-MM-DD'), TO_TIMESTAMP('2026-08-18 18:18:18', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2026-08-18 18:18:18 -03:30', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2026-08-18 18:18:18', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '1-8' YEAR TO MONTH, INTERVAL '18 18:18:18' DAY TO SECOND, HEXTORAW('18'), TO_BLOB(HEXTORAW('18')), 'Text 18', 'こんにちは')");
		db.execute("INSERT INTO all_types_demo VALUES (19, 19.00, 190.0, 1.9, 19.9, 'Trent', 'Tau', 'S', 'H', TO_DATE('2000-01-01', 'YYYY-MM-DD'), TO_TIMESTAMP('2000-01-01 00:00:00', 'YYYY-MM-DD HH24:MI:SS'), TO_TIMESTAMP_TZ('2000-01-01 00:00:00 +00:00', 'YYYY-MM-DD HH24:MI:SS TZH:TZM'), TO_TIMESTAMP('2000-01-01 00:00:00', 'YYYY-MM-DD HH24:MI:SS'), INTERVAL '99-0' YEAR TO MONTH, INTERVAL '29 00:00:00' DAY TO SECOND, HEXTORAW('19'), TO_BLOB(HEXTORAW('19')), 'Text 19', 'NText 19')");
		
		db.query("SELECT * FROM ALL_TYPES_DEMO");
		
		// =========== //

		long id = 0;
		axon::recordset2r rc(db);

		std::string name;
		while (rc.next())
		{
			rc.get(0, id);
			rc.get(5, name);
			std::cout<<id<<")"<<name<<std::endl;
		}
		rc.done();
/*
		while (db.next())
		{
			std::string name, unit, joining;
			int eid = 0;
			
			// db.get<int>(0, eid);
			// db.get<std::string>(1, name);
			// db.get<std::string>(2, unit);
			// db.get<std::string>(3, joining);
			
			db.get(0, eid);
			db.get(1, name);
			db.get(2, unit);
			db.get(3, joining);

			// db>>eid>>name>>unit>>joining;
			
			std::cout<<counter++<<": "<<eid<<"//"<<name<<"//"<<unit<<"//"<<joining<<std::endl;

			// std::cout<<db; // print all columns
		}
*/		db.done();

		// db.execute("DROP TABLE TBL_EMPLOYEE_INFO");
		db.execute("DROP TABLE ALL_TYPES_DEMO");

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	std::cerr<<"runtime: "<<ctm.now()/1000.00<<"ms"<<std::endl;
	return 1;
}

