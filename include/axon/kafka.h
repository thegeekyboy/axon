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

#include <axon/stream2r.h>

#define AXON_KAFKA_POLL_TIMEOUT 100

namespace axon
{
	namespace stream2r
	{
		namespace rdk
		{
			class config {

				rd_kafka_conf_t *_kc;
				bool _new;

				public:
					config(): _kc(nullptr), _new(true) { _kc = rd_kafka_conf_new(); }
					config(const config &other): _new(false) { _kc = rd_kafka_conf_dup(other._kc); }
					config(const rd_kafka_conf_t *other): _new(false) { _kc = rd_kafka_conf_dup(other); }
					config(rd_kafka_t *other): _new(false) { _kc = rd_kafka_conf_dup(rd_kafka_conf(other)); }
					~config() { if (_kc) rd_kafka_conf_destroy(_kc); }

					void set(std::string name, std::string value) {
						char error[1024];
						if (rd_kafka_conf_set(_kc, name.c_str(), value.c_str(), error, sizeof(error)) != RD_KAFKA_CONF_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
						_new = false;
					}
					void reset() { _kc = nullptr; }
					bool is_new() const { return _new; }
					rd_kafka_conf_t *get() const {
						if (!_kc) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "config object not ready");
						return _kc;
					}
					operator rd_kafka_conf_t*() { return get(); }
			};

			class rdkafka {

				rd_kafka_t *_rk;

				public:
					rdkafka() = delete;
					rdkafka(const rdkafka&) = delete;
					rdkafka(config &conf, rd_kafka_type_t type = RD_KAFKA_PRODUCER) {
						char error[1024];
						_rk = rd_kafka_new(type, conf, error, sizeof(error));
						if (!_rk) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
						conf.reset();
					}
					~rdkafka() { if (_rk) rd_kafka_destroy(_rk); }
					rd_kafka_t *get() const { if (!_rk) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "rdkafka not initialized"); return _rk; }
					operator rd_kafka_t*() { return get(); }
					bool valid() const { return _rk != nullptr; }
			};

			class options {

				rd_kafka_AdminOptions_t *_opts;

				public:
					options() = delete;
					options(const options&) = delete;
					options(rdkafka &rdk, rd_kafka_admin_op_t op) {
						_opts = rd_kafka_AdminOptions_new(rdk.get(), op);
						if (!_opts) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create admin options");
					}
					~options() { if (_opts) rd_kafka_AdminOptions_destroy(_opts); }
					rd_kafka_AdminOptions_t *get() const { return _opts; }
					operator rd_kafka_AdminOptions_t*() { return get(); }
			};

			class event {

				rd_kafka_event_t *_ev;

				public:
					event(): _ev(nullptr) {};
					explicit event(rd_kafka_event_t *ev) : _ev(ev) {}

					~event() { if (_ev) rd_kafka_event_destroy(_ev); }

					rd_kafka_event_t *get() const { return _ev; }
					operator rd_kafka_event_t*() { return _ev; }
					bool valid() const { return _ev != nullptr; }

					event& operator=(rd_kafka_event_t *ev) {
						if (ev == _ev) return *this;
						if (_ev) rd_kafka_event_destroy(_ev);
						_ev = ev;

						return *this;
					}

					rd_kafka_event_type_t type() const {
						return _ev ? rd_kafka_event_type(_ev) : RD_KAFKA_EVENT_NONE;
					}
			};

			class queue {

				rd_kafka_queue_t *_queue;

				public:
					queue() = delete;
					queue(axon::stream2r::rdk::rdkafka &rdk): _queue(nullptr) {
						if (!rdk.valid()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "rd_kafka_t is null");
						if (!(_queue = rd_kafka_queue_new(rdk.get())))
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create queue");
					}
					~queue() { if (_queue) rd_kafka_queue_destroy(_queue); }
					rd_kafka_queue_t *get() const { return _queue; }
					operator rd_kafka_queue_t*() { return get(); }
			};

			class message {

				rd_kafka_message_t *_rdkm;
				axon::stream2r::rdk::rdkafka &_rdk;

				public:
					message(axon::stream2r::rdk::rdkafka &rdk): _rdkm(nullptr), _rdk(rdk) {
						if ((_rdkm = rd_kafka_consumer_poll(_rdk.get(), AXON_KAFKA_POLL_TIMEOUT)) && _rdkm->err)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, rd_kafka_message_errstr(_rdkm));
					}
					~message() {
						if (_rdkm) rd_kafka_message_destroy(_rdkm);
					}
					rd_kafka_message_t *get() const { return _rdkm; }
					std::string name() {
						if (!_rdkm || !_rdkm->rkt) return "";
						return rd_kafka_topic_name(_rdkm->rkt);
					}
					size_t size() const { return _rdkm ? _rdkm->len : 0; }
					void *payload() const { return _rdkm ? _rdkm->payload : nullptr; }
					bool empty() const { return _rdkm == nullptr; }
					void drain() {
						rd_kafka_message_t *msg;
						while ((msg = rd_kafka_consumer_poll(_rdk.get(), 100)) != nullptr)
							rd_kafka_message_destroy(msg);
					}
			};

			class serdes {

				bool _init { false };
				serdes_conf_t *_config;
				serdes_t *_serdes { nullptr };

				public:
					serdes() {
						if (!(_config = serdes_conf_new(nullptr, 0, nullptr)))
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize serdes config object");
						_init = true;
					}
					~serdes() { if (_serdes) serdes_destroy(_serdes); }

					void init() {
						char error[1024];
						if (!(_serdes = serdes_new(_config, error, 1023)))
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
					}
					void set(std::string name, std::string value) {
						char error[1024];
						if (serdes_conf_set(_config, name.c_str(), value.c_str(), error, sizeof(error)) != SERDES_ERR_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);
					}
					serdes_t *get() const { return _serdes; }
			};

			class subscription {

				rd_kafka_topic_partition_list_t *_subscription;

				public:
					subscription() = delete;
					subscription(int count);
					~subscription();

					void add(std::string topic);
					void remove(std::string topic);
					int  count();
					void info();

					rd_kafka_topic_partition_list_t *get();
					operator rd_kafka_topic_partition_list_t*() { return get(); }
			};
		}

		class kafka : public axon::stream2r::connector
		{
            bool _autocommit { true };
			bool _was_started { false };

 		    std::string _bootstrap_hosts;
            std::string _schema_hosts;
            std::string _consumer;
            std::vector<std::string> _topics;

            std::shared_ptr<axon::stream2r::rdk::rdkafka> _rdk;
            axon::stream2r::rdk::serdes _serdes;

			std::unique_ptr<axon::recordset2r> _deserialise(axon::stream2r::rdk::message&);
            void _avro_to_recordset(avro_value_t&, const std::string&, axon::recordset2r&);
			static void _rebalance(rd_kafka_t *r, rd_kafka_resp_err_t, rd_kafka_topic_partition_list_t *, void *);

            void _stop() override;
        public:

            kafka() = delete;
            kafka(const kafka&) = delete;

            kafka(std::string, std::string, std::string, axon::stream2r::rdk::config = axon::stream2r::rdk::config());
            ~kafka();

            void subscribe() override;
            void unsubscribe() override;

            bool start() override;
            bool start(axon::stream2r::cbfn) override;

			static void del(const std::string&, const std::string&, int = 10000);
			static void del(const std::string&, const std::vector<std::string>&, int = 10000);

            void fetch(axon::recordset2r&, int) override
            {
                throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kafka is callback-only — use start() with a callback");
            }

            bool autocommit() const { return _autocommit; }
            void autocommit(bool v) { _autocommit = v; }
        };
	}
}

#endif
