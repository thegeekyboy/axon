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

			bool _connected;
			std::vector<boost::regex> _filter;

		public:

			connection(std::string, std::string, std::string, uint16_t);
			connection(const connection&);
			~connection();

			virtual bool connect() = 0;
			virtual bool disconnect() = 0;

			virtual bool filter(std::string);
			virtual bool match(std::string);

			virtual bool chwd(std::string) = 0;
			virtual std::string pwd() = 0;
			virtual bool mkdir(std::string) = 0;
			virtual int list(const cb &) = 0;
			virtual int list(std::vector<axon::entry> &) = 0;
			virtual long long copy(std::string, std::string, bool = false) = 0;
			virtual bool ren(std::string, std::string) = 0;
			virtual bool del(std::string) = 0;

			virtual long long get(std::string, std::string, bool = false) = 0;
			virtual long long put(std::string, std::string, bool = false) = 0;
		};
	}
}

#endif
