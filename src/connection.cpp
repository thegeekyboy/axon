#include <axon.h>
#include <axon/connection.h>
#include <axon/util.h>
#include <axon/uri.h>

namespace axon
{
	namespace transfer
	{
		connection::connection(std::string hostname, std::string username, std::string password, uint16_t port)
		{
			_id = axon::util::uuid();
			_connected = false;

			DBGPRN("[%s] connection %s class starting.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());

			const char* r_hostname_ip = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}";
			const char* r_hostname_fqdn = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$";
			const char* r_username_p = "^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$";

			boost::regex regex_ipaddr(r_hostname_ip);
			boost::regex regex_fqdn(r_hostname_fqdn);
			boost::regex regex_username(r_username_p);
			
			if (!boost::regex_match(hostname, regex_ipaddr) && !boost::regex_match(hostname, regex_fqdn))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Wrong hostname/ip address format");

			if (username.size() <= 0 || !boost::regex_match(username, regex_username))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Username is empty or format is wrong");

			_hostname = hostname;
			_username = username;
			_password = password;
			_port = port;
			
		}

		connection::connection(const connection& rhs)
		{
			_hostname = rhs._hostname;
			_username = rhs._username;
			_password = rhs._password;
			_id = axon::util::uuid();
		}

		connection::~connection()
		{
			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
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