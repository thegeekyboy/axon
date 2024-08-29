#include <functional>
#include <csignal>

#include <axon.h>
#include <axon/util.h>
#include <axon/rabbit.h>

struct fileinfo {

	unsigned long index;
	unsigned long total;
	char filename[PATH_MAX];
	char batchid[64];
};

bool running = false;

namespace {
	std::function<void(int)> shutdown_handler;
	void signal_handler(int signal) { running = false; shutdown_handler(signal); }
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

	std::string vhost = "hyperion", queue = "test";
	int option = 0, count = 10;

	if (argc > 1) option = atoi(argv[1]);
	if (argc > 2) queue = argv[2];
	if (argc > 3) count = atoi(argv[3]);;

	if (option == 0)
	{
		axon::queue::producer p;
		// p.connect("localhost", 5672, vhost, "hyperion", "hyperion");
		p.connect("10.96.38.223", 30000, vhost, "hyperion-user", "hyperion-user");
		
		p.make_queue(queue);
		
		for (int i = 0; i < count; i++)
		{
			char fn[PATH_MAX];
			sprintf(fn, "filename=%d", i);
			struct fileinfo fi;
			fi.total=0;
			fi.index=0;
			strncpy(fi.batchid, fn, 64);
			sprintf(fi.filename, "%s", fn);

			p.push("amq.direct", queue,
				static_cast<char*>(static_cast<void*>(&fi)),
				sizeof(fi),
				axon::queue::encoding::binary
			);
			// p.push("amq.direct", queue, std::string("blah") + std::to_string(i)); // can be used to sent line of text!
		}
		p.close();
	}
	else
	{
		axon::queue::consumer c;
		std::string text;

		signal(SIGINT, signal_handler);
		shutdown_handler = [&](int signal) { printf("signal %d handler!\n", signal); c.stop(); };

		// c.connect("localhost", 5672, vhost, "hyperion", "hyperion");
		c.connect("10.96.38.223", 30000, vhost, "hyperion-user", "hyperion-user");

		c.make_queue(queue);
		// c.attach(queue, "amq.direct", queue); // if we want to use pop()

		running = true;

		// c.start([&](std::string txt) { std::cout<<queue<<": "<<txt<<std::endl; }); // use as lambda

		// axon::queue::envelope e = c.pop();
		// if (e.size > 0) std::cout<<e.ss.str()<<std::endl;
		// sleep(1);

		while (running) {
			// axon::queue::envelope e = c.pop();
			axon::queue::envelope e = c.get(queue);

			if (e.enc == axon::queue::encoding::text)
				std::cout<<e.ss.str()<<std::endl;
			else if (e.enc == axon::queue::encoding::binary)
			{
				struct fileinfo fio;

				std::string blah = e.ss.str();
				memcpy(&fio, blah.c_str(), sizeof(fio));
				std::cout<<fio.filename<<std::endl;
			}
		}

		c.delete_queue(queue);
		c.stop();
		c.close();
	}
}