#ifndef AXON_CQN_H_
#define AXON_CQN_H_

#include <axon/oracle.h>

#include <thread>

// required reading https://docs.oracle.com/en/database/oracle/oracle-database/19/lnoci/notification-streams-advanced-queuing.html

namespace axon
{
	namespace stream
	{
		class recordset {

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

				recordset() = delete;
				recordset(std::any);
				~recordset();

				size_t get(std::string, void *, size_t);
				bool is_empty() { return _empty; }
				void print();
		};

		typedef ub4 (*cqnfn) (dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
		typedef void (*cbfn) (axon::database::operation, axon::database::change, std::string, std::string, void*);

		typedef struct {
			std::string id;
			std::string topic;
			std::any sub;
			cbfn callback;
			// std::function<void(axon::stream::recordset*)> callback;
			OCISubscription *subsptr;
		} topic_t;

		typedef struct {
			bool empty = true;
			axon::database::operation op;
			axon::database::change ch;
			std::string table;
			std::string rowid;
		} message_t;

		class csubscription {
			// OK RAII
			topic_t _topic;
			std::deque<message_t> _pipe;
			bool _subscribing;
			std::string _uuid;
			OCISubscription *_pointer;
			axon::database::environment _environment;
			axon::database::context *_context;
			axon::database::error _error;

			public:
				csubscription() = delete;
				csubscription(axon::database::context *ctx): _context(ctx) {
					axon::timer ctm(__PRETTY_FUNCTION__);

					_pointer = (OCISubscription *) 0;
					_uuid = axon::util::uuid();
					_subscribing = false;

					if (OCIHandleAlloc((dvoid *) _environment.get(), (dvoid **) &_pointer, OCI_HTYPE_SUBSCRIPTION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize subscription");
				}
				~csubscription() {
					axon::timer ctm(__PRETTY_FUNCTION__);

					if (_subscribing)
						detach();

					if (_pointer != (OCISubscription *) 0)
						OCIHandleFree(_pointer, (ub4) OCI_HTYPE_SUBSCRIPTION);
				}
				OCISubscription *get() {
					axon::timer ctm(__PRETTY_FUNCTION__);
					if (_pointer == (OCISubscription *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "subscription not ready");
					return _pointer;
				}
				operator OCISubscription*() { return get(); }

				void attach(axon::stream::topic_t&);
				void detach();
				static ub4 notify(dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
				void callback(axon::database::operation, axon::database::change, std::string, std::string);
				axon::stream::message_t pop() {
					if (!_pipe.empty())
					{
						axon::stream::message_t data = _pipe.front();
						_pipe.pop_front();
						return data;
					}

					return {};
				}
		};

		class cqn {

			// std::shared_ptr<axon::database::oracle> _oracle;
			axon::database::oracle *_oracle;
			std::vector<topic_t> _topics;
			bool _connected, _subscribing;
			std::string _uuid, _consumer;
			int _port;

			std::thread _runner;
			std::function<void(axon::stream::recordset*)> _interceptor;

			// axon::database::environment _environment;
			// axon::database::context _context;
			// axon::database::error _error;
			// axon::database::server _server;
			// axon::database::session _session;

			std::vector<std::unique_ptr<axon::stream::csubscription>> _list;

			public:
				cqn(axon::database::oracle*, std::string);
				cqn(axon::database::oracle*);
				~cqn();

				std::string subscribe(std::string, cbfn);
				// std::string subscribe(std::string, std::function<void(axon::stream::recordset*)>);
				std::string subscribe(std::string);

				void connect();
				void disconnect();

				void start();
				void stop();

				unsigned long long counter();

				// std::unique_ptr<axon::database::resultset> next();
				std::tuple<std::string, std::unique_ptr<axon::database::resultset>> next();
		};
	}
}

#endif
