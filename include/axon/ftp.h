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
				ftp(const ftp& rhs) : connection(rhs) {  };
				~ftp();

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				bool mkdir(std::string);
				int list(const axon::transport::transfer::cb &);
				int list(std::vector<axon::entry> &);
				long long copy(std::string &, std::string &, bool = false);
				bool ren(std::string, std::string);
				bool del(std::string);

				long long get(std::string, std::string, bool = false);
				long long put(std::string, std::string, bool = false);
			};
		}
	}
}

#endif