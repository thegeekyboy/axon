#include <chrono>

#include <axon.h>

#include <axon/socket.h>
#include <axon/connection.h>
#include <axon/ftp.h>
#include <axon/ftplist.h>
#include <axon/util.h>

namespace axon
{
	namespace transfer
	{
		ftp::~ftp()
		{
			disconnect();

			while (_sock.alive())
				usleep(500000);

			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		bool ftp::init()
		{
			_sock.init();

			return true;
		}

		bool ftp::connect()
		{
			init();
			_sock.open(_hostname, 21);

			_th = std::thread(&axon::transport::tcpip::socks::readline, &_sock);
			_th.detach();

			usleep(100000);

			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "220")
							break;
					}
				}
				usleep(10000);
			}

			login();

			return true;
		}

		bool ftp::disconnect()
		{
			if (_sock.alive())
				_sock.writeline("QUIT");

			_sock.stop();

			return true;
		}

		bool ftp::login()
		{
			_sock.writeline("USER " + _username);
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "331")
							break;
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			_sock.writeline("PASS " + _password);

			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');

						if (tokens[0] == "230")
							break;
						else if (tokens[0] == "530")
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Login incorrect");
					}
				}
				usleep(10000);
			}

			_sock.line();
			_sock.line();

			_sock.writeline("TYPE I");
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');

						if (tokens[0] == "200")
							return true;
						else if (tokens[0] == "500")
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot change transfer type to binary");
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response? - " + resp);
					}
				}
				usleep(10000);
			}

			return false;
		}

		bool ftp::chwd(std::string path)
		{
			_sock.writeline("CWD " + path);
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "250")
						{
							pwd();
							return true;
						}
						else if (tokens[0] == "550")
						{
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not change directory to " + path);
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			return false;
		}

		std::string ftp::pwd()
		{
			_sock.writeline("PWD");
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');

						if (tokens[0] == "257")
						{
							if (tokens[1].size() > 2)
							{
								std::string temp;
								boost::regex ex1("^\"(.*)\"$");
								boost::regex ex2("\"{2}");

								for (unsigned int i = 1; i <= tokens.size() - 1; i++)
								{
									if (i > 1)
										temp += ' ';

									temp += tokens[i];
								}

								temp = boost::regex_replace(temp, ex1, "$1");
								_path = boost::regex_replace(temp, ex2, "\"");

								return _path;
							}
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			return _path;
		}

		bool ftp::mkdir([[maybe_unused]] std::string dir)
		{
			return true;
		}

		long long ftp::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: implement remote system copy function
			// TODO: implement compression on remote
			std::string srcx;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			auto [path, filename] = axon::util::splitpath(srcx);

			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] server-side copy operation currently not supported");

			return 0L;
		}

		bool ftp::ren(std::string from, std::string to)
		{
			_sock.writeline("RNFR " + from);
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "350")
						{
							break;
						}
						else if (tokens[0] == "550")
						{
							return false;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			_sock.writeline("RNTO " + to);
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "250")
						{
							return true;
						}
						else if (tokens[0] == "550")
						{
							return false;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			return false;
		}

		bool ftp::del(std::string thefile)
		{
			_sock.writeline("DELE " + thefile);
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "250")
						{
							return true;
						}
						else if (tokens[0] == "550")
						{
							return false;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			return false;
		}

		int ftp::list(std::vector<axon::entry> &vec)
		{
			return list([&](const axon::entry &e) mutable { vec.push_back(e); });
		}

		int ftp::list(const axon::transfer::cb &cbfn)
		{
			char pasvhost[16];
			unsigned char v[6];
			unsigned int pasvport = 0;

			axon::transport::tcpip::socks tsock;

			_sock.writeline("PASV");
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "227")
						{
							sscanf(tokens[4].c_str()+1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
							sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
							pasvport = v[4]*256 + v[5];

							break;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			_sock.writeline("LIST");

			tsock.init();
			tsock.open(pasvhost, pasvport);
			tsock.readline();

			while (tsock.linewaiting())
			{
				axon::entry e;
				struct ftpparse ftpl;
				std::string retline = tsock.line();

				if (retline.size() > 20)
				{
					char linebuf[512];

					bzero(linebuf, 512);
					strcpy(linebuf, retline.c_str());
					ftpparse(&ftpl, linebuf, retline.size());

					if (ftpl.flagtrycwd == 0)
					{
						if (match(ftpl.name))
						{
								struct entry file;

								e.name = axon::util::trim(ftpl.name);
								e.size = ftpl.size;
								e.et = axon::protocol::FTP;

								cbfn(e);
						}
					}
				}
			}

			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "226")
							break;
					}
				}
				usleep(10000);
			}

			return true;
		}

		long long ftp::get(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compression
			char pasvhost[16];
			unsigned char v[6];
			unsigned int pasvport = 0;

			FILE *fp;
			//BZFILE *bfp;
			axon::transport::tcpip::socks tsock;

			_sock.writeline("PASV");
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "227")
						{
							sscanf(tokens[4].c_str()+1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
							sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
							pasvport = v[4] * 256 + v[5];

							break;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			if (!(fp = fopen(dest.c_str(), "wb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error opening file (" + dest +") for writing");

			_sock.writeline("RETR " + src);

			tsock.init();
			tsock.open(pasvhost, pasvport);

			char buf[MAXBUF];
			long long rc;
			long long szx = 0;

			while ((rc = tsock.read(buf, MAXBUF)) >= 0)
			{
				fwrite(buf, rc, 1, fp);
				szx += rc;
			}

			fclose(fp);

			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "226")
							break;
					}
				}
				usleep(10000);
			}

			return szx;
		}

		long long ftp::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: need to implement compression
			unsigned char v[6];
			char pasvhost[18];
			long pasvport = 0;
			FILE *fp;

			axon::transport::tcpip::socks tsock;
			std::string temp = src + ".tmp";

			_sock.writeline("PASV");
			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "227")
						{
							char tok[64];

							strcpy(tok, tokens[4].c_str());

							sscanf(tokens[4].c_str()+1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
							bzero(pasvhost, 18);
							sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
							pasvport = v[4] * 256 + v[5];

							break;
						}
						else
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
					}
				}
				usleep(10000);
			}

			if (!(fp = fopen(src.c_str(), "rb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot open source file '" + src + "'");

			_sock.writeline("STOR " + temp);

			tsock.init();
			tsock.open(pasvhost, pasvport);

			int rc, sb;
			char flb[MAXBUF+1];

			do {

				if ((sb = fread(flb, 1, MAXBUF, fp)) <= 0)
					break;

				rc = tsock.write((const char *) flb, sb);
			} while (rc > 0 && sb > 0);

			tsock.stop();

			fclose(fp);

			while (_sock.alive())
			{
				if (_sock.linewaiting())
				{
					std::string resp = _sock.line();

					if (resp.size() > 3)
					{
						std::vector<std::string> tokens = axon::util::split(resp, ' ');
						if (tokens[0] == "226")
							break;
					}
				}
				usleep(10000);
			}

			ren(temp, dest);

			return 0;
		}
	}
}
