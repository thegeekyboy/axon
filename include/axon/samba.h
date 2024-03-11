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

			static std::atomic<int> _instance;
			static std::mutex _mtx;

			bool init();

		public:
			samba(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) { };
			samba(const samba& rhs) : connection(rhs) {  };
			~samba();

			bool set(char, std::string);

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