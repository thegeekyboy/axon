#include <fstream>
#include <memory>
#include <mutex>
#include <atomic>

#include <fcntl.h>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>

#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <smb2/libsmb2-raw.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/samba.h>
#include <axon/util.h>

namespace axon
{
	namespace transfer
	{
		std::atomic<int> samba::_instance;
		std::mutex samba::_mtx;

		std::string samba::_smb2path(const std::string &abspath)
		{
			std::vector<std::string> parts = axon::util::split(abspath, '/');

			if (parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path belongs to share '" + parts[0] + "', connected to '" + _share + "'");

			std::string prefix;
			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + (i < parts.size() - 1 ? "\\" : "");

			return prefix;
		}

		bool samba::set(char c, std::string value)
		{
			switch (c)
			{
				case AXON_TRANSFER_SAMBA_DOMAIN:
					_domain = value;
					break;

				case AXON_TRANSFER_SAMBA_SHARE:
					_share = value;
					break;
			}

			return true;
		}

		samba::~samba()
		{
			if (_fileopen) close();
			disconnect();

			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		bool samba::init()
		{
			std::lock_guard<std::mutex> guard(_mtx);

			_smb2 = NULL;
			_dir = NULL;

			if ((_smb2 = smb2_init_context()) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot initialize smb context");

			_instance++;

			return true;
		}

		bool samba::connect()
		{
			init();

			smb2_set_security_mode(_smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
			smb2_set_authentication(_smb2, 1);

			smb2_set_domain(_smb2, _domain.c_str());
			smb2_set_user(_smb2, _username.c_str());
			smb2_set_password(_smb2, _password.c_str());

			if (smb2_connect_share(_smb2, _hostname.c_str(), _share.c_str(), _username.c_str()) < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "]" + std::string("connect failed :") + smb2_get_error(_smb2));

			_connected = true;

			return true;
		}

		bool samba::disconnect()
		{
			std::lock_guard<std::mutex> guard(_mtx);

			if (_dir)
				smb2_closedir(_smb2, _dir);

			if (_smb2)
				smb2_destroy_context(_smb2);

			_connected = false;
			_instance--;

			return true;
		}

		bool samba::chwd(std::string path)
		{
			std::string prefix;

			if (path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid path");

			if (path[0] == '.' || (path.size() >= 2 && path[0] == '.' && path[1] == '.') || path[0] != '/')
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] relative path not supported");

			// TODO: implement relative path

			std::vector<std::string> parts = axon::util::split(path, '/');

			if (parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] changing share from " + _share + " to " + parts[0] + " is not supported after connect");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "/";

			struct smb2dir *dir;

			if ((dir = smb2_opendir(_smb2, prefix.c_str())) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot open directory " + prefix);

			if (_dir)
				smb2_closedir(_smb2, _dir);

			_dir = dir;
			_path = path;

			return false;
		}

		std::string samba::pwd()
		{
			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			return _path;
		}

		bool samba::mkdir(std::string path)
		{
			std::string pathx, prefix;

			// if (_path.size() <= 0)
			// 	throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized"); <- should we do this?

			if (path[0] == '.' || (path.size() >= 2 && path[0] == '.' && path[1] == '.'))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] relative path not supported");

			if (path[0] == '/')
				pathx = path;
			else
				pathx = _path + "/" + path;

			std::vector<std::string> parts = axon::util::split(pathx, '/');

			if (parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] changing share from " + _share + " to " + parts[0] + " is not supported after connect");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "/";

			if (smb2_mkdir(_smb2, prefix.c_str()) < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error creating directory");

			return true;
		}

		off_t samba::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			std::string srcx, destx;
			long long filesize = 0;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			auto [path, filename] = axon::util::splitpath(srcx);

			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] server-side copy operation currently not supported");

			return filesize;
		}

		bool samba::ren(std::string src, std::string dest)
		{
			std::string srcx, destx;
			std::string parent, remainder;

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			std::vector<std::string> src_parts = axon::util::split(srcx, '/');
			std::vector<std::string> dest_parts = axon::util::split(destx, '/');

			if (src_parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] ren:changing share from " + _share + " to " + src_parts[0] + " is not supported after connect");

			if (dest_parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination share must be same");

			std::string src_prefix, dest_prefix;

			for (unsigned int i = 1; i < src_parts.size(); i++)
				src_prefix += src_parts[i] + "\\";
			src_prefix.erase(src_prefix.length() - 1);

			for (unsigned int i = 1; i < dest_parts.size(); i++)
				dest_prefix += dest_parts[i] + "\\";
			dest_prefix.erase(dest_prefix.length() - 1);

			if (smb2_rename(_smb2, src_prefix.c_str(), dest_prefix.c_str()) < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "]" + std::string("failed to rename file ") + smb2_get_error(_smb2));

			return true;
		}

		bool samba::del(std::string target)
		{
			std::string targetx;

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (target[0] == '/')
				targetx = target;
			else
				targetx = _path + "/" + target;

			std::vector<std::string> parts = axon::util::split(targetx, '/');
			std::string prefix;

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "\\";
			prefix.erase(prefix.length() - 1);

			if (smb2_unlink(_smb2, prefix.c_str()) < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "]" + std::string("failed to delete file ") + smb2_get_error(_smb2));

			return true;
		}

		size_t samba::list(const axon::transfer::cb &cbfn)
		{
			long count = 0;

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			smb2_rewinddir(_smb2, _dir);

			struct smb2dirent *ent;
			while ((ent = smb2_readdir(_smb2, _dir)))
			{
				if (_filter.size() == 0 || (_filter.size() > 0 && regex_match(ent->name, _filter[0])))
				{
					struct entry file;

					file.name = ent->name;
					file.size = ent->st.smb2_size;
					file.proto = axon::protocol::SAMBA;

					switch (ent->st.smb2_type)
					{
						case SMB2_TYPE_FILE:
							file.flag = axon::flags::FILE;
							break;

						case SMB2_TYPE_DIRECTORY:
							file.flag = axon::flags::DIR;
							break;

						case SMB2_TYPE_LINK:
							file.flag = axon::flags::LINK;
							break;

						default:
							break;
					}

					count++;
					cbfn(file);
				}
			}

			return count;
		}

		size_t samba::list(std::vector<axon::entry> &vec)
		{
			return list([&](const axon::entry &e) mutable {
				vec.push_back(e);
			});
		}

		off_t samba::get(std::string src, std::string dest, bool compress)
		{
			std::string srcx;
			struct smb2fh *fh;
			long long filesize = 0, count = 0;
			uint8_t buffer[axon::MAX_BUFFER_SIZE];

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			std::vector<std::string> parts = axon::util::split(srcx, '/');
			std::string prefix;

			if (parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] get:changing share from " + _share + " to " + parts[0] + " is not supported after connect");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "\\";
			prefix.erase(prefix.length() - 1);

			if ((fh = smb2_open(_smb2, prefix.c_str(), O_RDONLY)) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open remote file for reading");

			std::ofstream file(dest, std::ios::out | std::ios::trunc | std::ios::binary);
			boost::iostreams::filtering_ostream compressor;

			if (file.fail())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open file for writing" + dest);

			if (compress)
				compressor.push(boost::iostreams::bzip2_compressor());
			compressor.push(file);

			while ((count = smb2_pread(_smb2, fh, buffer, axon::MAX_BUFFER_SIZE - 1, filesize)) > 0)
			{
				compressor.write(reinterpret_cast<char*>(&buffer), count);
				filesize += count;
			}

			smb2_close(_smb2, fh);
			compressor.flush();
			compressor.reset();
			file.close();

			return filesize;
		}

		off_t samba::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			std::string destx;
			long long filesize = 0, count, rc;
			struct smb2fh *fh;
			uint8_t buffer[axon::MAX_BUFFER_SIZE];

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			std::vector<std::string> parts = axon::util::split(destx, '/');
			std::string prefix;

			if (parts[0] != _share)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] changing share from " + _share + " to " + parts[0] + " is not supported after connect");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "\\";
			prefix.erase(prefix.length() - 1);

			std::ifstream input(src, std::ios::in | std::ios::binary);

			if (input.fail())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open file for reading");

			if ((fh = smb2_open(_smb2, prefix.c_str(), O_WRONLY|O_CREAT)) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open remote file for writing - " + prefix + "<~~>" + smb2_get_error(_smb2));

			while (true)
			{
				input.read (reinterpret_cast<char*>(&buffer), axon::MAX_BUFFER_SIZE - 1);

				if ((count = input.gcount()) <= 0)
					break;

				if ((rc = smb2_write(_smb2, fh, buffer, count)) < 0)
					break;

				filesize += count;
			}

			smb2_close(_smb2, fh);
			input.close();

			return filesize;
		}

		bool samba::open(std::string filename, std::ios_base::openmode om)
		{
			DBGPRN("[%s] requested samba::open() %s to %s", _id.c_str(), filename.c_str(), ((om == std::ios::out) ? "write" : "read"));

			if (_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] a file is already open");

			if (_path.size() <= 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			std::string finalpath = (filename[0] == '/') ? filename : _path + "/" + filename;
			std::string smb2rel = _smb2path(finalpath);

			int flags = (om & std::ios::out) ? (O_WRONLY | O_CREAT | O_TRUNC) : O_RDONLY;

			if ((_fh = smb2_open(_smb2, smb2rel.c_str(), flags)) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not open file '" + smb2rel + "' - " + smb2_get_error(_smb2));

			_fileopen = true;
			_om = om;
			_offset = 0;

			return _fileopen;
		}

		bool samba::close()
		{
			DBGPRN("[%s] requested samba::close()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			smb2_close(_smb2, _fh);
			_fh = NULL;
			_fileopen = false;
			_offset = 0;

			return !_fileopen;
		}

		bool samba::push(axon::transfer::connection &conn)
		{
			DBGPRN("[%s] requested samba::push()", _id.c_str());

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			char buffer[axon::MAX_BUFFER_SIZE];
			ssize_t size;

			while ((size = this->read(buffer, axon::MAX_BUFFER_SIZE - 1)) > 0)
				conn.write(buffer, size);

			return true;
		}

		ssize_t samba::read(char *buffer, size_t size)
		{
			DBGPRN("[%s] requested samba::read() => size(%ld)", _id.c_str(), size);

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::in))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform read operation when file is open for write");

			ssize_t rc = smb2_pread(_smb2, _fh, reinterpret_cast<uint8_t*>(buffer), size, _offset);

			if (rc < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] read error - " + smb2_get_error(_smb2));

			_offset += rc;

			return rc;
		}

		ssize_t samba::write(const char *buffer, size_t size)
		{
			DBGPRN("[%s] requested samba::write() => size(%ld)", _id.c_str(), size);

			if (!_fileopen)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no file is open");

			if (!(_om & std::ios::out))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] cannot perform write operation when file is open for read");

			ssize_t remaining = static_cast<ssize_t>(size), written = 0;

			while (remaining > 0)
			{
				ssize_t rc = smb2_pwrite(_smb2, _fh, reinterpret_cast<const uint8_t*>(buffer + written), remaining, _offset);

				if (rc < 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] write error - " + smb2_get_error(_smb2));

				_offset += rc;
				written += rc;
				remaining -= rc;
			}

			return written;
		}

	}
}

