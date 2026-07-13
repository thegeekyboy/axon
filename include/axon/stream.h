#ifndef AXON_STREAM_H_
#define AXON_STREAM_H_

#include <any>
#include <thread>

#include <boost/regex.hpp>
#include <axon/util.h>
#include <axon/resultset.h>

namespace axon {

	namespace stream {

		const int MIN_COMMIT_COUNT = 50;
		using cbfn = std::function<void(std::unique_ptr<axon::resultset>)>;

		struct topic {

			protected:

			std::string id;
			axon::status stats;

			public:

			// generic
			std::string name;
			std::string target;

			// reserved for individual use
			std::any reserved00;
			std::any reserved01;
			std::any reserved02;

			axon::stream::cbfn callback;

			topic(): id(axon::util::uuid()), stats(axon::status::unknown), callback(nullptr) { };
		};

		class connector {

			protected:
				unsigned long long _counter;

				std::string _id, _name;
				std::string _hostname, _username, _password;
				uint16_t _port;

				bool _runnable, _running, _connected, _subscribed;

				std::vector<topic> _topic;

				std::thread _runner, _daemon;
				std::function<void(std::unique_ptr<axon::resultset>)> _callback;

				virtual void _stop() = 0;

			public:

				connector() = delete;
				connector(const connector&) = delete;

				connector(std::string, std::string, std::string, uint16_t);
				connector(std::string, std::string, std::string);

				~connector();

				virtual std::string id() final { return _id; };
				virtual std::string& name() final { return _name; };
				virtual std::string source() const { return _name; }
				virtual size_t count() final { return _topic.size(); }
				virtual size_t counter() final { return _counter; }

				virtual void add(axon::stream::topic) final;
				virtual void add(std::string, std::string, std::function<void(std::unique_ptr<axon::resultset>)>) final;

				virtual void subscribe() = 0;
				virtual void unsubscribe() = 0;

				virtual bool start() = 0;
				virtual bool start(axon::stream::cbfn) = 0;
				virtual void stop() final;

				virtual void fetch(axon::resultset&, int) = 0;
		};
	};
}

#endif

