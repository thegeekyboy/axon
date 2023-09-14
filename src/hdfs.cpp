#include <atomic>
#include <mutex>
#include <thread>

#include <bzlib.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/hdfs.h>
#include <axon/util.h>

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
					hdfsFreeBuilder(_builder);

				DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::helper::demangle(typeid(*this).name()).c_str());
			}

			bool hdfs::init()
			{
				_builder = hdfsNewBuilder();

				return true;
			}

			bool hdfs::connect()
			{
				DBGPRN("[%s] requested hdfs::connect()", _id.c_str());

				if (_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] already connected");

				init();
				hdfsBuilderSetForceNewInstance(_builder);

				hdfsBuilderSetNameNode(_builder, _hostname.c_str());
				hdfsBuilderSetNameNodePort(_builder, _port);
				
				hdfsBuilderConfSetStr(_builder, "dfs.encrypt.data.transfer", "true");			// when kerberos and secure is on, we must enable this
				hdfsBuilderConfSetStr(_builder, "dfs.encrypt.data.transfer.algorithm", "3des");	// when kerberos and secure is on, we must enable this
				hdfsBuilderConfSetStr(_builder, "hadoop.security.authentication", "kerberos");	// TODO: only activate when auth method is KRB
				hdfsBuilderSetKerbTicketCachePath(_builder, _cache.c_str());
				// hdfsBuilderSetUserName(_builder, _username.c_str());							// I believe this is optional for kerberos authentication

				_filesystem = hdfsBuilderConnect(_builder);

				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot connect to data source - " + hdfsGetLastError());

				char tmpname[PATH_MAX];

				if (!hdfsGetWorkingDirectory(_filesystem, tmpname, PATH_MAX))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot retrive working path - " + hdfsGetLastError());
				
				_path = tmpname;

				return true;
			}

			bool hdfs::disconnect()
			{
				if (_filesystem)
					hdfsDisconnect(_filesystem);
				
				return true;
			}

			bool hdfs::chwd(std::string path)
			{
				DBGPRN("[%s] requested hdfs::chwd() = %s", _id.c_str(), path.c_str());
				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (!hdfsExists(_filesystem, path.c_str()))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path does not exist " + path);

				if (hdfsSetWorkingDirectory(_filesystem, path.c_str()) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot change directory to " + path);

				_path = path;

				return true;
			}

			std::string hdfs::pwd()
			{
				return _path;
			}

			bool hdfs::mkdir(std::string path)
			{
				DBGPRN("[%s] requested hdfs::mkdir() = %s", _id.c_str(), path.c_str());
				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (hdfsCreateDirectory(_filesystem, path.c_str()) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot create directory " + hdfsGetLastError());

				return true;
			}

			int hdfs::list(const axon::transport::transfer::cb &cbfn)
			{
				DBGPRN("[%s] requested hdfs::list()", _id.c_str());
				int fileCount = 0;
				hdfsFileInfo *hdfsList = NULL;

				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if ((hdfsList = hdfsListDirectory(_filesystem, _path.c_str(), &fileCount)))
				{
					for (int i = 0; i < fileCount; i++)
					{
						struct entry file;
						auto [path, filename] = axon::helper::splitpath(hdfsList[i].mName);

						if (_filter.size() == 0 || (_filter.size() > 0 && regex_match(file.name, _filter[0])))
						{
							file.et = axon::protocol::HDFS;
							file.name = filename;

							if (hdfsList[i].mKind == kObjectKindFile)
								file.flag = axon::flags::FILE;
							else if (hdfsList[i].mKind == kObjectKindDirectory)
								file.flag = axon::flags::DIR;
							else
								file.flag = axon::flags::UNKNOWN;

							cbfn(file);
						}
					}

					hdfsFreeFileInfo(hdfsList, fileCount);
				}

				return true;
			}

			int hdfs::list(std::vector<entry> &vec)
			{
				return list([&vec](const axon::entry &e) mutable { vec.push_back(e); });
			}

			long long hdfs::copy(std::string src, std::string dest, bool compress)
			{
				DBGPRN("[%s] requested hdfs::copy() = %s, %s", _id.c_str(), src.c_str(), dest.c_str());
				std::string srcx, destx;

				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;

				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;

				auto [path, filename] = axon::helper::splitpath(srcx);

				if (src == dest || srcx == dest || path == dest || filename == dest)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (hdfsCopy(_filesystem, srcx.c_str(), _filesystem, destx.c_str()) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot copy file");
				
				return 0L;
			}

			bool hdfs::ren(std::string src, std::string dest)
			{
				DBGPRN("[%s] requested hdfs::ren() = %s, %s", _id.c_str(), src.c_str(), dest.c_str());

				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (hdfsRename(_filesystem, src.c_str(), dest.c_str()) == -1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot rename file - " + hdfsGetLastError());

				return true;
			}

			bool hdfs::del(std::string target)
			{
				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (hdfsDelete(_filesystem, target.c_str(), 0) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot delete " + target);

				return true;
			}

			int hdfs::cb(const struct entry *e)
			{
				return 0;
			}

			long long hdfs::get(std::string src, std::string dest, bool compress)
			{
				DBGPRN("[%s] requested hdfs::get() = %s", _id.c_str(), src.c_str());
				long long filesize = 0, rc = 0;
				char FILEBUF[MAXBUF];
				FILE *fp;
				hdfsFile file;
				std::string srcx;

				int bzerr;
				unsigned int inbyte, outbyte;
				BZFILE *bfp = NULL;

				if (!_filesystem)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] file system not connected.");

				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;

				if (!(file = hdfsOpenFile(_filesystem, srcx.c_str(), O_RDONLY, 0, 0, 0)))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot open file - " + hdfsGetLastError());

				if (!(fp = fopen(dest.c_str(), "wb")))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot open file for writing - " + dest);

				if (compress)
				{
					bfp = BZ2_bzWriteOpen(&bzerr, fp, 3, 0, 30);

					if (bzerr != BZ_OK)
					{
						hdfsCloseFile(_filesystem, file);
						BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
						fclose(fp);
						unlink(dest.c_str());

						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open compression stream");
					}
				}

				do {
					rc = hdfsRead(_filesystem, file, &FILEBUF, MAXBUF);

					if (rc == -1)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] read error from remote");

					if (rc > 0)
					{
						if (compress)
						{
							BZ2_bzWrite(&bzerr, bfp, FILEBUF, rc);
							if (bzerr == BZ_IO_ERROR)
							{
								BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
								fclose(fp);
								unlink(dest.c_str());

								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error in compression stream");
							}
						}
						else
							fwrite(FILEBUF, rc, 1, fp);
						filesize += rc;
					}
				} while (rc != 0);

				hdfsCloseFile(_filesystem, file);

				if (compress)
					BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);

				fflush(fp);
				fclose(fp);

				return filesize;
			}

			long long hdfs::put(std::string src, std::string dest, bool compress)
			{
				DBGPRN("[%s] requested put() = %s", _id.c_str(), src.c_str());
				std::string destx, temp;
				char FILEBUF[MAXBUF];
				long rc, wc;
				unsigned long long filesize = 0;
				
				FILE *fp;
				hdfsFile file;

				if (dest[0] == '/')
				{
					temp = dest + ".tmp";
					destx = dest;
				}
				else
				{
					temp = _path + "/" + dest + ".tmp";
					destx = _path + "/" + dest;
				}

				if (!(fp = fopen(src.c_str(), "rb")))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot open source file '" + src + "'");

				if (!(file = hdfsOpenFile(_filesystem, temp.c_str(), O_WRONLY, 0, 0, 0)))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot open file - " + hdfsGetLastError());

				do {
					
					rc = fread(FILEBUF, 1, MAXBUF, fp);

					if (rc <= 0)
						break;

					if ((wc = hdfsWrite(_filesystem, file, (void*) FILEBUF, rc)) != rc)
			 			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not write to hdfs - " + hdfsGetLastError());

				} while (rc > 0);

				fclose(fp);
				hdfsFlush(_filesystem, file);
				hdfsCloseFile(_filesystem, file);

				ren(temp, destx);

				return filesize;
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
				return false;
			}
		}
	}
}