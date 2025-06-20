#include <fstream>
#include <memory>
#include <atomic>
#include <iomanip>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <bzlib.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/ssh.h>
#include <axon/util.h>

namespace axon
{
	namespace transfer
	{
		static std::atomic_uint __libshh2_session_count;
		static std::mutex __libssh2_init_mutex;

		/* Helper Functions */

		std::string get_sftp_error_desc(unsigned long sftperr)
		{
			if (sftperr == LIBSSH2_FX_OK)
				return "LIBSSH2_FX_OK";
			else if (sftperr == LIBSSH2_FX_EOF)
				return "LIBSSH2_FX_EOF";
			else if (sftperr == LIBSSH2_FX_NO_SUCH_FILE)
				return "File/Folder not found or accessable";
			else if (sftperr == LIBSSH2_FX_PERMISSION_DENIED)
				return "Permission denied by remote system";
			else if (sftperr == LIBSSH2_FX_FAILURE)
				return "Operation failed";
			else if (sftperr == LIBSSH2_FX_BAD_MESSAGE)
				return "LIBSSH2_FX_BAD_MESSAGE";
			else if (sftperr == LIBSSH2_FX_NO_CONNECTION)
				return "Disconnected or no connection";
			else if (sftperr == LIBSSH2_FX_CONNECTION_LOST)
				return "Connection lost in operation";
			else if (sftperr == LIBSSH2_FX_OP_UNSUPPORTED)
				return "Unsupported operation";
			else if (sftperr == LIBSSH2_FX_INVALID_HANDLE)
				return "Invalid Handle";
			else if (sftperr == LIBSSH2_FX_NO_SUCH_PATH)
				return "No such path or directory";
			else if (sftperr == LIBSSH2_FX_FILE_ALREADY_EXISTS)
				return "File already exists";
			else if (sftperr == LIBSSH2_FX_WRITE_PROTECT)
				return "Remote system write protected";
			else if (sftperr == LIBSSH2_FX_NO_MEDIA)
				return "No Media";
			else if (sftperr == LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM)
				return "No space left on filesystem";
			else if (sftperr == LIBSSH2_FX_QUOTA_EXCEEDED)
				return "Quota Exceeded";
			else if (sftperr == LIBSSH2_FX_UNKNOWN_PRINCIPAL)
				return "LIBSSH2_FX_UNKNOWN_PRINCIPAL";
			else if (sftperr == LIBSSH2_FX_LOCK_CONFLICT)
				return "LIBSSH2_FX_LOCK_CONFLICT";
			else if (sftperr == LIBSSH2_FX_DIR_NOT_EMPTY)
				return "Directory is not empty";
			else if (sftperr == LIBSSH2_FX_NOT_A_DIRECTORY)
				return "Not a directory";
			else if (sftperr == LIBSSH2_FX_INVALID_FILENAME)
				return "Invalid filename";
			else if (sftperr == LIBSSH2_FX_LINK_LOOP)
				return "LIBSSH2_FX_LINK_LOOP";

			return nullptr; // Dont know if this is the right thing to do! Might cause unknown behavior?
		}

		channel::channel(LIBSSH2_SESSION* session)
		{
			if (!(_channel = libssh2_channel_open_session(session)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not open channel");
		}

		void channel::request_pty()
		{
			request_pty("vanilla");
		}

		void channel::request_pty(std::string term)
		{
			if (libssh2_channel_request_pty(_channel, term.c_str()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not request a pty.");
		}

		channel::~channel()
		{
			if (_channel)
				libssh2_channel_free(_channel);
		}

		fingerprint::fingerprint(LIBSSH2_SESSION* s)
		{
			this->_session = s;
			this->_md5  = libssh2_hostkey_hash(s, LIBSSH2_HOSTKEY_HASH_MD5);
			if(this->_md5 == NULL)
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not get MD5 signature.");
			}
			this->_sha1 = libssh2_hostkey_hash(s, LIBSSH2_HOSTKEY_HASH_SHA1);
			if(this->_sha1 == NULL)
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not get SHA1 signature.");
			}
		}

		const char * fingerprint::get_md5()
		{
			return this->_md5;
		}

		const char * fingerprint::get_sha1()
		{
			return this->_sha1;
		}

		std::string fingerprint::get_hex_md5()
		{
			if(this->_hex_md5.length() == 0)
			{
				std::string hex;
				std::stringstream s;

				s<<hex<<std::setfill('0');

				for(unsigned char i = 0; i < 16; i++)
				{
					s<<std::setw(2)<<int(this->_md5[i]);
					hex += ":" + s.str().substr(s.str().length() - 2, 2);
					s.str("");
				}
				this->_hex_md5 = hex.substr(1); // Get rid of the first colon
			}
			return this->_hex_md5;
		}

		std::string fingerprint::get_hex_sha1()
		{
			if(this->_hex_sha1.length() == 0)
			{
				std::string hex;
				std::stringstream s;

				s<<hex<<std::setfill('0');

				for(unsigned char i = 0; i < 20; i++)
				{
					s<<std::setw(2)<<int(this->_sha1[i]);
					hex += ":" + s.str().substr(s.str().length() - 2, 2);
					s.str("");
				}
				this->_hex_sha1 = hex.substr(1); // Get rid of the first colon
			}
			return this->_hex_sha1;
		}

		session::session()
		{
			std::lock_guard<std::mutex> lock(__libssh2_init_mutex);
			{
				if(__libshh2_session_count == 0)
				{
					if ((_rc = libssh2_init(0)) != 0)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error in libssh2_init()");

				}

				__libshh2_session_count++;
			}

			_use_scp = false;
			_mode = auth_methods::PASSWORD;
			_sock = socket(AF_INET, SOCK_STREAM, 0);

			this->_session = libssh2_session_init();
		}

		session::~session()
		{
			libssh2_session_disconnect(this->_session, "Closing connection, goodbye!");
			libssh2_session_free(this->_session);

			std::lock_guard<std::mutex> lock(__libssh2_init_mutex);
			{
				__libshh2_session_count--;
				if(__libshh2_session_count == 0)
				{
					libssh2_exit();
				}
			}

			close(_sock);
		}

		LIBSSH2_SESSION *session::sessionid()
		{
			return _session;
		}

		void session::open(std::string host, unsigned short port)
		{
			DBGPRN("requested session::open() to %s :: %d", host.c_str(), port);
			struct sockaddr_in sin;
			hostent *record = gethostbyname(host.c_str());

			sin.sin_family = AF_INET;
			sin.sin_port = htons(port);
			sin.sin_addr = *((struct in_addr *) record->h_addr);

			if (connect(_sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "socket exception at " + host + ":"+ std::to_string(port) + " - " + std::strerror(errno));

			libssh2_session_set_blocking(this->_session, 1);

			if ((_rc = libssh2_session_handshake(this->_session, this->_sock)) != 0)
			{
				std::string _errstr;

				if (_rc == LIBSSH2_ERROR_SOCKET_NONE)
					_errstr = "The socket is invalid";
				else if (_rc == LIBSSH2_ERROR_BANNER_SEND)
					_errstr = "Unable to send banner to remote host";
				else if (_rc == LIBSSH2_ERROR_KEX_FAILURE)
					_errstr = "Encryption key exchange with the remote host failed";
				else if (_rc == LIBSSH2_ERROR_SOCKET_SEND)
					_errstr = "Unable to send data on socket";
				else if (_rc == LIBSSH2_ERROR_SOCKET_DISCONNECT)
					_errstr = "The socket is disconnected";
				else if (_rc == LIBSSH2_ERROR_PROTO)
					_errstr = "An invalid SSH protocol response was received on the socket";

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "libssh2_session_handshake() - " + _errstr);
			}
		}

		auth_methods_t session::get_auth_methods(std::string username)
		{
			char* userauthlist;
			auth_methods_t types = 0;

			userauthlist = libssh2_userauth_list(this->_session, username.c_str(), username.length());

			if (strstr(userauthlist, "password") != NULL) {
				types |= auth_methods::PASSWORD;
			}
			if (strstr(userauthlist, "keyboard-interactive") != NULL) {
				types |= auth_methods::INTERACTIVE;
			}
			if (strstr(userauthlist, "publickey") != NULL) {
				types |= auth_methods::PRIVATEKEY;
			}

			return types;
		}

		fingerprint session::get_host_fingerprint()
		{
			return fingerprint(this->_session);
		}

		void session::login(std::string username, std::string password) //throw(axon::exception)
		{
			int code;

			// if ((code = libssh2_userauth_password(this->_session, username.c_str(), password.c_str())) != 0)
			if ((code = libssh2_userauth_password_ex(this->_session, username.c_str(), username.size(), password.c_str(), password.size(), NULL)) != 0)
			{
				std::string _errstr;

				if (code == LIBSSH2_ERROR_ALLOC)
					_errstr = "An internal memory allocation call failed";
				else  if (code == LIBSSH2_ERROR_SOCKET_SEND)
					_errstr = "Unable to send data on socket";
				else  if (code == LIBSSH2_ERROR_PASSWORD_EXPIRED)
					_errstr = "The password for the user has expired";
				else  if (code == LIBSSH2_ERROR_AUTHENTICATION_FAILED)
					_errstr = "Invalid username/password or public/private key";
				else
					_errstr = "Generic failure";

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "libssh2_userauth_password() - " + _errstr);
			}
		}

		void session::login(std::string username, std::string pubkey, std::string privkey) //throw(axon::exception)
		{
			int code;

			if ((code = libssh2_userauth_publickey_fromfile_ex(this->_session, username.c_str(), username.size(), pubkey.c_str(), privkey.c_str(), NULL)) != 0)
			{
				std::string _errstr;

				if (code == LIBSSH2_ERROR_ALLOC)
					_errstr = "An internal memory allocation call failed";
				else if (code == LIBSSH2_ERROR_SOCKET_SEND)
					_errstr = "Unable to send data on socket";
				else if (code == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					_errstr = "Socket timeout while authenticating";
				else if (code == LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED)
					_errstr = "The username/public key combination was invalid";
				else if (code == LIBSSH2_ERROR_AUTHENTICATION_FAILED)
					_errstr = "Authentication using the supplied public key was not accepted";
				else
					_errstr = "Generic failure";

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "libssh2_userauth_publickey_fromfile_ex() - " + _errstr);
			}
		}

		channel* session::open_channel()
		{
			LIBSSH2_CHANNEL* _channel;
			// TODO Check the return value for the specific error.
			if (!(_channel = libssh2_channel_open_session(this->_session)))
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not open channel");
			}
			channel* c = new channel(_channel);
			return c;
		}

		void session::set(int prop, auth_methods_t mode)
		{
			if (prop == AXON_TRANSFER_SSH_MODE)
			{
				_mode = mode;
			}
		}

		void session::set(int prop, bool value)
		{
			if (prop == AXON_TRANSFER_SSH_USE_SCP)
			{
				_use_scp = value;
			}
		}

		void session::set([[maybe_unused]] int prop, [[maybe_unused]] int value)
		{
			// not sure how to set this 
			// switch (prop)
			// {
			// 	case AXON_TRANSFER_SSH_PORT:
			// 		*this->connection::_port = value;
			// 		return true;
			// }
		}

		void session::set(int prop, std::string value)
		{
			if (prop == AXON_TRANSFER_SSH_PRIVATEKEY)
				_privkey = value;
		}

		// sftp class declaration
		long long sftp::_scp_get(std::string src, std::string dest, bool compress)
		{
			DBGPRN("[%s] requested sftp::_scp_get() = %s", _id.c_str(), src.c_str());
			LIBSSH2_CHANNEL *channel;
			char FILEBUF[MAXBUF];
			long long filesize = 0;
			struct stat fileinfo;

			std::string srcx, destx, temp;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if (!(channel = libssh2_scp_recv2(_session, srcx.c_str(), &fileinfo)))
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
				else {
					// char *errmsg;
					// int msglen;
					// libssh2_session_last_error(_session, &errmsg, &msglen, 0);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Generic error occured while opening file");
				}
			}

			std::ofstream file;
			file.exceptions(std::ofstream::badbit);

			try {

				file.open(dest, std::ios::out | std::ios::binary);
				boost::iostreams::filtering_ostream out;

				if (compress)
				{
					out.push(boost::iostreams::bzip2_compressor());
					out.push(file);
				}

				while (filesize < fileinfo.st_size)
				{
					memset(FILEBUF, '\0', MAXBUF);
					int rc = libssh2_channel_read(channel, FILEBUF, MAXBUF);

					if (rc > 0)
					{
						if (compress)
							out<<FILEBUF;
						else
							file<<FILEBUF;

						filesize += rc;
					}
					else
						break;
				}
			} catch (const std::ofstream::failure& e) {
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error opening file to writing - " + dest);
			}

			libssh2_channel_free(channel);
			channel = NULL;

			return filesize;
		}

		long long sftp::_sftp_get(std::string src, std::string dest, bool compress)
		{
			DBGPRN("[%s] requested sftp::_sftp_get() of %s", _id.c_str(), src.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			LIBSSH2_SFTP_HANDLE *hsftp;
			char FILEBUF[MAXBUF];
			unsigned long long filesize = 0;

			std::string srcx, destx, temp;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if (!(hsftp = libssh2_sftp_open(_sftp, srcx.c_str(), LIBSSH2_FXF_READ, 0)))
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr) + " - " + src);
				}
			}

			FILE *fp;
			BZFILE *bfp = NULL;
			int bzerr;
			unsigned int inbyte, outbyte;

			if (!(fp = fopen(dest.c_str(), "wb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error opening file for writing - " + dest);

			if (compress)
			{
				bfp = BZ2_bzWriteOpen(&bzerr, fp, 3, 0, 30);

				if (bzerr != BZ_OK)
				{
					BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
					fclose(fp);
					unlink(dest.c_str());

					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not open compression stream");
				}
			}

			do {

				int rc = libssh2_sftp_read(hsftp, FILEBUF, MAXBUF);

				if (rc > 0)
				{
					filesize += rc;
					if (compress)
					{
						BZ2_bzWrite(&bzerr, bfp, FILEBUF, rc);
						if (bzerr == BZ_IO_ERROR)
						{
							BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
							fclose(fp);
							unlink(dest.c_str());

							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error in compression stream");
						}
					}
					else
						fwrite(FILEBUF, rc, 1, fp);
				}
				else
					break;

			} while (true);

			if (compress)
				BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
			fflush(fp);
			fclose(fp);

			libssh2_sftp_close(hsftp);

			return filesize;
		}

		sftp::~sftp()
		{
			if (_fileopen) close();
			disconnect();
		}

		bool sftp::connect()
		{
			DBGPRN("[%s] requested sftp::connect() to %s", _id.c_str(), _hostname.c_str());
				axon::timer ctm(__PRETTY_FUNCTION__);

			if (_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already connected!");

			session::open(_hostname, _port);
			if (_mode == AXON_TRANSFER_SSH_PRIVATEKEY)
				session::login(_username, _privkey+".pub", _privkey);
			else
				session::login(_username, _password);
			init();

			_connected = true;

			return true;
		}

		bool sftp::disconnect()
		{
			if (_sftp)
				libssh2_sftp_shutdown(_sftp);

			_sftp = NULL;

			return true;
		}

		bool sftp::init()
		{
			char path[PATH_MAX];
			int len;


			// this can fail when accounts has expired but password or private key authentication is ok
			if ((_sftp = libssh2_sftp_init(_session)) == NULL)
			{
				char *errorMessage;
				libssh2_session_last_error(_session, &errorMessage, NULL, 0);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot initialize SFTP Session - " + errorMessage);
			}

			if ((len = libssh2_sftp_realpath(_sftp, ".", path, PATH_MAX-1)) <= 0)
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot retrieve home/landing folder path");
			}
			_path = path;

			return true;
		}

		bool sftp::chwd(std::string path)
		{
			DBGPRN("[%s] requested sftp::chwd to %s", _id.c_str(), path.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			LIBSSH2_SFTP_HANDLE *hsftp;


			if (!(hsftp = libssh2_sftp_opendir(_sftp, path.c_str())))
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
			}
			else
				_path = path;

			libssh2_sftp_closedir(hsftp);

			return true;
		}

		std::string sftp::pwd()
		{
			return _path;
		}

		bool sftp::mkdir(std::string dir)
		{
			DBGPRN("[%s] requested sftp::mkdir = %s", _id.c_str(), dir.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			int rc = 0;
			std::string dirx, cmd;
			channel c(_session);

			if (dir[0] == '/')
				dirx = dir;
			else
				dirx = _path + "/" + dir;

			cmd = "mkdir " + dirx;

			if ((rc = libssh2_channel_exec(c.get(), cmd.c_str())) != 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not execute command mkdir");

			// TODO: capture mkdir response and parse for error

			return true;
		}

		int sftp::list(const axon::transfer::cb &cbfn)
		{
			DBGPRN("[%s] requested sftp::list to %s", _id.c_str(), _path.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			int count = 0;
			LIBSSH2_SFTP_HANDLE *hsftp;

			if (!(hsftp = libssh2_sftp_opendir(_sftp, _path.c_str() )))
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
			}

			do
			{
				char filename[511];
				char longentry[512];
				LIBSSH2_SFTP_ATTRIBUTES attrs;

				if (libssh2_sftp_readdir_ex(hsftp, filename, sizeof(filename), longentry, sizeof(longentry), &attrs))
				{
					// if (!LIBSSH2_SFTP_S_ISDIR(attrs.permissions) && filename[0] != '.')
					// {
						if (match(filename))
						{
							struct entry file;

							file.name = filename;
							file.size = attrs.filesize;

							if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions))
								file.flag = axon::flags::DIR;
							else if (LIBSSH2_SFTP_S_ISLNK(attrs.permissions))
								file.flag = axon::flags::LINK;
							else if (LIBSSH2_SFTP_S_ISREG(attrs.permissions))
								file.flag = axon::flags::FILE;
							else if (LIBSSH2_SFTP_S_ISCHR(attrs.permissions))
								file.flag = axon::flags::CHAR;
							else if (LIBSSH2_SFTP_S_ISBLK(attrs.permissions))
								file.flag = axon::flags::BLOCK;
							else if (LIBSSH2_SFTP_S_ISFIFO(attrs.permissions))
								file.flag = axon::flags::FIFO;
							else if (LIBSSH2_SFTP_S_ISSOCK(attrs.permissions))
								file.flag = axon::flags::SOCKET;
							else {
								// TODO: this is not correct need to see what the issue is!!!!
								file.flag = axon::flags::FILE;
							}

							file.et = axon::protocol::SFTP;
							count++;

							cbfn(file);
						}
					// }
				}
				else
					break;

			} while (true);

			libssh2_sftp_closedir(hsftp);

			return count;
		}

		int sftp::list(std::vector<entry> &vec)
		{
			return list([&vec](const axon::entry &e) mutable { vec.push_back(e); });
		}

		long long sftp::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			DBGPRN("[%s] requested sftp::cp = %s, %s", _id.c_str(), src.c_str(), dest.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			int rc = 0;
			std::string srcx, cmd;
			channel c(_session);

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			auto [path, filename] = axon::util::splitpath(srcx);

			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

			cmd = "cp -f " + srcx + " " + dest;

			if ((rc = libssh2_channel_exec(c.get(), cmd.c_str())) != 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not execute command cp");

			// TODO:     capture cp response and parse for error

			return 0L;
		}

		bool sftp::ren(std::string src, std::string dest)
		{
			DBGPRN("[%s] requested sftp::ren() %s to %s", _id.c_str(), src.c_str(), dest.c_str());
			//std::lock_guard<std::mutex> lock(_lock); <- deadlock here
			int i;
			std::string srcx, destx;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			if ((i = libssh2_sftp_rename(_sftp, srcx.c_str(), destx.c_str())) != 0)
			{
				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
			}

			return true;
		}

		bool sftp::del(std::string src)
		{
			DBGPRN("[%s] requested sftp::del() of %s", _id.c_str(), src.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			int i;
			std::string srcx;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if ((i = libssh2_sftp_unlink(_sftp, srcx.c_str())) != 0)
			{
				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
			}

			return true;
		}

		long long sftp::get(std::string src, std::string dest, bool compress)
		{
			if (_is_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] data transfer stream already active.");

			if (_use_scp)
				return this->_scp_get(src, dest, compress);

			return this->_sftp_get(src, dest, compress);
		}

		long long sftp::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			DBGPRN("[%s] requested sftp::put() = %s", _id.c_str(), src.c_str());
			std::lock_guard<std::mutex> lock(_lock);
			LIBSSH2_SFTP_HANDLE *hsftp;
			std::string destx, temp;
			char *BUFPTR, FILEBUF[MAXBUF];
			int rc;
			unsigned long long filesize = 0;
			size_t sb;
			FILE *fp;

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			if (!(fp = fopen(src.c_str(), "rb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot open source file '" + src + "'");

			if (!(hsftp = libssh2_sftp_open(_sftp, destx.c_str(), LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH)))
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] An internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
				{
					fclose(fp);
					unsigned long sftperr = libssh2_sftp_last_error(_sftp);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
				}
			}

			do {

				sb = fread(FILEBUF, 1, MAXBUF, fp);
				if (sb <= 0) {
					break;
				}
				BUFPTR = FILEBUF;

				do {
					rc = libssh2_sftp_write(hsftp, BUFPTR, sb);

					if(rc < 0)
						break;

					BUFPTR += rc;
					sb -= rc;
					filesize += rc;

				} while (sb);

			} while (rc > 0);

			fclose(fp);
			libssh2_sftp_close(hsftp);

			return filesize;
		}

		bool sftp::open(std::string filename, std::ios_base::openmode om)
		{
			DBGPRN("[%s] requested file::open() %s to %s", _id.c_str(), filename.c_str(), ((om==std::ios::out)?"write":"read"));

			if (_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] a file is already open");

			std::string finalpath;

			if (filename[0] == '/')
				finalpath = filename;
			else
				finalpath = _path + "/" + filename;

			if (om & std::ios::out)
				_hsftp = libssh2_sftp_open(_sftp, finalpath.c_str(), LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
			else
				_hsftp = libssh2_sftp_open(_sftp, finalpath.c_str(), LIBSSH2_FXF_READ, 0);

			if (!_hsftp)
			{
				int i = libssh2_session_last_errno(_session);

				if (i == LIBSSH2_ERROR_ALLOC)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] internal memory allocation call failed");
				else if (i == LIBSSH2_ERROR_SOCKET_SEND)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] unable to send data on socket");
				else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] timeout while waiting on socket");
				else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(libssh2_sftp_last_error(_sftp)));
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] unknown sftp error");
			}

			_fileopen = true;
			_om = om;

			return _fileopen;
		}

		bool sftp::close()
		{
			DBGPRN("[%s] requested sftp::close()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			libssh2_sftp_close(_hsftp);
			_hsftp = NULL;
			_fileopen = false;

			return !_fileopen;
		}

		bool sftp::push(axon::transfer::connection& conn)
		{
			DBGPRN("[%s] requested sftp::push()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			char buffer[MAXBUF];
			ssize_t size;

			while ((size = this->read(buffer, MAXBUF-1)) > 0 || size == LIBSSH2_ERROR_EAGAIN)
				conn.write(buffer, size);

			return true;
		}

		ssize_t sftp::read(char* buffer, size_t size)
		{
			DBGPRN("[%s] requested sftp::read() => size(%ld)", _id.c_str(), size);

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::in))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform read operation when file is open for write");

			ssize_t rc = libssh2_sftp_read(_hsftp, buffer, size);

			if (rc == LIBSSH2_ERROR_EAGAIN)
				return LIBSSH2_ERROR_EAGAIN;
			else if (rc == LIBSSH2_ERROR_SFTP_PROTOCOL)
			{
				unsigned long sftperr = libssh2_sftp_last_error(_sftp);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, get_sftp_error_desc(sftperr));
			}

			return rc;
		}

		ssize_t sftp::write(const char* buffer, size_t size)
		{
			DBGPRN("[%s] requested sftp::write() => size(%ld)", _id.c_str(), size);

			ssize_t rc = 0, remaining = size, filesize = 0;

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::out))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform write operation when file is open for read");

			do {
				rc = libssh2_sftp_write(_hsftp, buffer, remaining);

				if(rc < 0)
					break;

				buffer += rc;
				remaining -= rc;
				filesize += rc;

			} while (remaining);

			return filesize;
		}
	}
}
