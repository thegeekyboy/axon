#ifndef AXON_HDFS_H_
#define AXON_HDFS_H_

#define AXON_TRANSFER_HDFS_DOMAIN		'a'
#define AXON_TRANSFER_HDFS_URI			'b'
#define AXON_TRANSFER_HDFS_ENCRYPT		'c'
#define AXON_TRANSFER_HDFS_LOGLEVEL		'd'
#define AXON_TRANSFER_HDFS_PROTECT		'e'
#define AXON_TRANSFER_HDFS_AUTHTYPE		'f'
#define AXON_TRANSFER_HDFS_AUTHORIZE	'g'
#define AXON_TRANSFER_HDFS_AUTHALGO		'h'
#define AXON_TRANSFER_HDFS_CACHEPATH	'i'

#define AXON_TRANSFER_HDFS_PORT         0x01

#include <hdfs/hdfs.h>

namespace axon
{
	namespace transfer
	{
		class hdfs : public connection {

			std::string _domain, _cache;

			struct hdfsBuilder *_builder;
			hdfsFS _filesystem;
			hdfsFile _fp;

			static std::atomic<int> _instance;

			bool push(axon::transfer::connection&);

		public:
			hdfs(std::string, std::string, std::string, uint16_t);
			hdfs(const hdfs& rhs) : connection(rhs) {  };
			~hdfs();

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			int list(const axon::transfer::cb &);
			int list(std::vector<entry> &);
			long long copy(std::string, std::string, bool);
			long long copy(std::string src, std::string dest) { return copy(src, dest, false); };
			bool ren(std::string, std::string);
			bool del(std::string);

			int cb(const struct entry *);

			long long get(std::string, std::string, bool);
			long long put(std::string, std::string, bool);

			bool set(char, std::string);
			bool set(char, int);

			bool open(std::string, std::ios_base::openmode);
			bool close();

			ssize_t read(char*, size_t);
			ssize_t write(const char*, size_t);
		};
	}
}

#endif
