#ifndef AXON_SAMBA_H_
#define AXON_SAMBA_H_

#define AXON_TRANSFER_SAMBA_DOMAIN 'd'
#define AXON_TRANSFER_SAMBA_SHARE 's'

namespace axon
{
	namespace transfer
	{
		class samba : public connection {

			std::string _domain, _share;

			struct smb2_context *_smb2;
			struct smb2dir *_dir;
			struct smb2fh *_fh;
			uint64_t _offset;

			static std::atomic<int> _instance;
			static std::mutex _mtx;

			bool init();
			std::string _smb2path(const std::string &);

		public:
			samba(std::string hostname, std::string username, std::string password, uint16_t port): connection(hostname, username, password, port) { };
			samba(std::string hostname, std::string username, std::string password): connection(hostname, username, password) { };
			samba(const samba& rhs): connection(rhs) {  };
			~samba();

			bool set(char, std::string);

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

