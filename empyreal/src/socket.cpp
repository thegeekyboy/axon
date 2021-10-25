#include <main.h>

namespace tcn
{
	namespace transport
	{
		namespace tcpip
		{
			bool nonblocking(int fd, bool blocking)
			{
				if (fd < 0) return false;

				int flags = fcntl(fd, F_GETFL, 0);

				if (flags < 0)
					return false;
				
				flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
				
				return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
			}

			socks::socks()
			{
				_fd = INVALID_SOCKET;
				_port = 0;
				_alive = false;
				_reader = false;
			}

			socks::~socks()
			{
			}

			bool socks::init()
			{
				int on = 1, flags;
				struct linger lng = { 0, 0 };
				
				if ((_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error initializing socket()");

				if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) == -1)
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error setting options with setsockopt()");

				if (setsockopt(_fd, SOL_SOCKET, SO_LINGER, (void *) &lng, sizeof(lng)) == -1)
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error setting linger with setsockopt()");

				return true;
			}

			bool socks::alive()
			{
				return _alive;
			}

			bool socks::stop()
			{
				_alive = false;

				while (_reader)
					usleep(10000);

				shutdown(_fd, SHUT_RDWR);

				if (_fd != INVALID_SOCKET)
					close(_fd);
			}

			bool socks::open(std::string host, unsigned short port)
			{
				struct sockaddr_in sin;
				struct hostent *phe;

				_host = host;
				_port = port;

				memset(&sin, 0, sizeof(sin));
				sin.sin_family = AF_INET;
				sin.sin_port = htons(_port);

				if (inet_aton(_host.c_str(), &sin.sin_addr) == 0)
				{
					if ((phe = gethostbyname(_host.c_str())) == NULL)
						throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Cannot resolve hostname");

					memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
				}

				if (connect(_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Socket connection connect() error");

				if (!nonblocking(_fd, true))
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Could not set non-blocking mode");

				_alive = true;

				return true;
			}

			long long socks::read(void *buf, size_t len)
			{
				long long nbytes = 0;
				struct timeval tv;
				fd_set orig, rfds;

				tv.tv_sec = 0;
				tv.tv_usec = 5000;

				FD_ZERO(&orig);
				FD_ZERO(&rfds);
				FD_SET(_fd, &orig);

				if (_alive)
				{
					if ((nbytes = recv(_fd, buf, len, 0)) <= 0)
					{
						int errid = errno;

						if (nbytes == 0)
						{
							nbytes = -1;
						}
						else if (nbytes < 0)
						{
							if ((errid == EAGAIN) || (errid == EWOULDBLOCK))
								nbytes = 0;
							else
								nbytes = -2;
						}
					}
				}

				return nbytes;
			}

			int socks::readline()
			{
				char buf[2];
				size_t nbytes;
				std::string line;

				struct timeval tv;
				fd_set orig, rfds;

				tv.tv_sec = 0;
				tv.tv_usec = 5000;

				FD_ZERO(&orig);
				FD_ZERO(&rfds);
				FD_SET(_fd, &orig);

				_reader = true;

				while (_alive)
				{
					rfds = orig;

					int rc = select(_fd+1, &rfds, NULL, NULL, &tv);

					if ( rc == -1)
					{
						break;
					}
					else if (rc > 0)
					{
						if ((nbytes = recv(_fd, buf, 1, 0)) <= 0)
						{
							if (nbytes == 0) {
								break;
							} else {
								perror("recv");
							}
						}
						else
						{
							buf[1] = 0;

							if (buf[0] == '\r' || buf[0] == '\n')
							{
								_buffer.push(line);
								line.clear();
							}
							else
								line += buf[0];
						}
					}
				}

				_alive = false;
				_reader = false;

				return 0;
			}

			bool socks::purge()
			{
				_buffer.empty();

				return true;
			}

			bool socks::linewaiting()
			{
				if (_buffer.size() > 0)
					return true;
				else
					return false;
			}

			std::string socks::line()
			{
				std::string nextline;

				if (_buffer.size() > 0)
				{
					nextline = _buffer.front();
					_buffer.pop();
				}

				return nextline;
			}

			bool socks::writeline(std::string data)
			{
				int retval;

				data += "\r\n";
				
				//std::cout<<"SENT:("<<data.size()<<")>"<<data<<std::endl;

				if (write(data.c_str(), data.size()) <= 0)
					throw cException(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error writing()-ing to socket");

				return true;
			}

			long long socks::write(const char *buf, size_t len)
			{
				size_t sent = 0, remain = len;

				do
				{
					sent = send(_fd, buf, remain, 0);
					remain -= sent;
				} while (remain > 0 && sent > 0);

				if (sent <= 0)
					return -1;

				return len;
			}
		}
	}
}