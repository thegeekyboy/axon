#include <atomic>
#include <mutex>
#include <thread>

#include <hdfs.h>

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
				disconnect();

				if (_builder)
				// 	hdfsFreeBuilder(_builder);
				free(_builder);
			}

			bool hdfs::init()
			{
				_builder = hdfsNewBuilder();

				return true;
			}

			bool hdfs::connect()
			{
				init();

				hdfsBuilderSetForceNewInstance(_builder);

				hdfsBuilderSetNameNode(_builder, _hostname.c_str());
				hdfsBuilderSetNameNodePort(_builder, _port);
				
				hdfsBuilderSetKerbTicketCachePath(_builder, _cache.c_str());
				hdfsBuilderSetUserName(_builder, _username.c_str()); // I believe this is optional for kerberos authentication

				_filesystem = hdfsBuilderConnect(_builder);

				return true;
			}

			bool hdfs::disconnect()
			{
				//if (_filesystem)
					hdfsDisconnect(_filesystem);
				
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
					case AXON_TRANSFER_HDFS_CACHE:
						_cache = value;
						return true;
				}
				return false;
			}

			bool hdfs::set(char key, int value)
			{
				switch (key)
				{
					case AXON_TRANSFER_HDFS_PORT:
						_port = value;
						return true;
				}
				return false;
			}
		}
	}
}