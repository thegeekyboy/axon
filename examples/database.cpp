#include <iostream>
#include <string>

#include <axon.h>
#include <axon/util.h>
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

	// try {
	// 	axon::database::scylladb db;
	// 	axon::database::bind test = 1.5;

	// 	db[AXON_DATABASE_KEYSPACE] = keyspace;
	// 	db.connect(address, username, password);

	// 	std::cout<<"scylladb version: " <<db.version()<<std::endl;
	
	// 	db.execute("SELECT * FROM BLAH WHERE X = ?", &test);
	// } catch (axon::exception &e) {
	// 	std::cerr<<"exception: "<<e.what()<<std::endl;
	// }

	try {
		axon::database::scylladb db;

		db[AXON_DATABASE_KEYSPACE] = keyspace;

		db.connect(address, username, password);
		db.ping();
		std::cout<<db.version();

		// std::vector<std::string> slist;
		// slist.push_back("HV1DGWAI58H385LM9LYACPCX");

		// axon::database::bind l = slist;
		int st = atoi(argv[1]);
		long blah = st;
		std::string orderid = (char*) argv[2];
		axon::database::bind i = st, f = 15.1, s = (text*) "asd", addr = "BB840GPOUO", ld = blah;
		CassUuid uuid;
		axon::database::bind x = &uuid;
		int cnt = 1;

		// db.execute("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE :x_1 = :y_3 AND ROWNUM < :rn", &f, &f, &i);

		// db.query("SELECT TABLE_NAME FROM ALL_ALL_TABLES WHERE :xk = ':ym' AND ROWNUM <= :rn and TABLE_NAME = :my", f, s, i, x);
		// db.query("SELECT * FROM CPS_TRANSACTION_NORMALIZED WHERE ROWNUM > :rn", i);
		// db.query("select * from system.clients where port > :p ALLOW FILTERING", i);
		// db.execute("select * from system.clients where port > :p ALLOW FILTERING", i);
		db.query("select orderid, actual_amount, eventcount from hyperion.cps_transaction_normalized limit 10");
		// db.query("select * from hyperion.cps_transaction_normalized where orderid = :oid");
		// db<<addr;
		while (db.next())
		{
			std::string acctype;
			long amount = 0;
			int eventcount = 0;
			db.get<long>(1, amount);
			db.get<int>(0, eventcount);
			db.get<std::string>(0, acctype);
			// db>>acctype>>amount>>eventcount;
			std::cout<<"Count: "<<cnt++<<", OrderID: "<<acctype<<", Account Type: "<<eventcount<<", Amount: "<<amount<<std::endl;
		}
		db.done();

	} catch (axon::exception &e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}

	return 1;
}