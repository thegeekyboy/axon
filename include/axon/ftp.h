#ifndef AXON_FTP_H_
#define AXON_FTP_H_

#include <initializer_list>
#include <string>

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

			std::string _wait_for(const std::string &cmd, std::initializer_list<std::string> expected, std::initializer_list<std::string> errors);

		public:
			ftp(std::string, std::string, std::string, uint16_t);
			ftp(std::string, std::string, std::string);
			ftp(const ftp&);
			~ftp();

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			size_t list(const axon::transfer::cb &);
			size_t list(std::vector<axon::entry> &);
			off_t copy(std::string, std::string, bool);
			off_t copy(std::string, std::string);
			bool ren(std::string, std::string);
			bool del(std::string);

			off_t get(std::string, std::string, bool);
			off_t put(std::string, std::string, bool);

			bool open(std::string, std::ios_base::openmode);
			bool close();

			ssize_t read(char*, size_t);
			ssize_t write(const char*, size_t);
		};
	}
}

#endif

