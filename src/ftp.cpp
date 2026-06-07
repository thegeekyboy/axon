#include <chrono>

#include <axon.h>
#include <axon/socket.h>
#include <axon/connection.h>
#include <axon/ftp.h>
#include <axon/ftplist.h>
#include <axon/util.h>

static constexpr unsigned int FTP_TIMEOUT_MS = 5000;

namespace axon
{
	namespace transfer
	{
		bool ftp::push(axon::transfer::connection&) { return false; };

		ftp::ftp(std::string hostname, std::string username, std::string password, uint16_t port): connection(hostname, username, password, port) {};
		ftp::ftp(std::string hostname, std::string username, std::string password): connection(hostname, username, password) {};
		ftp::ftp(const ftp& rhs) : connection(rhs) {};

		ftp::~ftp()
		{
			disconnect();

			while (_sock.alive())
				usleep(500000);
		}

		bool ftp::init()
		{
			_sock.init();
			return true;
		}

		// ----------------------------------------------------------------
		// _wait_for() — internal helper
		//
		// Sends `cmd` (if non-empty), then blocks until a line arrives
		// whose numeric code matches one of the codes in `expected`.
		// Throws on unexpected codes or timeout.  Returns the full
		// response line.
		// ----------------------------------------------------------------
		std::string ftp::_wait_for(const std::string &cmd, std::initializer_list<std::string> expected, std::initializer_list<std::string> errors)
		{
			if (!cmd.empty())
				_sock.writeline(cmd);

			while (true)
			{
				if (!_sock.wait(FTP_TIMEOUT_MS))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,
						"[" + _id + "] Timeout waiting for FTP response" +
						(cmd.empty() ? "" : " after: " + cmd));

				std::string resp = _sock.line();
				if (resp.size() <= 3) continue;

				std::string code = axon::util::split(resp, ' ')[0];

				for (auto &e : expected)
					if (code == e) return resp;

				for (auto &e : errors)
					if (code == e)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] FTP error response: " + resp);

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected FTP response: " + resp);
			}
		}

		bool ftp::connect()
		{
			init();
			_sock.open(_hostname, 21);

			_th = std::thread(&axon::transport::tcpip::socks::readline, &_sock);
			_th.detach();

			_wait_for("", {"220"}, {});
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
			_wait_for("USER " + _username, {"331"}, {});
			_wait_for("PASS " + _password, {"230"}, {"530"});

			// Drain any extra welcome lines the server may send.
			_sock.line();
			_sock.line();

			_wait_for("TYPE I", {"200"}, {"500"});

			return true;
		}

		bool ftp::chwd(std::string path)
		{
			_wait_for("CWD " + path, {"250"}, {"550"});
			pwd();

			return true;
		}

		std::string ftp::pwd()
		{
			std::string resp = _wait_for("PWD", {"257"}, {});

			std::vector<std::string> tokens = axon::util::split(resp, ' ');
			if (tokens.size() >= 2 && tokens[1].size() > 2)
			{
				std::string temp;
				static const boost::regex ex1("^\"(.*)\"$");
				static const boost::regex ex2("\"{2}");

				for (unsigned int i = 1; i <= tokens.size() - 1; i++)
				{
					if (i > 1) temp += ' ';
					temp += tokens[i];
				}

				temp  = boost::regex_replace(temp, ex1, "$1");
				_path = boost::regex_replace(temp, ex2, "\"");
			}

			return _path;
		}

		bool ftp::mkdir([[maybe_unused]] std::string dir)
		{
			return true;
		}

		off_t ftp::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			std::string srcx = (src[0] == '/') ? src : _path + "/" + src;
			auto [path, filename] = axon::util::splitpath(srcx);

			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,
					"[" + _id + "] source and destination cannot be the same");

			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,
				"[" + _id + "] server-side copy operation currently not supported");

			return 0L;
		}

		off_t ftp::copy(std::string src, std::string dest) { return copy(src, dest, false); };

		bool ftp::ren(std::string from, std::string to)
		{
			_wait_for("RNFR " + from, {"350"}, {"550"});
			_wait_for("RNTO " + to,   {"250"}, {"550"});

			return true;
		}

		bool ftp::del(std::string thefile)
		{
			_wait_for("DELE " + thefile, {"250"}, {"550"});

			return true;
		}

		size_t ftp::list(std::vector<axon::entry> &vec)
		{
			return list([&](const axon::entry &e) mutable { vec.push_back(e); });
		}

		size_t ftp::list(const axon::transfer::cb &cbfn)
		{
			char pasvhost[16];
			unsigned char v[6];
			unsigned int pasvport = 0;

			axon::transport::tcpip::socks tsock;

			// PASV — get data channel address
			std::string resp = _wait_for("PASV", {"227"}, {});
			std::vector<std::string> tokens = axon::util::split(resp, ' ');

			sscanf(tokens[4].c_str() + 1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
			sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
			pasvport = v[4] * 256 + v[5];

			_sock.writeline("LIST");

			tsock.init();
			tsock.open(pasvhost, pasvport);
			tsock.readline();

			size_t count = 0;

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

					if (ftpl.flagtrycwd == 0 && match(ftpl.name))
					{
						e.name  = axon::util::trim(ftpl.name);
						e.size  = ftpl.size;
						e.proto = axon::protocol::FTP;
						cbfn(e);
						count++;
					}
				}
			}

			_wait_for("", {"226"}, {});

			return count;
		}

		off_t ftp::get(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			char pasvhost[16];
			unsigned char v[6];
			unsigned int pasvport = 0;

			FILE *fp;
			axon::transport::tcpip::socks tsock;

			// PASV
			std::string resp = _wait_for("PASV", {"227"}, {});
			std::vector<std::string> tokens = axon::util::split(resp, ' ');

			sscanf(tokens[4].c_str() + 1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
			sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
			pasvport = v[4] * 256 + v[5];

			if (!(fp = fopen(dest.c_str(), "wb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,
					"[" + _id + "] Error opening file (" + dest + ") for writing");

			_sock.writeline("RETR " + src);

			tsock.init();
			tsock.open(pasvhost, pasvport);

			char buf[axon::MAX_BUFFER_SIZE];
			long long rc, szx = 0;

			while ((rc = tsock.read(buf, axon::MAX_BUFFER_SIZE - 1)) >= 0)
			{
				fwrite(buf, rc, 1, fp);
				szx += rc;
			}

			fclose(fp);

			_wait_for("", {"226"}, {});

			return szx;
		}

		off_t ftp::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			unsigned char v[6];
			char pasvhost[18];
			long pasvport = 0;
			FILE *fp;

			axon::transport::tcpip::socks tsock;
			std::string temp = src + ".tmp";

			// PASV
			std::string resp = _wait_for("PASV", {"227"}, {});
			std::vector<std::string> tokens = axon::util::split(resp, ' ');

			sscanf(tokens[4].c_str() + 1, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
			bzero(pasvhost, 18);
			sprintf(pasvhost, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
			pasvport = v[4] * 256 + v[5];

			if (!(fp = fopen(src.c_str(), "rb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot open source file '" + src + "'");

			_sock.writeline("STOR " + temp);

			tsock.init();
			tsock.open(pasvhost, pasvport);

			int rc, sb;
			char flb[axon::MAX_BUFFER_SIZE];

			do {
				if ((sb = fread(flb, 1, axon::MAX_BUFFER_SIZE - 1, fp)) <= 0)
					break;

				rc = tsock.write((const char *) flb, sb);
			} while (rc > 0 && sb > 0);

			tsock.stop();
			fclose(fp);

			_wait_for("", {"226"}, {});

			ren(temp, dest);

			return 0;
		}

		bool ftp::open(std::string, std::ios_base::openmode) { return false; }
		bool ftp::close() { return false; }
		ssize_t ftp::read(char*, size_t) { return -1; }
		ssize_t ftp::write(const char*, size_t) { return -1; }
	}
}