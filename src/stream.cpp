#include <axon/util.h>
#include <axon/stream.h>

namespace axon {

	namespace stream {

		interface::interface(std::string hostname, std::string username, std::string password, uint16_t port):
		_id(axon::util::uuid()), _hostname(hostname), _username(username), _password(password), _port(port), _runnable(false), _running(false), _connected(false), _subscribed(false)
		{
			BENCHMARK;

			const char* r_hostname_ip = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}";
			const char* r_hostname_fqdn = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$";
			const char* r_username_p = "^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$";

			static const boost::regex regex_ipaddr(r_hostname_ip);
			static const boost::regex regex_fqdn(r_hostname_fqdn);
			static const boost::regex regex_username(r_username_p);

			if (!boost::regex_match(hostname, regex_ipaddr) && !boost::regex_match(hostname, regex_fqdn))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "wrong hostname/ip address format");

			if (username.size() <= 0 || !boost::regex_match(username, regex_username))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "username is empty or format is wrong");
		}

		interface::interface(std::string hostname, std::string username, std::string password): interface(hostname, username, password, -1) { }

		interface::~interface() { }

		void interface::add(axon::stream::topic t)
		{
			if (t.name.size() > 2 && t.target.size() > 2 && t.callback != nullptr)
				_topic.emplace_back(std::move(t));
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "topic data incomplete");
		}

		void interface::add(std::string name, std::string target, std::function<void(std::unique_ptr<axon::recordset>)> callback)
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

		void interface::stop()
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
	};
};
