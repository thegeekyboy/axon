#ifndef SOCKET_H_
#define SOCKET_H_

namespace tcn
{
	namespace transport
	{
		namespace tcpip {

			#define INVALID_SOCKET (SOCKET)(~0)
			typedef unsigned int SOCKET;

			class socks {

				int _fd;
				std::queue<std::string> _buffer;

				std::string _host;
				unsigned short _port;
				std::atomic<bool> _alive;
				std::atomic<bool> _reader;

			public:
				socks();
				~socks();

				bool init();
				bool open(std::string, unsigned short);
				bool stop();

				bool alive();

				long long read(void *, size_t);
				int readline();
				bool linewaiting();
				std::string line();
				bool purge();
				
				bool writeline(std::string);
				long long write(const char *, size_t);
			};
		}
	}
}
#endif