#include <axon.h>
#include <axon/util.h>
#include <axon/kinesis.h>

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

	axon::stream::kinesis k(hostname, username, password, 25);

	k.name() = "blah";
	
	std::cout<<k.name()<<std::endl;

	return 0;
}