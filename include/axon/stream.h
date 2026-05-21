#ifndef AXON_STREAM_H_
#define AXON_STREAM_H_

#include <any>
#include <thread>

#include <boost/regex.hpp>
#include <axon/util.h>
#include <axon/recordset.h>

namespace axon {

	namespace stream {

		using cbfn = std::function<void(std::unique_ptr<axon::recordset>)>;

		struct topic {

			protected:

			std::string id;
			axon::status stats;

			public:

			// generic
			std::string name;
			std::string target;

			// reserved for individual use
			std::any data0;
			std::any data1;
			std::any data2;

			axon::stream::cbfn callback;

			topic(): id(axon::util::uuid()), stats(axon::status::unknown), callback(nullptr) { };
		};

		class interface {

			protected:
				unsigned long long _counter;

				std::string _id, _name;
				std::string _hostname, _username, _password;
				uint16_t _port;

				bool _runnable, _running, _connected, _subscribed;

				std::vector<topic> _topic;

				std::thread _runner, _daemon;
				std::function<void(std::unique_ptr<axon::recordset>)> _callback;

				virtual void _stop() = 0;

			public:

				interface() = delete;
				interface(const interface&) = delete;

				interface(std::string, std::string, std::string, uint16_t);
				interface(std::string, std::string, std::string);

				~interface();

				virtual std::string id() final { return _id; };
				virtual std::string& name() final { return _name; };
				virtual size_t count() final { return _topic.size(); }
				virtual size_t counter() final { return _counter; }

				virtual void add(axon::stream::topic) final;
				virtual void add(std::string, std::string, std::function<void(std::unique_ptr<axon::recordset>)>) final;

				virtual void subscribe() = 0;
				virtual void unsubscribe() = 0;

				virtual bool start() = 0;
				virtual bool start(axon::stream::cbfn) = 0;
				virtual void stop() final;

				virtual std::unique_ptr<axon::recordset> next() = 0;
		};
	};
}

#endif