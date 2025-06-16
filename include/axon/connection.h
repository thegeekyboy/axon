#ifndef AXON_CONNECTION_H_
#define AXON_CONNECTION_H_

#include <boost/regex.hpp>

#define AXON_TRANSFER_CONNECTION_PORT 0x01

namespace axon
{
	namespace transfer {

		// typedef int (*callback) (const struct entry *);
		typedef std::function<void(axon::entry &)> cb;

		class connection {

			protected:
				std::string _id;
				std::string _hostname, _username, _password;
				std::string _path;
				uint16_t _port;
				axon::proto_t _proto;

				bool _connected, _fileopen;
				std::ios_base::openmode _om;
				std::vector<boost::regex> _filter;

				virtual bool push(axon::transfer::connection&) = 0;

			public:

				connection(std::string, std::string, std::string, uint16_t);
				connection(std::string, std::string, std::string);
				connection(const connection&);
				~connection();

				virtual bool set(char, std::string) = 0;

				virtual bool connect() = 0;
				virtual bool disconnect() = 0;

				virtual bool filter(std::string) final;
				virtual bool match(std::string) final;

				virtual bool chwd(std::string) = 0;
				virtual std::string pwd() = 0;
				virtual bool mkdir(std::string) = 0;
				virtual int list(const cb &) = 0;
				virtual int list(std::vector<axon::entry> &) = 0;
				virtual long long copy(std::string, std::string, bool) = 0;
				virtual long long copy(std::string, std::string) = 0;
				virtual bool ren(std::string, std::string) = 0;
				virtual bool del(std::string) = 0;

				virtual long long get(std::string, std::string, bool) = 0;
				virtual long long put(std::string, std::string, bool) = 0;
				virtual long long put(std::string src, std::string dest) final { return put(src, dest, false); };
				virtual long long put(std::string src) final { return put(src, src, false); };

				virtual bool open(std::string, std::ios_base::openmode) = 0;
				virtual bool close() = 0;

				virtual ssize_t read(char*, size_t) = 0;
				virtual ssize_t write(const char*, size_t) = 0;

				friend axon::transfer::connection& operator>>(axon::transfer::connection& src, axon::transfer::connection& dest) {
					src.push(dest);
					return src;
				}
				friend axon::transfer::connection& operator<<(axon::transfer::connection& dest, axon::transfer::connection& src) {
					src.push(dest);
					return dest;
				}
		};
	}
}

#endif
