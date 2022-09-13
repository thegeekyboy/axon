#include <axon.h>
#include <axon/connection.h>
#include <axon/util.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			connection::connection(std::string hostname, std::string username, std::string password)
			{
				_connected = false;

				const char* hostname_ip = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}";
				const char* hostname_fqdn = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$";
				
				const char* username_p = "^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$";

				boost::regex regex_ipaddr(hostname_ip);
				boost::regex regex_fqdn(hostname_fqdn);
				boost::regex regex_username(username_p);

				if (!boost::regex_match(hostname, regex_ipaddr) && !boost::regex_match(hostname, regex_fqdn))
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Wrong hostname format");
					return;
				}

				if (username.size() <= 0)
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Username cannot be empty");
					return;
				}

				if (!boost::regex_match(username, regex_username))
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Username format is wrong");
					return;
				}

				_hostname = hostname;
				_username = username;
				_password = password;
				_id = axon::helper::uuid();
			}

			connection::connection(const connection& rhs)
			{
				_hostname = rhs._hostname;
				_username = rhs._username;
				_password = rhs._password;
				_id = axon::helper::uuid();
			}

			connection::~connection()
			{
				DBGPRN("[%s] connection parent class dying.", _id.c_str());
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
		}
	}
}