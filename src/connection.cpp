#include <axon.h>
#include <axon/connection.h>
#include <axon/util.h>
#include <axon/uri.h>

namespace axon
{
	std::atomic<int> AwsStack::_instance = 0;
	std::mutex AwsStack::_lock;

	std::string protoname(axon::protocol i)
	{
		switch (i)
		{
			case axon::protocol::UNKNOWN:
				return "axon::protocol::UNKNOWN";
				break;

			case axon::protocol::NOTHING:
				return "axon::protocol::NOTHING";
				break;

			case axon::protocol::FILE:
				return "axon::protocol::FILE";
				break;

			case axon::protocol::SFTP:
				return "axon::protocol::SFTP";
				break;

			case axon::protocol::FTP:
				return "axon::protocol::FTP";
				break;

			case axon::protocol::S3:
				return "axon::protocol::S3";
				break;

			case axon::protocol::SAMBA:
				return "axon::protocol::SAMBA";
				break;

			case axon::protocol::SCP:
				return "axon::protocol::SCP";
				break;

			case axon::protocol::HDFS:
				return "axon::protocol::HDFS";
				break;

			case axon::protocol::DATABASE:
				return "axon::protocol::DATABASE";
				break;

			case axon::protocol::KAFKA:
				return "axon::protocol::KAFKA";
				break;

			default:
				return "axon::protocol::UNKNOWN";
				break;
		}

		return "axon::protocol::UNKNOWN";
	}

	axon::protocol protoid(std::string& name)
	{
		if (name == "NOTHING" || name == "AXON::PROTOCOL::NOTHING")
			return axon::protocol::NOTHING;
		else if (name == "FILE" || name == "AXON::PROTOCOL::FILE")
			return axon::protocol::FILE;
		else if (name == "SFTP" || name == "AXON::PROTOCOL::SFTP")
			return axon::protocol::SFTP;
		else if (name == "FTP" || name == "AXON::PROTOCOL::FTP")
			return axon::protocol::FTP;
		else if (name == "S3" || name == "AXON::PROTOCOL::S3")
			return axon::protocol::S3;
		else if (name == "SAMBA" || name == "AXON::PROTOCOL::SAMBA")
			return axon::protocol::SAMBA;
		else if (name == "SCP" || name == "AXON::PROTOCOL::SCP")
			return axon::protocol::SCP;
		else if (name == "HDFS" || name == "AXON::PROTOCOL::HDFS")
			return axon::protocol::HDFS;
		else if (name == "DATABASE" || name == "AXON::PROTOCOL::DATABASE")
			return axon::protocol::DATABASE;
		else if (name == "KAFKA" || name == "AXON::PROTOCOL::KAFKA")
			return axon::protocol::KAFKA;

		return axon::protocol::UNKNOWN;
	}

	std::string authname(axon::authtype i)
	{
		switch (i)
		{
			case axon::authtype::UNKNOWN:
				return "axon::authtype::UNKNOWN";
				break;

			case axon::authtype::PASSWORD:
				return "axon::authtype::PASSWORD";
				break;

			case axon::authtype::PRIVATEKEY:
				return "axon::authtype::PRIVATEKEY";
				break;

			case axon::authtype::KERBEROS:
				return "axon::authtype::KERBEROS";
				break;

			case axon::authtype::NTLM:
				return "axon::authtype::NTLM";
				break;

			default:
				return "axon::authtype::UNKNOWN";
				break;
		}

		return "axon::protocol::UNKNOWN";
	}

	axon::authtype authid(std::string& name)
	{
		if (name == "PASSWORD" || name == "AXON::AUTHTYPE::PASSWORD")
			return axon::authtype::PASSWORD;
		else if (name == "PRIVATEKEY" || name == "AXON::AUTHTYPE::PRIVATEKEY")
			return axon::authtype::PRIVATEKEY;
		else if (name == "KERBEROS" || name == "AXON::AUTHTYPE::KERBEROS")
			return axon::authtype::KERBEROS;
		else if (name == "NTLM" || name == "AXON::AUTHTYPE::NTLM")
			return axon::authtype::NTLM;

		return axon::authtype::UNKNOWN;
	}

	namespace transfer
	{
		connection::connection(std::string hostname, std::string username, std::string password, uint16_t port):
		_id(axon::util::uuid()), _hostname(hostname), _username(username), _password(password), _port(port), _connected(false), _fileopen(false)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

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
