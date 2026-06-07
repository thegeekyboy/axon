#ifndef AXON_SOCKET_H_
#define AXON_SOCKET_H_

#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
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

				std::mutex _mtx;
				std::condition_variable _cv;

			public:
				socks();
				~socks();

				bool init();
				bool open(std::string, unsigned short);
				bool stop();

				bool alive();

				long long read(void *, size_t);
				int  readline();		// runs on its own thread; now notifies _cv
				bool linewaiting();
				std::string line();
				bool purge();

				bool writeline(std::string);
				long write(std::string);
				long write(const char *, size_t);

				bool wait(unsigned int timeout_ms = 0);
			};
		}
	}
}
#endif