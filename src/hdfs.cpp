#include <axon.h>
#include <axon/connection.h>
#include <axon/hdfs.h>
#include <axon/util.h>

#include <bzlib.h>

#define AXON_HADOOP_CFG_URI			"dfs.default.uri"
#define AXON_HADOOP_CFG_LOGLEVEL	"dfs.client.log.severity"
#define AXON_HADOOP_CFG_PROTECT		"hadoop.rpc.protection"							// can be authentication/?
#define AXON_HADOOP_CFG_AUTHTYPE	"hadoop.security.authentication"				// can be simple, kerberos (https://hadoop.apache.org/docs/stable/hadoop-project-dist/hadoop-common/SecureMode.html)
#define AXON_HADOOP_CFG_AUTHORIZE	"hadoop.security.authorization"					// can be true/false
#define AXON_HADOOP_CFG_ENCRYPT		"dfs.encrypt.data.transfer"						// can be true/false
#define AXON_HADOOP_CFG_AUTHALGO	"dfs.encrypt.data.transfer.algorithm"			// can be 3ds
#define AXON_HADOOP_CFG_CACHEPATH	"hadoop.security.kerberos.ticket.cache.path"	// path to the cache file

namespace axon
{
	namespace transfer
	{
		hdfs::hdfs(std::string hostname, std::string username, std::string password, uint16_t port):
		connection(hostname, username, password, port), _builder(NULL), _filesystem(NULL), _fp(NULL)
		{
			_builder = hdfsNewBuilder();
			hdfsBuilderSetForceNewInstance(_builder);
		};

		hdfs::~hdfs()
		{
			if (_fileopen) close();
			// disconnect();

			if (_builder)
				hdfsFreeBuilder(_builder);

			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		bool hdfs::connect()
		{
			DBGPRN("[%s] requested hdfs::connect()", _id.c_str());

			if (_filesystem)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] already connected");

			// hdfsBuilderSetForceNewInstance(_builder);

			hdfsBuilderSetNameNode(_builder, _hostname.c_str());
			hdfsBuilderSetNameNodePort(_builder, _port);
			hdfsBuilderSetUserName(_builder, _username.c_str());

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

		int hdfs::list(const axon::transfer::cb &cbfn)
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
					auto [path, filename] = axon::util::splitpath(hdfsList[i].mName);

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

		long long hdfs::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
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

			auto [path, filename] = axon::util::splitpath(srcx);

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

		int hdfs::cb(const struct entry *)
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

		long long hdfs::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			DBGPRN("[%s] requested put() = %s", _id.c_str(), src.c_str());
			std::string destx, temp;
			char FILEBUF[MAXBUF];
			long rc, wc;
			unsigned long long filesize = 0;

			FILE *fp;
			hdfsFile file;

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			if (!(fp = fopen(src.c_str(), "rb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Cannot open source file '" + src + "'");

			if (!(file = hdfsOpenFile(_filesystem, destx.c_str(), O_WRONLY|O_CREAT, 0, 0, 0)))
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

			return filesize;
		}

		bool hdfs::set(char key, std::string value)
		{
			switch (key)
			{
				case AXON_TRANSFER_HDFS_DOMAIN: _domain = value; return true;

				case AXON_TRANSFER_HDFS_CACHEPATH: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_CACHEPATH, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_URI: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_URI, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_LOGLEVEL: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_LOGLEVEL, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_PROTECT: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_PROTECT, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_AUTHTYPE: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_AUTHTYPE, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_AUTHORIZE: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_AUTHORIZE, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_AUTHALGO: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_AUTHALGO, value.c_str()); return true;
				case AXON_TRANSFER_HDFS_ENCRYPT: hdfsBuilderConfSetStr(_builder, AXON_HADOOP_CFG_ENCRYPT, value.c_str()); return true;
			}

			return false;
		}

		bool hdfs::set(char, int)
		{
			return false;
		}

		bool hdfs::open(std::string filename, std::ios_base::openmode om)
		{
			DBGPRN("[%s] requested file::open() %s to %s", _id.c_str(), filename.c_str(), ((om==std::ios::out)?"write":"read"));

			if (_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] a file is already open");

			std::string finalpath;

			if (filename[0] == '/')
				finalpath = filename;
			else
				finalpath = _path + "/" + filename;

			std::vector<std::string> parts = axon::util::split(finalpath, '/');

			if (om & std::ios::out)
				_fp = hdfsOpenFile(_filesystem, finalpath.c_str(), O_RDONLY, 0, 0, 0);
			else
				_fp = hdfsOpenFile(_filesystem, finalpath.c_str(), O_WRONLY|O_CREAT, 0, 0, 0);

			if (!_fp) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error opening file " + finalpath);

			_fileopen = true;
			_om = om;

			return _fileopen;
		}

		bool hdfs::close()
		{
			DBGPRN("[%s] requested hdfs::close()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			hdfsFlush(_filesystem, _fp);
			hdfsCloseFile(_filesystem, _fp);
			_fp = NULL;

			_fileopen = false;

			return !_fileopen;
		}

		bool hdfs::push(axon::transfer::connection& conn)
		{
			DBGPRN("[%s] requested hdfs::push()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			char buffer[MAXBUF];
			ssize_t size = 0;

			while ((size = this->read(buffer, MAXBUF-1)) > 0)
				conn.write(buffer, size);

			return true;
		}

		ssize_t hdfs::read(char* buffer, size_t size)
		{
			DBGPRN("[%s] requested hdfs::read() => size(%ld)", _id.c_str(), size);

			ssize_t filesize = 0;

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::in))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform read operation when file is open for write");

			if ((filesize = hdfsRead(_filesystem, _fp, &buffer, size)) > 0)
				return 0;

			return filesize;
		}

		ssize_t hdfs::write(const char* buffer, size_t size)
		{
			DBGPRN("[%s] requested hdfs::write() => size(%ld)", _id.c_str(), size);

			ssize_t rc = 0, remaining = size, filesize = 0;

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::out))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform write operation when file is open for read");

			do {

				if ((rc = hdfsWrite(_filesystem, _fp, (void*) buffer, remaining)) < 0)
					break;

				buffer += rc;
				filesize += rc;
				remaining -= rc;

			} while (remaining > 0);

			return filesize;
		}
	}
}
