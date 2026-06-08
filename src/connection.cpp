#include <axon.h>
#include <axon/connection.h>
#include <axon/util.h>
#include <axon/uri.h>

namespace axon
{
	std::atomic<int> AwsStack::_instance = 0;
	std::mutex AwsStack::_lock;

	namespace transfer
	{
		connection::connection(std::string hostname, std::string username, std::string password, uint16_t port):
		_id(axon::util::uuid()), _hostname(hostname), _username(username), _password(password), _port(port), _connected(false), _fileopen(false)
		{
			BENCHMARK;

			if (!axon::util::validator::hostname(hostname))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "wrong hostname/ip address format");

			if (!axon::util::validator::username(username))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "username is empty or format is wrong");
		}

		connection::connection(std::string hostname, std::string username, std::string password):
		connection(hostname, username, password, -1)
		{

		}

		connection::connection(const connection& rhs):
		_id(axon::util::uuid()), _hostname(rhs._hostname), _username(rhs._username), _password(rhs._password)
		{
		}

		connection::~connection()
		{
		}

		bool connection::filter(std::string flt)
		{
			try {

				if (flt.size() > 0)
					_filter.push_back(boost::regex(flt));

			} catch (boost::regex_error &e) {

				return false;
			}

			return true;
		}

		bool connection::match(std::string filename)
		{
			bool retval = false;

			if (_filter.size() > 0)
			{
				for (auto &flt : _filter)
				{
					if (boost::regex_match(filename, flt))
						retval = true;
				}
			}
			else
				retval = true;

			return retval;
		}
	}
}

