#include <iostream>
#include <signal.h>

#include <axon.h>
#include <axon/util.h>

// #include <axon/scylladb.h>

#include <axon/stream.h>

#include <axon/kinesis.h>
// #include <axon/kafka.h>
// #include <axon/cqn.h>
#include <axon/ocn.h>


static bool canrun = false;
static unsigned long count = 0;

static void stop (int sig)
{
	canrun = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

// void counter(axon::stream::cqn *hook)
void counter()
{
	long tick = axon::timer::epoch();

	canrun = true;
	while (canrun)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		long tt = axon::timer::epoch();
		float tps = count/(tt-tick);
		tick = tt;

		std::cout<<"count: "<<count<<", "<<tps<<" rps"<<std::endl;

		count = 0;
	}

	// hook->stop();
}

// void parse(std::shared_ptr<axon::stream::recordset>, axon::database::scylladb*)
// {
// 	axon::timer ctm(__PRETTY_FUNCTION__);
// }
void parse2([[maybe_unused]] std::unique_ptr<axon::recordset2r> rc)
{
	std::cout<<count<<" got a ping()"<<std::endl;
	count++;
}

void parse(std::unique_ptr<axon::recordset2r> rc)
{
	if (rc) std::cout<<*rc<<std::endl;

	count++;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[], [[maybe_unused]] char* env[])
{
	const char *envp;
	std::string hostname, username, password, schema_registry, domain, ora_sid, krb5_keytab, krb5_cachepath, bootstrap, kafka_consumer_group, scylla_keyspace, proxy;

	if ((envp = std::getenv("http_proxy")) != nullptr) proxy = envp;
	if ((envp = std::getenv("AXON_DOMAIN")) != nullptr) domain = envp;
	if ((envp = std::getenv("AXON_ORA_SID")) != nullptr) ora_sid = envp;
	if ((envp = std::getenv("AXON_USERNAME")) != nullptr) username = envp;
	if ((envp = std::getenv("AXON_PASSWORD")) != nullptr) password = envp;
	if ((envp = std::getenv("AXON_HOSTNAME")) != nullptr) hostname = envp;
	if ((envp = std::getenv("AXON_BOOTSTRAP")) != nullptr) bootstrap = envp;
	if ((envp = std::getenv("AXON_KRB5_KEYTAB")) != nullptr) krb5_keytab = envp;
	if ((envp = std::getenv("AXON_KRB5_CACHEPATH")) != nullptr) krb5_cachepath = envp;
	if ((envp = std::getenv("AXON_SCHEMA_REGISTRY")) != nullptr) schema_registry = envp;
	if ((envp = std::getenv("AXON_SCYLLA_KEYSPACE")) != nullptr) scylla_keyspace = envp;
	if ((envp = std::getenv("AXON_KAFKA_CONSUMER_GROUP")) != nullptr) kafka_consumer_group = envp;

	// axon::stream::kafka source(bootstrap, schema, "axon_trx");

	signal(SIGINT, stop);

	try {
		// axon::stream::kinesis source(hostname, username, password);
		axon::stream2r::ocn source(ora_sid, username, password);

		// source.account() = "354285753755";
		// source.name() = "dse_uat_hyperion_event_consumer";
		// source.add("customerapp-event-stream", "customerapp-event-stream", parse);
		// source.add("uat-next-kinesis-stream", "uat-next-kinesis-stream", parse);
		// source.add("non_existing_stream", "non_existing_stream", parse);
		// source.add("uat-next-fp-kinesis-stream", "uat-next-fp-kinesis-stream", parse);
		// source.add("uat-next-kinesis-stream", "uat-next-kinesis-stream", parse2);

		source.connect();
		source.add("CPS_TRANS_RECORD", "SELECT * FROM CPSTXN.CPS_TRANS_RECORD", parse2);

		source.subscribe();

		canrun = true;

		source.start(parse);
		// source.start(nullptr);
		while (canrun) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		
		// source.start();
		// while (canrun)
		// {
		// 	parse(source.next());
		// 	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// }

		source.stop();

	} catch (axon::exception &e) {
		std::cerr<<e.what()<<std::endl;
	}

	// eventloop
	




	return 0;

/*
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
*/
/*
	while (canrun)
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

	// canrun = false;
	// th.join();

	return 0;
}

