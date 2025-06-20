#ifndef AXON_FTP_H_
#define AXON_FTP_H_

namespace axon
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
			ftp(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) {  };
			ftp(const ftp& rhs) : connection(rhs) {  };
			~ftp();

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			int list(const axon::transfer::cb &);
			int list(std::vector<axon::entry> &);
			long long copy(std::string, std::string, bool);
			bool ren(std::string, std::string);
			bool del(std::string);

			long long get(std::string, std::string, bool);
			long long put(std::string, std::string, bool);
		};
	}
}

#endif
