#ifndef AXON_MML_H_
#define AXON_MML_H_

#include <boost/format.hpp>
#include <boost/regex.hpp>
namespace axon
{
	namespace telecom
	{
		namespace core
		{
			class mml
			{
				transport::tcpip::socks _sock;

				std::string _hostname;
				unsigned short int _port;

				std::thread _th;
				boost::regex _success, _error, _break;

			protected:

				short int exec(std::string cmd) { return exec(cmd, NULL, 5, nullptr); }
				short int exec(std::string cmd, std::vector<std::string> *vec) { return exec(cmd, vec, 5, nullptr); }
				short int exec(std::string cmd, unsigned short int timeout) { return exec(cmd, NULL, timeout, nullptr); }
				short int exec(std::string cmd, const std::function<int(std::string, short int&)> &cb) { return exec( cmd, NULL, 5, cb); }
				short int exec(std::string cmd, std::vector<std::string> *vec, unsigned short int timeout) { return exec(cmd, vec, timeout, nullptr); }

				short int exec(std::string, std::vector<std::string> *, unsigned short int, const std::function<int(std::string, short int&)> &);

			public:

				mml();
				~mml();

				bool connect(std::string, unsigned short int);
				bool disconnect();
				bool conditions(std::string, std::string, std::string);
			};


			class huamsc : public mml
			{
				std::string _username, _password;
				std::string _msc;

			public:

				bool login(std::string, std::string);
				bool logout();

				bool reg(std::string);
				bool unreg();

				short int addclrdsg(std::string, unsigned short int);
				short int delclrdsg(std::string, unsigned short int);

				bool query(telecom::subscriber &);
			};

			class huahlr : public mml
			{
				std::string _username, _password;

			public:

				bool login(std::string, std::string);
				bool logout();

				bool remove(std::string);
				bool vlrnum(telecom::subscriber &);
			};
		}
	}
}

#endif