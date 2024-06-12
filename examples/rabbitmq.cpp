#include <functional>
#include <csignal>

#include <axon.h>
#include <axon/util.h>
#include <axon/rabbit.h>

namespace {
std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }
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

	std::string name = "hyperion";
	int option = 0;

	if (argc > 1) option = atoi(argv[1]);

	if (option == 0)
	{
		axon::queue::producer p;
		p.connect("localhost", 5672, name, name, name);
		for (int i = 0; i < 10; i++) p.push(std::string("blah") + std::to_string(i), name, name);
		p.close();
	}
	else
	{
		axon::queue::consumer c;

		// signal(SIGINT, signal_handler);
		// shutdown_handler = [&](int signal) { c.stop(); };

		c.connect("localhost", 5672, name, name, name);
		c.attach(name, name, name);
		c.start([&](std::string txt) { std::cout<<name<<": "<<txt<<std::endl; });
		sleep(3);
		c.stop();
		c.close();
	}
}