#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/s3.h>


int main()
{
	std::string proxy, address, username, password, keyspace, bootstrap, schema, consumer;

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
		else if (parts[0] == "http_proxy")
			proxy = parts[1];
	}

	if (username.empty() || password.empty() !hostname.empty())
		return 1;

	axon::transfer::s3 conn(hostname, , "SECRET KEY", 8080);
	std::vector<axon::entry> v;

	conn.set(AXON_TRANSFER_S3_PROXY, proxy);

	try {
		
		conn.connect();
		conn.chwd("/bucket-name/");
		conn.pwd();
		// conn.login();
		// conn.ren("ftp.test", "mark.test");
		// conn.del("cantdel");

		if (conn.list(v))
		{
			for (auto &elm : v)
				if (elm.flag == axon::flags::FILE)
					std::cout<<"elm.name<<std::endl";
		}

		conn.disconnect();
	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}