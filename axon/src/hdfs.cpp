#include <atomic>
#include <mutex>

#include <axon.h>
#include <axon/connection.h>
#include <axon/hdfs.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			hdfs::~hdfs()
			{

			}

			bool hdfs::connect()
			{
				return true;
			}

			bool hdfs::disconnect()
			{
				return true;
			}

			bool hdfs::chwd(std::string path)
			{
				return true;
			}

			std::string hdfs::pwd()
			{
				return _path;
			}

			bool hdfs::mkdir(std::string path)
			{
				return true;
			}

			int hdfs::list(const axon::transport::transfer::cb &cbfn)
			{
				return true;
			}

			int hdfs::list(std::vector<entry> &vec)
			{
				return list([&vec](const axon::entry &e) mutable { vec.push_back(e); });
			}

			long long hdfs::copy(std::string& src, std::string& dest, bool compress)
			{
				return 0L;
			}

			bool hdfs::ren(std::string src, std::string dest)
			{
				return true;
			}

			bool hdfs::del(std::string target)
			{
				return true;
			}

			int hdfs::cb(const struct entry *e)
			{
				return 0;
			}

			long long hdfs::get(std::string src, std::string dest, bool compress)
			{
				return 0L;
			}

			long long hdfs::put(std::string src, std::string dest, bool compress)
			{
				return 0L;
			}

			bool hdfs::set(char key, std::string value)
			{
				switch (key)
				{
					case AXON_TRANSFER_HDFS_DOMAIN:
						_domain = value;
						return true;
				}
				return false;
			}
		}
	}
}