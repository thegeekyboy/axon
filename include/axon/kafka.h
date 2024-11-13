#ifndef AXON_KAFKA_H_
#define AXON_KAFKA_H_

#include <string>
#include <thread>
#include <functional>
#include <limits.h>

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
				kconf(): _kc(NULL) { _kc = rd_kafka_conf_new(); }
				~kconf() { if (_kc != NULL) rd_kafka_conf_destroy(_kc); };
				void set(std::string name, std::string value) {

					char error[1024];

					if (rd_kafka_conf_set(_kc, name.c_str(), value.c_str(), error, sizeof(error)) != RD_KAFKA_CONF_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
				};
				rd_kafka_conf_t *get() const {
					if (_kc == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "config object not ready");
					return _kc;
				};
				void reset() { _kc = NULL; }
				operator rd_kafka_conf_t*() { return get(); }
		};

		class serdes {

			bool _init = false;
			serdes_conf_t *_config;
			serdes_t *_serdes;

			public:
				serdes():
				_init(false) {
					if (!(_config = serdes_conf_new(NULL, 0, NULL)))
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize serdes config object");
					_init = true;
				};
				~serdes() { if (_init) serdes_destroy(_serdes); };
				void init() {
					char error[1024];

					if (!(_serdes = serdes_new(_config, error, 1023)))
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
				operator rd_kafka_topic_partition_list_t*() { return get(); }
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
					else if constexpr(std::is_same<T, long long>::value)
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
				void print();
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

#endif
