#ifndef AXON_HDFS_H_
#define AXON_HDFS_H_

#define AXON_TRANSFER_HDFS_AUTH         'a'
#define AXON_TRANSFER_HDFS_DOMAIN       'b'
#define AXON_TRANSFER_HDFS_CACHE        'c'

#define AXON_TRANSFER_HDFS_PORT         0x01

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			class hdfs : public connection {

				std::string _domain, _cache;

				struct hdfsBuilder *_builder = NULL;
				hdfsFS _filesystem = NULL;

				static std::atomic<int> _instance;
				static std::mutex _mtx;

				bool init();

			public:
				hdfs(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) { };
				hdfs(const hdfs& rhs) : connection(rhs) {  };
				~hdfs();

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

				bool set(char, std::string);
				bool set(char, int);
			};
		}
	}
}

#endif