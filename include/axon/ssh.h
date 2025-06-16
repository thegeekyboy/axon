#ifndef AXON_SSH_H_
#define AXON_SSH_H_

#include <mutex>

#include <libssh2.h>
#include <libssh2_sftp.h>

#define AXON_TRANSFER_SSH_MODE       0x0001
#define AXON_TRANSFER_SSH_PRIVATEKEY 0x0002
#define AXON_TRANSFER_SSH_USE_SCP    0x0004
#define AXON_TRANSFER_SSH_PORT       0x0004

namespace axon
{
	namespace transfer
	{
		typedef unsigned char auth_methods_t;

		struct auth_methods
		{
			static const auth_methods_t PASSWORD = 1;
			static const auth_methods_t PRIVATEKEY = 2;
			static const auth_methods_t INTERACTIVE = 4;
		};

		class channel
		{
			friend class session;
			LIBSSH2_CHANNEL* _channel;

			channel(LIBSSH2_CHANNEL* c)	{ _channel = c; }

		public:
			channel(LIBSSH2_SESSION*);
			~channel();

			void request_pty();
			void request_pty(std::string term);
			LIBSSH2_CHANNEL* get() { return _channel; };
		};

		class fingerprint
		{
			const char* _md5;
			const char* _sha1;

			std::string _hex_md5;
			std::string _hex_sha1;

			LIBSSH2_SESSION* _session;

		public:

			fingerprint(LIBSSH2_SESSION* s);
			const char *get_md5();
			const char *get_sha1();
			std::string get_hex_md5();
			std::string get_hex_sha1();
		};

		class session
		{
			friend class channel;
			int _sock;
			int _rc;

		protected:

			std::string _privkey;
			auth_methods_t _mode;
			bool _use_scp;

			LIBSSH2_SESSION *_session;

		public:
			void set(int, int);
			void set(int, bool);
			void set(int, std::string);
			void set(int, auth_methods_t);

			session();
			virtual ~session();

			LIBSSH2_SESSION *sessionid();
			void open(std::string host, unsigned short port);
			auth_methods_t get_auth_methods(std::string username);
			fingerprint get_host_fingerprint();
			void login(std::string username, std::string password); //throw(axon::exception);
			void login(std::string username, std::string pubkey, std::string privkey); //throw(axon::exception);
			channel* open_channel();
		};

		class sftp : public connection, public session {

			LIBSSH2_SFTP *_sftp;
			LIBSSH2_SFTP_HANDLE *_hsftp;

			std::mutex _lock;

			bool _connected;
			bool _is_open;

			bool init();
			long long _scp_get(std::string, std::string, bool);
			long long _sftp_get(std::string, std::string, bool);

		public:
			sftp(std::string hostname, std::string username, std::string password, uint16_t port): connection(hostname, username, password, port) { _sftp = NULL; _connected = false; _is_open = false; };
			sftp(std::string hostname, std::string username, std::string password): connection(hostname, username, password) { _sftp = NULL; _connected = false; _is_open = false; };
			sftp(const sftp& rhs) : connection(rhs) { _sftp = NULL; };
			~sftp();

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			int list(const axon::transfer::cb &);
			int list(std::vector<entry> &);
			long long copy(std::string, std::string, bool);
			long long copy(std::string src, std::string dest) { return copy(src, dest, false); };
			bool ren(std::string, std::string);
			bool del(std::string);

			int cb(const struct entry *);

			long long get(std::string, std::string, bool);
			long long put(std::string, std::string, bool);

			bool open(std::string, std::ios_base::openmode);
			bool close();

			bool push(axon::transfer::connection&);

			ssize_t read(char*, size_t);
			ssize_t write(const char*, size_t);
		};
	}
}

#endif
