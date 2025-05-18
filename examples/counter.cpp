#include <iostream>
#include <thread>
#include <signal.h>

#include <axon.h>
#include <axon/util.h>
#include <axon/database.h>
#include <axon/scylladb.h>
#include <axon/kafka.h>

static bool running = false;

static void stop (int sig)
{
	running = false;
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

void counter(axon::stream::kafka *hook)
{
	running = true;

	while (running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	hook->stop();
}

void dbg(avro_value_t *avro)
{
	char *as_json;

	if (avro_value_to_json(avro, 1, &as_json))
		fprintf(stderr, "=> avro_to_json failed: %s\n", avro_strerror());
	else {
		printf("%15s\n", as_json);
		free(as_json);
	}
}

void parse(axon::stream::recordset *rc)
{
	std::string event_type, screen, msisdn, session, event_date, fingerprint;

	rc->get("EVENT_TYPE", event_type);
	rc->get("DEVICE_FINGERPRINTID", fingerprint);
	rc->get("event_timestamp", event_date);
	rc->get("SESSION_ID", session);
	rc->get("MSISDN", msisdn);
	rc->get("SCREEN_NAME", screen);

	// if (event_type == "bKash.events.fp")
	// 	dbg(avro);
	
	std::cout<<"["<<event_date<<"] {"<<fingerprint<<" = "<<msisdn<<"} => "<<event_type<<" <> "<<session<<" - "<<screen<<std::endl;
}

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], [[maybe_unused]]char* env[])
{
	std::string address, username, password, keyspace, bootstrap, schema, consumer;

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
	}

	if (argc <= 2) return 0;

	axon::stream::kafka uat(bootstrap, schema, "axon::counter");

	uat.add("uat_streamproxy_generic");
	uat.subscribe();

	signal(SIGINT, stop);

	if (false)
	{
		uat.start(&parse);
		std::thread th(counter, &uat);
		th.join();
	}
	else 
	{
		running = true;

		std::unique_ptr<axon::stream::recordset> rc;

		while (running && (rc = uat.next()))
		{
			if (rc->is_empty())
				continue;

			std::string event_flag, event_type, screen, msisdn, session, event_date, fingerprint;

			rc->get("EVENT_FLAG", event_flag);
			rc->get("EVENT_TYPE", event_type);
			rc->get("DEVICE_FINGERPRINTID", fingerprint);
			rc->get("event_timestamp", event_date);
			rc->get("SESSION_ID", session);
			rc->get("MSISDN", msisdn);
			rc->get("SCREEN_NAME", screen);

			std::cout<<"["<<event_date<<"] {"<<fingerprint<<" = "<<msisdn<<"} => "<<event_type<<" <> "<<session<<" - "<<screen<<" "<<event_flag<<std::endl;
		}

		uat.stop();
	}

	return 0;
}
