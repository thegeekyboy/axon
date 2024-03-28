#include <iostream>
#include <string>

#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>
#include <axon/oracle.h>
#include <axon/scylladb.h>

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
	axon::timer ctm(__PRETTY_FUNCTION__);
	std::string address, username, password, keyspace;

	for(int i=0;env[i]!=NULL;i++)
	{
    	auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_HOSTNAME")
			address = parts[1];
		else if (parts[0] == "AXON_KEYSPACE")
			keyspace = parts[1];
	}

	if (argc <= 2) return 0;

	try {
		std::shared_ptr<axon::database::interface> _db;

		std::shared_ptr<axon::database::oracle> ora(new axon::database::oracle());
		_db = std::dynamic_pointer_cast<axon::database::interface>(ora);

		// std::shared_ptr<axon::database::scylladb> scylla(new axon::database::scylladb());
		// _db = std::dynamic_pointer_cast<axon::database::interface>(scylla);
		// (*scylla)[AXON_DATABASE_KEYSPACE] = keyspace;

		// std::shared_ptr<axon::database::sqlite> sqlite(new axon::database::sqlite());
		// _db = std::dynamic_pointer_cast<axon::database::interface>(sqlite);
		
		_db->connect(address, username, password);

		// _db->connect("/tmp/test.db", username, password);
		_db->ping();
		std::cout<<_db->version()<<std::endl;

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database::bind l = slist;

		int st = atoi(argv[1]);
		std::string argval = argv[2];

		axon::database::bind i = 1100, f = 15.1, s = (text*) "BD", addr = "BB840GPOUO";
		std::string orderid = "BB830GPN0J";
		int counter = 1;

		// _db->execute("SELECT * FROM CPSTXN.CPS_ORDERHIS WHERE ROWNUM < 10");
		// ora->execute("DELETE FROM ALL_DATA_TYPES WHERE TID = :rd", 10);
		// ora->execute("DELETE FROM ALL_DATA_TYPES WHERE TID < :tid", st);
		// _db->execute("select date_col, timestamp_col, timestamp_with_tz, timestamp_with_3_frac_sec_col FROM ALL_DATA_TYPES");
		// _db->execute("select sysdate cdate from dual");
		// db.execute("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE :x_1 = :y_3 AND ROWNUM < :rn", &f, &f, &i);
		// _db->query("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE TABLE_NAME = :tn", argval);
		// db.query("SELECT * FROM TRANSACTION_NORMALIZED WHERE ROWNUM > :rn", i);
		// db.query("select * from system.clients where port > :p ALLOW FILTERING", i);
		// db.execute("select * from system.clients where port > :p ALLOW FILTERING", i);
		// db.query("select orderid, actual_amount, eventcount from hyperion.transaction_normalized limit 10");
		// db.query("select * from hyperion.transaction_normalized where orderid = :oid");
		// _db->query("select * from users where email = :e", argval);
		// _db->query("select * from hyperion.cps_transaction_normalized");
		_db->query("SELECT * FROM EMP_UNIT WHERE UPPER(EMPNAME) LIKE :name", argval);
		// _db->query("SELECT * FROM EMP_UNIT WHERE EMPID < :eid", st);
		// _db->query("SELECT * FROM ALL_ALL_TABLES WHERE 1 < :rn", 2);
		// _db->query("SELECT * FROM EMP_UNIT");
		// db<<argval;

		while (_db->next())
		{
			// std::cout<<"=> "<<counter++;
			std::string name;
			long amount = 0;
		// 	int eventcount = 0;
		// // 	db.get<long>(1, amount);
		// // 	db.get<int>(2, eventcount);
			// _db->get(0, name);
			_db->get(1, name);
			// _db->get(1, name);
			// _db->get(2, name);
			// _db->get(3, name);
		// 	// db>>amount>>eventcount>>name;
		// 	std::cout<<"| long: "<<amount<<", int: "<<eventcount<<", text: "<<name;
		// // 	std::cout<<"Count: "<<cnt++<<", OrderID: "<<acctype<<", Account Type: "<<eventcount<<", Amount: "<<amount<<std::endl;

			std::cout<<counter++<<": "<<name<<std::endl;
		}
		_db->done();

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	return 1;
}