#include <axon.h>
#include <axon/connection.h>
#include <axon/util.h>
#include <axon/uri.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			connection::connection(std::string uri_, std::string username, std::string password)
			{
				_connected = false;

				const char* r_hostname_ip = "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}";
				const char* r_hostname_fqdn = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$";
				const char* r_port = "^[0-9]{0,5}$";
				
				const char* r_username_p = "^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$";

				boost::regex regex_ipaddr(r_hostname_ip);
				boost::regex regex_fqdn(r_hostname_fqdn);
				boost::regex regex_port(r_port);
				boost::regex regex_username(r_username_p);

				axon::helper::uri u = axon::helper::uri::parse(uri_);

				if (!boost::regex_match(u.host, regex_ipaddr) && !boost::regex_match(u.host, regex_fqdn))
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Wrong hostname format");
					return;
				}

				if (u.port.size() > 0 && !boost::regex_match(u.port, regex_ipaddr))
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Wrong port format");
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

				_hostname = u.host;
				_port = (u.port.size())?std::stoi(u.port):0;
				_username = username;
				_password = password;
				_id = axon::helper::uuid();

				if (u.protocol.size() >= 3)
				{
					if (u.protocol == "file")
						_proto = axon::protocol::FILE;
					else if (u.protocol == "sftp")
						_proto = axon::protocol::SFTP;
					else if (u.protocol == "ftp")
						_proto = axon::protocol::FTP;
					else if (u.protocol == "s3")
						_proto = axon::protocol::S3;
					else if (u.protocol == "smb" || u.protocol == "samba")
						_proto = axon::protocol::SAMBA;
					else if (u.protocol == "hdfs")
						_proto = axon::protocol::HDFS;
					else if (u.protocol == "aws")
						_proto = axon::protocol::AWS;
					else if (u.protocol == "scp")
						_proto = axon::protocol::SCP;
					else if (u.protocol == "db" || u.protocol == "oracle" || u.protocol == "sqlite")
						_proto = axon::protocol::DATABASE;
					else if (u.protocol == "kafka")
						_proto = axon::protocol::KAFKA;
					else if (u.protocol == "https" || u.protocol == "http")
					{
					}
					else
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Protocl/Schema is not supported");
				}
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