#include <axon.h>
#include <connection.h>
#include <ssh.h>

namespace axon
{
	namespace transport
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
					return "File not found";
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

			void channel::request_pty()
			{
				request_pty("vanilla");
			}

			void channel::request_pty(std::string term)
			{
				if (libssh2_channel_request_pty(_channel, term.c_str()) )
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Could not request a pty.");
				}
			}

			channel::~channel()
			{
				libssh2_channel_free(_channel);
			}

			fingerprint::fingerprint(LIBSSH2_SESSION* s)
			{
				this->_session = s;
				this->_md5  = libssh2_hostkey_hash(s, LIBSSH2_HOSTKEY_HASH_MD5);
				if(this->_md5 == NULL)
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Could not get MD5 signature.");
				}
				this->_sha1 = libssh2_hostkey_hash(s, LIBSSH2_HOSTKEY_HASH_SHA1);
				if(this->_sha1 == NULL)
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Could not get SHA1 signature.");
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
							std::cout<<"There was an error in libssh2_init()"<<std::endl;

					}

					__libshh2_session_count++;
				}

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
				struct sockaddr_in sin;
				sin.sin_family = AF_INET;
				sin.sin_port = htons(port);
				sin.sin_addr.s_addr = inet_addr(host.c_str());
				
				if (connect(_sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0)
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Socket Exception!");
				}
				
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

					throw axon::exception(__FILENAME__, __LINE__, __func__, "libssh2_session_handshake() - " + _errstr);
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
					types |= auth_methods::KEYS;
				}

				return types;
			}

			fingerprint session::get_host_fingerprint()
			{
				return fingerprint(this->_session);
			}

			void session::login(std::string username, std::string password) throw(axon::exception)
			{
				int _err;

				if((_err = libssh2_userauth_password(this->_session, username.c_str(), password.c_str())) != 0)
				{
					std::string _errstr;

					if (_err == LIBSSH2_ERROR_ALLOC)
						_errstr = "An internal memory allocation call failed";
					else  if (_err == LIBSSH2_ERROR_SOCKET_SEND)
						_errstr = "Unable to send data on socket";
					else  if (_err == LIBSSH2_ERROR_PASSWORD_EXPIRED)
						_errstr = "The password for the user has expired";
					else  if (_err == LIBSSH2_ERROR_AUTHENTICATION_FAILED)
						_errstr = "Invalid username/password or public/private key";
					else
						_errstr = "Generic failure";

					throw axon::exception(__FILENAME__, __LINE__, __func__, "libssh2_userauth_password() - " + _errstr);
				}
			}

			channel* session::open_channel()
			{
				LIBSSH2_CHANNEL* _channel;
				// TODO Check the return value for the specific error.
				if (!(_channel = libssh2_channel_open_session(this->_session)))
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Could not open channel");
				}
				channel* c = new channel(_channel);
				return c;
			}

			bool sftp::connect()
			{
				open(_hostname, 22);
				login(_username, _password);
				init();

				return true;
			}

			bool sftp::disconnect()
			{
				if (_sftp)
					libssh2_sftp_shutdown(_sftp);

				return true;
			}

			bool sftp::init()
			{

				if (!(_sftp = libssh2_sftp_init(_session)))
				{
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Cannot initialize SFTP Session");
				}

				return true;
			}

			bool sftp::chwd(std::string path)
			{
				// TODO
				// Here some checking need to be done to make sure the folder exists
				// and return a appropriate result.
				// hint: 
				_path = path;

				return true;
			}

			std::string sftp::pwd()
			{
				return _path;
			}

			int sftp::list(callback cbfn)
			{
				LIBSSH2_SFTP_HANDLE *hsftp;
				
				if (!(hsftp = libssh2_sftp_opendir(_sftp, _path.c_str() )))
				{
					int i = libssh2_session_last_errno(_session);

					if (i == LIBSSH2_ERROR_ALLOC)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
					}
				}

				do
				{
					char mem[511];
					char longentry[512];
					LIBSSH2_SFTP_ATTRIBUTES attrs;

					if (libssh2_sftp_readdir_ex(hsftp, mem, sizeof(mem), longentry, sizeof(longentry), &attrs))
					{
						if (!LIBSSH2_SFTP_S_ISDIR(attrs.permissions) && mem[0] != '.')
						{
							if (_filter.size())
							{
								if (regex_match(mem, _filter[0]))
								{
									struct entry file;

									file.name = mem;
									file.size = attrs.filesize;

									cbfn(&file);
								}
							}
							else
							{
								struct entry file;

								file.name = mem;
								file.size = attrs.filesize;

								cbfn(&file);
							}
						}
					}
					else
						break;

				} while (true);

				return 0; // this should return the count of files?
			}

			int sftp::list(std::vector<entry> *vec)
			{
				LIBSSH2_SFTP_HANDLE *hsftp;
				
				if (!(hsftp = libssh2_sftp_opendir(_sftp, _path.c_str())))
				{
					int i = libssh2_session_last_errno(_session);

					if (i == LIBSSH2_ERROR_ALLOC)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
					}
				}

				do
				{
					char mem[511];
					char longentry[512];
					LIBSSH2_SFTP_ATTRIBUTES attrs;

					if (libssh2_sftp_readdir_ex(hsftp, mem, sizeof(mem), longentry, sizeof(longentry), &attrs))
					{
						if (_filter.size() > 0)
						{
							if (regex_match(mem, _filter[0]))
							{
								struct entry file;
							
								file.name = mem;
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

								vec->push_back(file);						
							}
						}
						else
						{
							struct entry file;
						
							file.name = mem;
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
								
							vec->push_back(file);
						}
					}
					else
						break;

				} while (true);

				return true;
			}

			bool sftp::ren(std::string src, std::string dest)
			{
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
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
					}
				}

				return true;
			}

			bool sftp::del(std::string src)
			{
				int i;
				std::string srcx;

				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;

				if ((i = libssh2_sftp_unlink(_sftp, srcx.c_str())) != 0)
				{
					if (i == LIBSSH2_ERROR_ALLOC)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
					}
				}

				return true;
			}

			long long sftp::get(std::string src, std::string dest, bool compress = false)
			{
				LIBSSH2_SFTP_HANDLE *hsftp;
				char FILEBUF[MAXBUF];
				int bzerr;
				unsigned long long filesize = 0;
				unsigned int inbyte, outbyte;

				std::string srcx, destx, temp;
				
				FILE *fp;
				BZFILE *bfp = NULL;

				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;

				if (!(hsftp = libssh2_sftp_open(_sftp, srcx.c_str(), LIBSSH2_FXF_READ, 0)))
				{
					int i = libssh2_session_last_errno(_session);

					if (i == LIBSSH2_ERROR_ALLOC)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
					}
				}

				if (!(fp = fopen(dest.c_str(), "wb")))
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Error opening file for writing");

				if (compress)
				{
					bfp = BZ2_bzWriteOpen(&bzerr, fp, 3, 0, 30);

					if (bzerr != BZ_OK)
					{
						BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
						fclose(fp);
						unlink(dest.c_str());

						throw axon::exception(__FILENAME__, __LINE__, __func__, "Could not open compression stream");
					}
				}

				do {

					int rc = libssh2_sftp_read(hsftp, FILEBUF, MAXBUF);

					if (rc > 0)
					{
						if (compress)
						{
							BZ2_bzWrite(&bzerr, bfp, FILEBUF, rc);
							if (bzerr == BZ_IO_ERROR)
							{
								BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
								fclose(fp);
								unlink(dest.c_str());

								throw axon::exception(__FILENAME__, __LINE__, __func__, "Error in comppression stream");
							}
						}
						else
							fwrite(FILEBUF, rc, 1, fp);
						filesize += rc;
					}
					else
						break;

				} while (true);

				if (compress)
					BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
				fclose(fp);

				return filesize;
			}

			long long sftp::put(std::string src, std::string dest)
			{
				LIBSSH2_SFTP_HANDLE *hsftp;
				std::string destx, temp;
				char *BUFPTR, FILEBUF[MAXBUF];
				int rc;
				unsigned long long filesize = 0;
				size_t sb;
				FILE *fp;

				if (dest[0] == '/')
				{
					temp = dest + ".tmp";
					destx = dest;
				}
				else
				{
					temp = _path + "/" + dest + ".tmp";
					destx = _path + "/" + dest;
				}

				if (!(fp = fopen(src.c_str(), "rb")))
					throw axon::exception(__FILENAME__, __LINE__, __func__, "Cannot open source file '" + src + "'");

				if (!(hsftp = libssh2_sftp_open(_sftp, temp.c_str(), LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH)))
				{
					int i = libssh2_session_last_errno(_session);

					if (i == LIBSSH2_ERROR_ALLOC)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "An internal memory allocation call failed");
					else if (i == LIBSSH2_ERROR_SOCKET_SEND)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Unable to send data on socket");
					else if (i == LIBSSH2_ERROR_SOCKET_TIMEOUT)
						throw axon::exception(__FILENAME__, __LINE__, __func__, "Timeout while waiting on socket");
					else if (i == LIBSSH2_ERROR_SFTP_PROTOCOL)
					{
						fclose(fp);
						unsigned long sftperr = libssh2_sftp_last_error(_sftp);
						throw axon::exception(__FILENAME__, __LINE__, __func__, get_sftp_error_desc(sftperr));
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

				ren(temp, destx);

				return filesize;
			}

			int cb(const struct entry *e)
			{
				return 0;
			}
		}
	}
}