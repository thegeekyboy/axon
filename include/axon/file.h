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
			FILE *_fp;

			bool init();

		public:
			file(std::string hostname, std::string username, std::string password, uint16_t port): connection(hostname, username, password, port), _fd(-1), _fp(NULL) {  };
			file(std::string hostname, std::string username, std::string password): connection(hostname, username, password), _fd(-1), _fp(NULL) {  };
			file(const file& rhs) : connection(rhs) {  };
			~file();

			bool set(char, std::string) { return true; };

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			size_t list(const axon::transfer::cb &);
			size_t list(std::vector<entry> &);
			off_t copy(std::string, std::string, bool);
			off_t copy(std::string src, std::string dest) { return copy(src, dest, false); };
			bool ren(std::string, std::string);
			bool del(std::string);

			int cb(const struct entry *);

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

