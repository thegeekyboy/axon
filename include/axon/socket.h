#ifndef AXON_SOCKET_H_
#define AXON_SOCKET_H_

#include <queue>
#include <atomic>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define INVALID_SOCKET (SOCKET)(~0)

namespace axon
{
	namespace transport
	{
		namespace tcpip {


			typedef int SOCKET;

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