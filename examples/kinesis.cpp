#include <signal.h>

#include <axon.h>
#include <axon/util.h>
#include <axon/kinesis.h>

static unsigned long count { 0 };
static bool canrun { false };

static void stop (int sig)
{
	canrun = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

void parse(std::unique_ptr<axon::resultset> rc)
{
	while (rc->next())
	{
		std::vector<uint8_t> data;

		rc->get("data", data);
		std::cout<<rc->source()<<" =>> "<<std::string(data.begin(), data.end());
		count++;
	}
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
	std::string proxy, hostname, username, password, keyspace, bootstrap, schema, consumer;

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
		else if (parts[0] == "AXON_BOOTSTRAP")
			bootstrap = parts[1];
		else if (parts[0] == "AXON_SCHEMA_REGISTRY")
			schema = parts[1];
		else if (parts[0] == "AXON_CONSUMER_GROUP")
			consumer = parts[1];
		else if (parts[0] == "http_proxy")
			proxy = parts[1];
	}

	if (username.empty() || password.empty() || hostname.empty())
		return 1;

	signal(SIGINT, stop);

	try {

		axon::stream::kinesis source(hostname, username, password);

		source.account() = "354285753755";
		source.name() = "dse_uat_hyperion_event_consumer";

		source.add("uat-next-fp-kinesis-stream", "uat-next-fp-kinesis-stream", parse);
		source.add("uat-next-kinesis-stream", "uat-next-kinesis-stream", parse);
		source.subscribe();

		source.start();
		canrun = true;

		while (canrun) std::this_thread::sleep_for(std::chrono::milliseconds(200));;

		source.stop();

	} catch(axon::exception& e) {
		std::cerr<<e.what()<<std::endl;
	}

	return 0;
}

