#ifndef AXON_NOTHING_H_
#define AXON_NOTHING_H_

#define AXON_TRANSFER_NOTHING_PROXY    'A'
#define AXON_TRANSFER_NOTHING_ENDPOINT 'B'

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			class nothing : public connection {

			public:
				nothing(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) { };;
				nothing(const nothing& rhs) : connection(rhs) {  };
				~nothing();

				bool set(char, std::string);

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				bool mkdir(std::string);
				int list(const axon::transport::transfer::cb &);
				int list(std::vector<entry> &);
				long long copy(std::string, std::string, bool = false);
				bool ren(std::string, std::string);
				bool del(std::string);

				int cb(const struct entry *);

				long long get(std::string, std::string, bool = false);
				long long put(std::string, std::string, bool = false);
			};
		}
	}
}

#endif