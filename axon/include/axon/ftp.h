#ifndef AXON_FTP_H_
#define AXON_FTP_H_

#include <bzlib.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			class ftp : public connection
			{
				transport::tcpip::socks _sock;
				std::thread _th;

				bool init();
				bool login();

			public:
				ftp(std::string hostname, std::string username, std::string password) : connection(hostname, username, password) {  };
				~ftp();

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				int list(callback);
				int list(std::vector<axon::entry> *);
				bool ren(std::string, std::string);
				bool del(std::string);

				long long get(std::string, std::string, bool);
				long long put(std::string, std::string, bool);
			};
		}
	}
}

#endif