#ifndef AXON_FILE_H_
#define AXON_FILE_H_

struct linux_dirent {
	unsigned long  d_ino;
	off_t          d_off;
	unsigned short d_reclen;
	char           d_name[PATH_MAX];
};

namespace axon
{
	namespace transfer
	{
		class file : public connection {

			int _fd = -1;

			bool init();

		public:
			file(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) {  };
			file(const file& rhs) : connection(rhs) {  };
			~file();

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			int list(const axon::transfer::cb &);
			int list(std::vector<entry> &);
			long long copy(std::string, std::string, bool = false);
			bool ren(std::string, std::string);
			bool del(std::string);

			int cb(const struct entry *);

			long long get(std::string, std::string, bool = false);
			long long put(std::string, std::string, bool = false);
		};
	}
}

#endif
