#include <axon/util.h>
#include <axon/stream2r.h>

namespace axon {

	namespace stream2r {

		connector::connector(std::string hostname, std::string username, std::string password, uint16_t port):
		_id(axon::util::uuid()), _hostname(hostname), _username(username), _password(password), _port(port), _runnable(false), _running(false), _connected(false), _subscribed(false)
		{
			BENCHMARK;

			if (!axon::util::validator::hostname(hostname))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Wrong hostname/ip address format");

			if (!axon::util::validator::username(username))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Username is empty or format is wrong");
		}

		connector::connector(std::string hostname, std::string username, std::string password): connector(hostname, username, password, -1) { }

		connector::~connector() { }

		void connector::add(axon::stream2r::topic t)
		{
			if (t.name.size() > 2 && t.target.size() > 2 && t.callback != nullptr)
				_topic.emplace_back(std::move(t));
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "topic data incomplete");
		}

		void connector::add(std::string name, std::string target, std::function<void(std::unique_ptr<axon::recordset2r>)> callback)
		{
			topic t;

			if (name.size() > 2 && target.size() > 2 && callback != nullptr)
			{
				t.name = name;
				t.target = target;
				t.callback = callback;
				_topic.emplace_back(std::move(t));
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "topic data incomplete");

		}

		void connector::stop()
		{
			if (_runnable)
				_runnable = false;

			_stop();

			if (_daemon.joinable())
				_daemon.join();

			if (_runner.joinable())
				_runner.join();

			unsubscribe();
		}
	}
}

