#include <iostream>
#include <string>
#include <thread>

#include <unistd.h>
#include <limits.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

#include <librdkafka/rdkafka.h>

#include <axon.h>
#include <axon/kafka.h>
#include <axon/util.h>

namespace axon
{
	namespace stream
	{
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

			if (!(_rk = rd_kafka_new(RD_KAFKA_CONSUMER, config.get(), error, sizeof(error))))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);

			rd_kafka_poll_set_consumer(_rk);

			config.reset();

			_runnable = false;
			_connected = false;

			_counter = 0;
		}

		kafka::~kafka()
		{
			stop();
		}

		bool kafka::add(std::string topic)
		{
			_topic.push_back(topic);

			return true;
		}

		void kafka::connect()
		{
			if (!_rk)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kafka connection not initialized");

			if (_topic.size() < 1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "topic list is empty");

			_subscription = rd_kafka_topic_partition_list_new(_topic.size());
			
			for (std::string &topic : _topic)
				rd_kafka_topic_partition_list_add(_subscription, topic.c_str(), RD_KAFKA_PARTITION_UA);
			
			if ((err = rd_kafka_subscribe(_rk, _subscription)))
			{
				DBGPRN("=> Failed to subscribe to %d topics: %s\n", _subscription->cnt, rd_kafka_err2str(err));

				rd_kafka_topic_partition_list_destroy(_subscription);
				rd_kafka_destroy(_rk); // <-- this needs to go in destructor

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to subscribe to topic");
			}

			DBGPRN("=> Subscribed to %d topic(s), waiting for rebalance and messages...\n", _subscription->cnt);
			rd_kafka_topic_partition_list_destroy(_subscription);

			_connected = true;
		}

		bool kafka::start(std::function<void(avro_value_t *)> fn)
		{
			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected yet!");

			f = fn;
			_runnable = true;

			_runner = std::thread([this] () {

				unsigned long long counter = 0;

				while (_runnable)
				{
					axon::timer(__PRETTY_FUNCTION__);
					rd_kafka_message_t *rkm;

					if (!(rkm = rd_kafka_consumer_poll(_rk, 300)))
					{
						counter = 0;
						continue;
					}

					if (rkm->err)
					{
						DBGPRN("=> Consumer error: %d->%s\n", rkm->err, rd_kafka_message_errstr(rkm));
						rd_kafka_message_destroy(rkm);

						continue;
					}

					try {
						avro_value_t avro;
						char error[1024];

						if (serdes_deserialize_avro(_serdes.get(), &avro, NULL, rkm->payload, rkm->len, error, 1024))
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);

						f(&avro);
						avro_value_decref(&avro);

					} catch(const std::bad_function_call& e) {
						std::cout<<e.what()<<std::endl;
					}

					rd_kafka_message_destroy(rkm);

					if ((counter % MIN_COMMIT_COUNT) == 0)
					{
						err = rd_kafka_commit(_rk, NULL, 0);
						if (err)
						{
							DBGPRN("=> Consumer error: %s\n", rd_kafka_err2str(err));
						}
					}

					counter++;
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

			if (_connected)
			{
				rd_kafka_unsubscribe(_rk);
				_connected = false;
			}

			if (_rk)
			{
				rd_kafka_consumer_close(_rk);
				rd_kafka_destroy(_rk);
				_rk = NULL;
			}
		}

		unsigned long long kafka::counter()
		{
			return _counter;
		}
	}
}