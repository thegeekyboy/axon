#ifndef CONNECTION_H_
#define CONNECTION_H_

namespace tcn
{
	namespace transport {

		namespace transfer {

			typedef int (*callback) (const struct entry *);

			class connection {

			protected:
				std::string _hostname, _username, _password, _path;

				bool _connected;
				std::vector<boost::regex> _filter;

			public:

				connection(std::string, std::string, std::string);
				~connection();

				virtual bool connect() = 0;
				virtual bool disconnect() = 0;

				virtual bool filter(std::string);

				virtual bool chwd(std::string) = 0;
				virtual std::string pwd() = 0;
				virtual int list(callback) = 0;
				virtual int list(std::vector<tcn::entry> *) = 0;
				virtual bool ren(std::string, std::string) = 0;
				virtual bool del(std::string) = 0;

				virtual long long get(std::string, std::string, bool) = 0;
				virtual long long put(std::string, std::string) = 0;
			};
		}
	}
}

#endif