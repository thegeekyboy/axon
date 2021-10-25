#include <axon.h>
#include <connection.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			connection::connection(std::string hostname, std::string username, std::string password)
			{
				_connected = false;

				const char* hostname_p = "\\b(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]?|0)"
										"\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]?|0)"
										"\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]?|0)"
										"\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]?|0)\\b";
				
				const char* username_p = "^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$";

				boost::regex regex_ipaddr(hostname_p);
				boost::regex regex_username(username_p);

				if (!boost::regex_match(hostname, regex_ipaddr))
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
			}

			connection::~connection()
			{

			}

			bool connection::filter(std::string flt)
			{
				try {

					_filter.push_back(boost::regex(flt));

				} catch (boost::regex_error &e) {

					return false;
				}

				return true;
			}
		}
	}
}