#ifndef AXON_SOCKET_H_
#define AXON_SOCKET_H_

#include <queue>
#include <atomic>
#include <thread>
#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

namespace axon
{
	namespace transport
	{
		namespace tcpip {

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
				long write(std::string);
				long write(const char *, size_t);
			};
		}
	}
}
#endif
