#ifndef AXON_FILE_H_
#define AXON_FILE_H_

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			class file : public connection {

				bool init();
				bool login();

			public:
				file(std::string hostname, std::string username, std::string password) : connection(hostname, username, password) {  };
				~file();

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				int list(callback);
				int list(std::vector<entry> *);
				bool ren(std::string, std::string);
				bool del(std::string);

				int cb(const struct entry *);

				long long get(std::string, std::string, bool);
				long long put(std::string, std::string);
			};
		}
	}
}

#endif