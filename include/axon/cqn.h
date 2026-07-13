#ifndef AXON_CQN_H_
#define AXON_CQN_H_

#include <axon/oracle.h>
#include <axon/recordset2r.h>

#include <thread>

// required reading https://docs.oracle.com/en/database/oracle/oracle-database/19/lnoci/notification-streams-advanced-queuing.html

namespace axon
{
	namespace stream
	{
		typedef ub4 (*cqnfn) (dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
		typedef void (*cbfn) (axon::database2r::operation, axon::database2r::change, std::string, std::string, void*);

		typedef struct {
			std::string id;
			std::string topic;
			std::any sub;
			cbfn callback;
			// std::function<void(axon::recordset2r*)> callback;
			OCISubscription *subsptr;
		} topic_t;

		typedef struct {
			bool empty = true;
			axon::database2r::operation op;
			axon::database2r::change ch;
			std::string table;
			std::string rowid;
		} message_t;

		class cqn {

			// sub classes
			class subscription {

				OCISubscription *_pointer;

				topic_t _topic;
				std::deque<message_t> _pipe;
				bool _subscribing;
				std::string _uuid;

				axon::database2r::environment _environment;
				std::shared_ptr<axon::database2r::context> _context;
				axon::database2r::error _error;

				public:
					subscription() = delete;
					subscription(std::shared_ptr<axon::database2r::context> ctx): _pointer(nullptr), _uuid(axon::util::uuid()), _context(ctx) {
						BENCHMARK;

						if (OCIHandleAlloc((dvoid *) _environment.get(), (dvoid **) &_pointer, OCI_HTYPE_SUBSCRIPTION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize subscription");
					}
					~subscription() {
						BENCHMARK;

						if (_subscribing)
							detach();

						if (_pointer != (OCISubscription *) 0)
							OCIHandleFree(_pointer, (ub4) OCI_HTYPE_SUBSCRIPTION);
					}
					OCISubscription *get() {
						BENCHMARK;
						if (_pointer == (OCISubscription *) 0)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "subscription not ready");
						return _pointer;
					}
					operator OCISubscription*() { return get(); }

					void attach(axon::stream::topic_t&);
					void detach();
					static ub4 notify(dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
					void callback(axon::database2r::operation, axon::database2r::change, std::string, std::string);
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
			// sub classes

			// std::shared_ptr<axon::database2r::oracle> _oracle;
			axon::database2r::oracle *_oracle;
			std::vector<topic_t> _topics;
			bool _connected, _subscribing;
			std::string _uuid, _consumer;
			int _port;

			std::thread _runner;
			std::function<void(axon::recordset2r*)> _interceptor;

			// axon::database2r::environment _environment;
			// axon::database2r::context _context;
			// axon::database2r::error _error;
			// axon::database2r::server _server;
			// axon::database2r::session _session;

			std::vector<std::unique_ptr<axon::stream::cqn::subscription>> _list;

			public:
				cqn(axon::database2r::oracle*, std::string);
				cqn(axon::database2r::oracle*);
				~cqn();

				std::string subscribe(std::string, cbfn);
				// std::string subscribe(std::string, std::function<void(axon::recordset2r*)>);
				std::string subscribe(std::string);

				void connect();
				void disconnect();

				void start();
				void stop();

				unsigned long long counter();

				// std::unique_ptr<axon::database2r::recordset2r> next();
				std::tuple<std::string, std::unique_ptr<axon::recordset2r>> next();
		};
	}
}

#endif
