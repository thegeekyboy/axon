#include <signal.h>

#include <axon.h>
#include <axon/scylladb.h>
#include <axon/cqn.h>

static bool running = false;
static unsigned long count = 0;

static void stop (int sig)
{
	running = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

// void counter(axon::stream::cqn *hook)
void counter()
{
	long tick = axon::timer::epoch();

	running = true;
	while (running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		long tt = axon::timer::epoch();
		float tps = count/(tt-tick);
		tick = tt;

		// std::cout<<"count: "<<count<<", "<<tps<<" rps"<<std::endl;

		count = 0;
	}

	// hook->stop();
}

void parse(std::shared_ptr<axon::stream::recordset>, axon::database::scylladb*)
{
	axon::timer ctm(__PRETTY_FUNCTION__);
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], char* env[])
{
	std::string address, username, password, keyspace, bootstrap, schema, consumer, sid;

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
		else if (parts[0] == "AXON_BOOTSTRAP")
			bootstrap = parts[1];
		else if (parts[0] == "AXON_SCHEMA_REGISTRY")
			schema = parts[1];
		else if (parts[0] == "AXON_CONSUMER_GROUP")
			consumer = parts[1];
		else if (parts[0] == "AXON_SID")
			sid = parts[1];
	}

	// axon::stream::kafka source(bootstrap, schema, "axon_trx");
	axon::database::oracle ora;
	ora.connect(sid, username, password);
	axon::stream::cqn source(&ora, "axon_trx");
	
	source.connect();
	std::thread th(counter);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	source.subscribe("SELECT * FROM CPSTXN.CPS_ORDERHIS", [](axon::database::operation, axon::database::change, std::string, std::string rowid, void *xsb) {
		axon::stream::csubscription *sub = static_cast<axon::stream::csubscription*>(xsb);
		axon::stream::message_t data = sub->pop();

		if (!data.empty) {
			DBGPRN("rowid: %s, table: %s", data.rowid.c_str(), data.table.c_str());

			std::string sql = "SELECT * FROM " + data.table + " WHERE ROWID = '" + data.rowid + "'";
		
			// std::shared_ptr<axon::database::statement> stmt = std::make_shared<axon::database::statement>(ora.get_context());
			// stmt->prepare(sql);
			// stmt->execute(axon::database::exec_type::select);

			// axon::database::resultset rs(stmt);

			// return std::make_tuple(data.table, std::make_unique<axon::database::resultset>(stmt));
			
		}
	});
	source.subscribe("SELECT * FROM CPSTXN.CPS_ORDER_REFDATA", [](axon::database::operation, axon::database::change, std::string table, std::string rowid, void *) {
		std::cout<<"callback for "<<table<<" and rowid"<<rowid<<std::endl;
	});
	source.subscribe("SELECT * FROM CPSTXN.CPS_TRANS_RECORD", [](axon::database::operation, axon::database::change, std::string table, std::string rowid, void *) {
		std::cout<<"callback for "<<table<<" and rowid"<<rowid<<std::endl;
	});
	// source.subscribe("SELECT * FROM BLAH");

	signal(SIGINT, stop);

	// axon::database::scylladb db;
	// db[AXON_DATABASE_KEYSPACE] = keyspace;
	// db.connect(address, username, password);

	source.start();
	// std::shared_ptr<axon::database::tableinfo> inf = db.getinfo("cps_transaction_normalized");
	// std::cout<<">> "<<inf->column_exists("credit_party_id")<<std::endl;

/*
	while (running)
	{
		// std::unique_ptr<axon::database::resultset> rc = source.next();
		auto [table, rc] = source.next();

		if (!rc)
			std::this_thread::sleep_for(std::chrono::milliseconds(80));
		else
		{
			if (rc->next())
			{
				std::string blah;
				// blah = rc->column_count();
				std::cout<<table<<": "<<rc->get_string(0)<<std::endl;
			}
		}
	}
*/

	// running = false;
	th.join();

	return 0;
}