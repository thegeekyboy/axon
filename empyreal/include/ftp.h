#ifndef FTP_H_
#define FTP_H_

namespace tcn
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
				int list(std::vector<tcn::entry> *);
				bool ren(std::string, std::string);
				bool del(std::string);

				long long get(std::string, std::string, bool);
				long long put(std::string, std::string);
			};
		}
	}
}

#endif