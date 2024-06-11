#include <iostream>
#include <string>
#include <memory>

#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>
#include <axon/oracle.h>
#include <axon/scylladb.h>

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

		axon::database::oracle ora;
		ora.connect(sid, username, password);
		axon::database::interface &db = ora;

		// axon::database::scylladb scylla;
		// scylla[AXON_DATABASE_KEYSPACE] = keyspace;
		// scylla.connect(hostname, username, password);
		// axon::database::interface &db = scylla;

		// axon::database::sqlite lite;
		// lite.connect("/tmp/test.db", username, password);
		// axon::database::interface &db = lite;

		db.ping();
		std::cout<<db.version()<<std::endl;

		if (argc <= 2) return 0;
		int ival = atoi(argv[1]);
		std::string sval = argv[2];

		// long blah = 1122;
		// char bs[23] = "this is a bs";

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database::bind l = slist;
		axon::database::bind i = 1100, f = 15.1, s = (text*) "BD", addr = (char*)"%BD%";

		db.execute("CREATE TABLE TBL_EMPLOYEE_INFO (EID INT, NAME VARCHAR2(256), DESIGNATION VARCHAR2(256), UNIT VARCHAR2(32), JOINING_DATE DATE, GENDER VARCHAR2(16), PRIMARY KEY (EID))");
		db.execute("INSERT INTO TBL_EMPLOYEE_INFO VALUES(1, 'SOME EMPLOYEE', 'CTO', 'TECHNOLOGY', '12-JAN-2020', 'Male')");
		db.execute("INSERT INTO TBL_EMPLOYEE_INFO VALUES(2, 'OTHER EMPLOYEE', 'CHRO', 'HUMAN RESOURCE', '1-SEP-2010', 'Female')");

		// db<<ival<<sval;
		// db.execute("INSERT INTO TBL_EMPLOYEE_INFO (EID, NAME) VALUES (:yx, :pp)", ival, sval);

		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO");
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE UNIT = :uname", sval);
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID = :eid", ival);

		db.query("SELECT * FROM TBL_EMPLOYEE_INFO");
		
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID < :eid");
		// db<<ival;

		// db.query("select * from TBL_EMPLOYEE_INFO where unit like :unit and eid < :eid");
		// db<<sval<<ival;

		int counter = 0;

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
		db.done();

		db.execute("DROP TABLE TBL_EMPLOYEE_INFO");

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	std::cerr<<"runtime: "<<ctm.now()/1000.00<<"ms"<<std::endl;
	return 1;
}