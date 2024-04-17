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

	if (argc <= 2) return 0;

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

		int ival = atoi(argv[1]);
		std::string sval = argv[2];

		// long blah = 1122;
		// char bs[23] = "this is a bs";

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database::bind l = slist;
		axon::database::bind i = 1100, f = 15.1, s = (text*) "BD", addr = (char*)"%BD%";

		// db.execute("DROP TABLE TABLE TBL_EMPLOYEE_INFO");
		// db.execute("CREATE TABLE TBL_EMPLOYEE_INFO(EID NUMBER(5), NAME VARCHAR2(256), DESIGNATION VARCHAR2(256), UNIT VARCHAR2(8), GRADE VARCHAR2(8), SUPERVISOR_NAME VARCHAR2(256), OFFICE_EMAIL VARCHAR2(256), PERSONAL_EMAIL VARCHAR2(256), MOBILE_NUMBER VARCHAR2(32), ALTERNATE_NUMBER VARCHAR2(32), AREA VARCHAR2(128), HAS_VPN NUMBER(1), HAS_RESIGNED NUMBER(1), JOINING_DATE DATE, LAST_WORKING_DATE DATE, LAST_PROMOTION_DATE DATE, EXPERIENCE_BEFORE_JOINING NUMBER(4), TOTAL_EXPERIENCE NUMBER(4), GENDER VARCHAR2(16), PRIMARY KEY (EID))");

		// db<<ival<<sval;
		// db.execute("INSERT INTO TBL_EMPLOYEE_INFO (EID, NAME) VALUES (:yx, :pp)", ival, sval);

		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO");
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE UNIT = :uname", sval);
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID = :eid", ival);
		
		// db.query("SELECT * FROM TBL_EMPLOYEE_INFO WHERE EID < :eid");
		// db<<ival;

		db.query("select * from TBL_EMPLOYEE_INFO where unit like :unit and eid < :eid");
		db<<sval<<ival;

		int counter = 0;

		while (db.next())
		{
			std::string name, unit, joining;
			int eid = 0;
			
			// db.get<int>(0, eid);
			// db.get<std::string>(1, name);
			// db.get<std::string>(2, unit);
			// db.get<std::string>(13, joining);
			
			db.get(0, eid);
			db.get(1, name);
			db.get(2, unit);
			db.get(13, joining);

			// db>>eid>>name>>unit;
			
			std::cout<<counter++<<": "<<eid<<"//"<<name<<"//"<<unit<<"//"<<joining<<std::endl;

			// std::cout<<db; // print all columns
		}
		db.done();

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	std::cerr<<"runtime: "<<ctm.now()/1000.00<<"ms"<<std::endl;
	return 1;
}