#include <iostream>
#include <string>

#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>
#include <axon/oracle.h>
#include <axon/scylladb.h>

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
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
		axon::timer(__PRETTY_FUNCTION__);
		// axon::database::scylladb db;
		
		axon::database::sqlite db;

		std::shared_ptr<axon::database::interface> _db;

		std::shared_ptr<axon::database::scylladb> scylla(new axon::database::scylladb());
		_db = std::dynamic_pointer_cast<axon::database::interface>(scylla);

		// std::shared_ptr<axon::database::sqlite> sqlite(new axon::database::sqlite());
		// _db = std::dynamic_pointer_cast<axon::database::interface>(sqlite);
		
		(*scylla)[AXON_DATABASE_KEYSPACE] = keyspace;
		_db->connect(address, username, password);

		// _db->connect("/tmp/test.db", username, password);
		_db->ping();
		std::cout<<_db->version()<<std::endl;

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database::bind l = slist;

		int st = atoi(argv[1]);
		std::string argval = argv[2];

		long blah = 1122;
		axon::database::bind i = 1100, f = 15.1, s = (text*) "asd", addr = "BB840GPOUO", ld = blah;
		std::string orderid = "BB830GPN0J";
		int counter = 1;

		// db.execute("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE :x_1 = :y_3 AND ROWNUM < :rn", &f, &f, &i);

		// db.query("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE :xk = ':ym' AND ROWNUM <= :rn and TABLE_NAME = :my", f, s, i, x);
		// db.query("SELECT * FROM TRANSACTION_NORMALIZED WHERE ROWNUM > :rn", i);
		// db.query("select * from system.clients where port > :p ALLOW FILTERING", i);
		// db.execute("select * from system.clients where port > :p ALLOW FILTERING", i);
		// db.query("select orderid, actual_amount, eventcount from hyperion.transaction_normalized limit 10");
		// db.query("select * from hyperion.transaction_normalized where orderid = :oid");
		// _db->query("select * from users where email = :e", argval);
		_db->query("select * from hyperion.cps_transaction_normalized");
		// db<<argval;
		while (_db->next())
		{
			std::cout<<"=> "<<counter++;
			std::string name;
			long amount = 0;
			int eventcount = 0;
		// 	db.get<long>(1, amount);
		// 	db.get<int>(2, eventcount);
			_db->get(0, name);
			// db>>amount>>eventcount>>name;
			std::cout<<"| long: "<<amount<<", int: "<<eventcount<<", text: "<<name;
		// 	std::cout<<"Count: "<<cnt++<<", OrderID: "<<acctype<<", Account Type: "<<eventcount<<", Amount: "<<amount<<std::endl;

			std::cout<<std::endl;
		}
		_db->done();

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	return 1;
}