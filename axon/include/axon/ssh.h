#ifndef AXON_SSH_H_
#define AXON_SSH_H_

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <bzlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define AXON_TRANSFER_SSH_MODE       0x0001
#define AXON_TRANSFER_SSH_PRIVATEKEY 0x0002

namespace axon
{
	namespace transport
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
				~channel();

				void request_pty();
				void request_pty(std::string term);
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
				LIBSSH2_SESSION * _session;

			public:
				void set(int, auth_methods_t);
				void set(int, int);
				void set(int, std::string);
				
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

				LIBSSH2_SFTP * _sftp;

				bool init();

			public:
				sftp(std::string hostname, std::string username, std::string password) : connection(hostname, username, password) { _sftp = NULL; };
				~sftp();

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				int list(const axon::transport::transfer::cb &);
				int list(std::vector<entry> &);
				bool ren(std::string, std::string);
				bool del(std::string);

				int cb(const struct entry *);

				long long get(std::string, std::string, bool);
				long long put(std::string, std::string, bool);
			};
		}
	}
}

#endif