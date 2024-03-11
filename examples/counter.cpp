#include <thread>
#include <signal.h>

#include <axon.h>
#include <axon/util.h>
#include <axon/database.h>
#include <axon/scylladb.h>



/* start kafka.h */
#include <librdkafka/rdkafka.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

namespace axon
{
	namespace stream
	{
		typedef void (*cbfn)(avro_value_t *);
		typedef void *(*cbfnptr)(void *);

		const int MIN_COMMIT_COUNT = 50;

		class kconf {

			rd_kafka_conf_t *_kc;

			public:
				kconf() { _kc = rd_kafka_conf_new(); }
				~kconf() { if (_kc) rd_kafka_conf_destroy(_kc); };
				void set(std::string name, std::string value) {

					char error[1024];

					if (rd_kafka_conf_set(_kc, name.c_str(), value.c_str(), error, sizeof(error)) != RD_KAFKA_CONF_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
				};
				rd_kafka_conf_t *get() const { return _kc; };
				void reset() { _kc = NULL; }
		};

		class serdes {

			bool _init = false;
			serdes_conf_t *_config;
			serdes_t *_serdes;

			public:
				serdes() {
					if (!(_config = serdes_conf_new(NULL, 0, NULL)))
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize serdes config object");
					_init = true;
				};
				~serdes() { if (_init) serdes_destroy(_serdes); };
				void init() {
					char error[1024];

					if (!(_serdes = serdes_new(_config, error, sizeof(error))))
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
				}
				void set(std::string name, std::string value) {

					char error[1024];

					if (serdes_conf_set(_config, name.c_str(), value.c_str(), error, sizeof(error)) != SERDES_ERR_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
				};
				serdes_t *get() const { return _serdes; };
		};

		class subscription {

			rd_kafka_topic_partition_list_t *_subscription;

			public:
				subscription() = delete;
				subscription(int);
				~subscription();

				void add(std::string);
				void remove(std::string);
				int count();
				rd_kafka_topic_partition_list_t *get();
				void info();
		};

		class recordset {

			avro_value_t _data;

			std::string _get_string(std::string);
			int _get_int(std::string);
			long _get_long(std::string);
			float _get_float(std::string);
			double _get_double(std::string);

			bool _empty;

			public:
				template <typename T>
				bool get(std::string name, T& p) {

					if constexpr(std::is_same<T, int>::value)
						p = static_cast<T>(_get_int(name));
					else if constexpr(std::is_same<T, long>::value)
						p = static_cast<T>(_get_long(name));
					else if constexpr(std::is_same<T, float>::value)
						p = static_cast<T>(_get_float(name));
					else if constexpr(std::is_same<T, double>::value)
						p = static_cast<T>(_get_double(name));
					else if constexpr(std::is_same<T, std::string>::value)
						p = static_cast<T>(_get_string(name));
					else return false;

					return true;
				}

				// recordset() = delete;
				recordset(axon::stream::serdes&, rd_kafka_message_t*);
				~recordset();

				size_t get(std::string, void *, size_t);
				bool is_empty() { return _empty; }
		};

		class kafka {

			std::string _bootstrap_hosts, _schema_hosts, _consumer;
			std::vector<std::string> _topic;

			serdes _serdes;

			rd_kafka_t *_rk;
			rd_kafka_resp_err_t err;

			char _hostname[HOST_NAME_MAX], error[512];

			bool _runnable, _subscribed;
			unsigned long long _counter;

			std::thread _runner;
			std::function<void(axon::stream::recordset*)> f;

			public:
				kafka(std::string, std::string, std::string);
				~kafka();

				bool add(std::string);

				void subscribe();
				bool start(std::function<void(axon::stream::recordset*)>);
				void stop();
				
				unsigned long long counter();

				std::unique_ptr<axon::stream::recordset> next();
		};
	}
}
/* end kafka.h */

/* start kafka.cpp */

namespace axon
{
	namespace stream
	{
// ### class subscription
		subscription::subscription(int count):
			_subscription(NULL)
		{
			_subscription = rd_kafka_topic_partition_list_new(count);
		}

		subscription::~subscription()
		{
			if (_subscription)
				rd_kafka_topic_partition_list_destroy(_subscription);
		}

		void subscription::add(std::string topic)
		{
			if (rd_kafka_topic_partition_list_add(_subscription, topic.c_str(), RD_KAFKA_PARTITION_UA) == NULL)
				std::cout<<"null returned"<<std::endl;
		}

		int subscription::count()
		{
			return _subscription->cnt;
		}

		rd_kafka_topic_partition_list_t *subscription::get()
		{
			return _subscription;
		}

		void subscription::info()
		{
			for (int i = 0; i < _subscription->cnt; i++) {
				rd_kafka_topic_partition_t *p = &_subscription->elems[i];
				printf("Topic \"%s\" partition %" PRId32, p->topic,
						p->partition);
				if (p->err)
						printf(" error %s", rd_kafka_err2str(p->err));
				else {
						printf(" offset %" PRId64 "", p->offset);

						if (p->metadata_size)
								printf(" (%d bytes of metadata)",
										(int)p->metadata_size);
				}
				printf("\n");
			}
		}

// ### class recordset

		std::string recordset::_get_string(std::string name)
		{
			std::string value;
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_STRING)
				{
					const char *buffer;
					size_t size;

					avro_value_get_string(&branch, &buffer, &size);

					if (size <= 0)
						return 0;

					value = buffer;
				}
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return value;
		}

		int recordset::_get_int(std::string name)
		{
			int32_t value = 0;
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_INT32)
					avro_value_get_int(&branch, &value);
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return value;
		}

		long recordset::_get_long(std::string name)
		{
			int64_t value = 0;
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_INT64)
					avro_value_get_long(&branch, &value);
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return value;
		}

		float recordset::_get_float(std::string name)
		{
			float value = 0;
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_FLOAT)
					avro_value_get_float(&branch, &value);
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return value;
		}

		double recordset::_get_double(std::string name)
		{
			double value = 0;
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_DOUBLE)
					avro_value_get_double(&branch, &value);
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return value;
		}

		size_t recordset::get(std::string name, void *value, size_t size)
		{
			avro_value_t init_iden_value;

			if (avro_value_get_by_name(&_data, name.c_str(), &init_iden_value, NULL) == 0)
			{
				int retcode = 0;
				avro_value_t branch;

				if ((retcode = avro_value_set_branch(&init_iden_value, 1, &branch)) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "branch not found");

				if (avro_value_get_type(&branch) == AVRO_BYTES)
				{
					const void *buffer;
					size_t bsz = 0;

					avro_value_get_bytes(&branch, &buffer, &bsz);

					if (bsz <= 0)
						return 0;

					memcpy(value, buffer, std::min(size, bsz));

					return std::min(size, bsz);
				}
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch");
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value not found");

			return 0;
		}

		recordset::recordset(axon::stream::serdes &src, rd_kafka_message_t *rkm)
		{
			char error[1024];

			if (rkm)
			{
				_empty = false;

				if (serdes_deserialize_avro(src.get(), &_data, NULL, rkm->payload, rkm->len, error, 1024))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);

				rd_kafka_message_destroy(rkm);
			}
			else
				_empty = true;
		}

		recordset::~recordset()
		{
			if (!_empty)
				avro_value_decref(&_data);
		}

// ## class kafka

		kafka::kafka(std::string bootstrap, std::string schema, std::string consumer): 
			_bootstrap_hosts(bootstrap),
			_schema_hosts(schema),
			_consumer(consumer)
		{
			kconf config;

			if (gethostname(_hostname, sizeof(_hostname)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Failed to lookup hostname");

			_serdes.set("schema.registry.url", _schema_hosts);
			_serdes.init();

			config.set("client.id", _hostname);
			config.set("group.id", _consumer);
			config.set("enable.partition.eof", "false");
			config.set("enable.auto.commit", "true");
			config.set("auto.offset.reset", "earliest");
			config.set("bootstrap.servers", _bootstrap_hosts);
#ifdef DEBUG
			// config.set("debug", "topic,metadata,feature");
			// rd_kafka_conf_set_log_cb(config.get(), logger);
#endif

			if (!(_rk = rd_kafka_new(RD_KAFKA_CONSUMER, config.get(), error, sizeof(error))))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);

			rd_kafka_poll_set_consumer(_rk);

			config.reset();

			_runnable = false;
			_subscribed = false;

			_counter = 0;
		}

		kafka::~kafka()
		{
			stop();
			rd_kafka_destroy(_rk);
		}

		bool kafka::add(std::string topic)
		{
			_topic.push_back(topic);

			return true;
		}

		void kafka::subscribe()
		{
			if (!_rk)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kafka connection not initialized");

			if (_topic.size() < 1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "topic list is empty");

			subscription sb(_topic.size());
			
			for (std::string &topic : _topic)
				sb.add(topic.c_str());
			
			if ((err = rd_kafka_subscribe(_rk, sb.get())))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, std::string(rd_kafka_err2str(err)));

			DBGPRN("=> Subscribed to %d topic(s), waiting for rebalance and messages...", sb.count());

			_subscribed = true;
		}

		bool kafka::start(std::function<void(axon::stream::recordset*)> fn)
		{
			if (!_subscribed)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected yet!");

			f = fn;
			_runnable = true;

			_runner = std::thread([this] () {

				while (_runnable)
				{
					rd_kafka_message_t *rkm;

					if (!(rkm = rd_kafka_consumer_poll(_rk, 300)))
						continue;

					if (rkm->err)
					{
						DBGPRN("=> Consumer error: %d->%s", rkm->err, rd_kafka_message_errstr(rkm));
						rd_kafka_message_destroy(rkm);

						continue;
					}

					try {
						recordset rc(_serdes, rkm);
						f(&rc);
					} catch(const std::bad_function_call& e) {
						std::cout<<e.what()<<std::endl;
					}

					if ((_counter % MIN_COMMIT_COUNT) == 0)
					{
						INFPRN("%s - rd_kafka_commit() at %d", __PRETTY_FUNCTION__, _counter);
						err = rd_kafka_commit(_rk, NULL, 0);
						if (err)
							DBGPRN("=> Consumer error: %s\n", rd_kafka_err2str(err));
					}

					_counter++;
				}
			});

			return true;
		}

		void kafka::stop()
		{
			if (_runnable)
				_runnable = false;

			if (_runner.joinable())
				_runner.join();

			if (_subscribed)
			{
				rd_kafka_unsubscribe(_rk);
				_subscribed = false;
			}

			if (_rk)
				rd_kafka_consumer_close(_rk);
		}

		unsigned long long kafka::counter()
		{
			return _counter;
		}

		std::unique_ptr<axon::stream::recordset> kafka::next()
		{
			if (_runnable)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid operation while callback active");

			rd_kafka_message_t *rkm = rd_kafka_consumer_poll(_rk, 300);

			if (rkm && rkm->err)
			{
				DBGPRN("=> Consumer error: %d->%s", rkm->err, rd_kafka_message_errstr(rkm));
				rd_kafka_message_destroy(rkm);

				return std::unique_ptr<axon::stream::recordset>(nullptr);
			}

			return std::make_unique<axon::stream::recordset>(_serdes, rkm);
		}
	}
}
/* end kafka.cpp */

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