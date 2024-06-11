#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/fsuid.h>

#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>

#include <bzlib.h>
#include <boost/filesystem.hpp>

#include <axon.h>
#include <axon/connection.h>
#include <axon/file.h>
#include <axon/util.h>


namespace axon
{
	namespace transfer
	{
		file::~file()
		{
			disconnect();
			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		bool file::init()
		{
			return true;
		}

		long long file::copy(std::string src, std::string dest, bool compress)
		{
			DBGPRN("[%s] requested file::copy() src = %s, dest = %s", _id.c_str(), src.c_str(), dest.c_str());

			int bzerr;
			unsigned int inbyte, outbyte;
			size_t filesize = 0, szr, szw;
			unsigned char FILEBUF[MAXBUF];
			std::string srcx;

			FILE *fps, *fpd;
			BZFILE *bfp = NULL;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			auto [path, filename] = axon::util::splitpath(srcx);
			
			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

			if (!(fpd = fopen(dest.c_str(), "wb")))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error opening file for writing " + dest);
			
			if (!(fps = fopen(src.c_str(), "rb")))
			{
				fclose(fpd);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error opening file for reading " + src);
			}

			if (compress)
			{
				bfp = BZ2_bzWriteOpen(&bzerr, fpd, 3, 0, 30);
				if (bzerr != BZ_OK)
				{
					BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
					fclose(fps);
					fclose(fpd);
					unlink(dest.c_str());

					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not open compression stream");
				}
			}

			do {

				if ((szr = fread(FILEBUF, 1, MAXBUF, fps)) > 0)
				{
					if (compress)
					{
						BZ2_bzWrite(&bzerr, bfp, FILEBUF, szr);
						
						if (bzerr == BZ_IO_ERROR)
						{
							BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);
							fclose(fps);
							fclose(fpd);
							unlink(dest.c_str());

							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Error in comppression stream");
						}
					}
					else
						szw += fwrite(FILEBUF, 1, szr, fpd);
					filesize += szr;
				}
				else
					break;

			} while (true);

			if (compress)
				BZ2_bzWriteClose(&bzerr, bfp, 0, &inbyte, &outbyte);

			fclose(fps);
			fflush(fpd);
			fclose(fpd);

			return filesize;
		}

		bool file::connect()
		{
			// char   *pw_name;		/* username */
			// char   *pw_passwd;	/* user password */
			// uid_t   pw_uid;		/* user ID */
			// gid_t   pw_gid;		/* group ID */
			// char   *pw_gecos;	/* user information */
			// char   *pw_dir;		/* home directory */
			// char   *pw_shell;	/* shell program */

			struct passwd *pw = getpwnam(_username.c_str());

			if (pw == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not lookup UID using username");

			if (setfsuid(-1) != (int) pw->pw_uid)
			{
				setfsuid(pw->pw_uid); // setfsuid is thread specific, so this change is applicable for this thread only.
				DBGPRN("requested uid = %d", pw->pw_uid);

				if (setfsuid(-1) != (int) pw->pw_uid)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not change fs uid with 'setfsuid' to " + std::to_string(pw->pw_uid));
			}

			return true;
		}

		bool file::disconnect()
		{
			if (_fd != -1)
				close(_fd);

			return true;
		}

		bool file::chwd(std::string path)
		{
			if (_fd != -1)
				close(_fd);

			if ((_fd = open(path.c_str(), O_RDONLY | O_DIRECTORY)) == -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Could not change directory - " + std::string(strerror(errno)));

			_path = path;
				
			return false;
		}

		std::string file::pwd()
		{
			if (_path.size() <= 0 && _fd != -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Path not initialized yet");
			
			return _path;
		}

		bool file::mkdir([[maybe_unused]] std::string dir)
		{
			return true;
		}

		bool file::ren(std::string src, std::string dest)
		{
			std::string srcx, destx;
			std::string parent, remainder;

			if (_path.size() <= 0 && _fd != -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Path not initialized yet");
			
			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;
			
			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			std::tie(parent, remainder) = axon::util::splitpath(destx);
			boost::filesystem::create_directories(parent);

			if (std::rename(srcx.c_str(), destx.c_str()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] File rename failed - " + std::string(strerror(errno)));
			
			return true;
		}

		bool file::del(std::string target)
		{
			std::string targetx;

			if (_path.size() <= 0 && _fd != -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Path not initialized yet");
			
			if (target[0] == '/')
				targetx = target;
			else
				targetx = _path + "/" + target;

			if (std::remove(targetx.c_str()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] File rename failed - " + std::string(strerror(errno)));

			return false;
		}

		int file::list(const axon::transfer::cb &cbfn)
		{
			struct linux_dirent *e;
			char buf[MAXBUF], d_type;
			long nread, count = 0;

			if (_path.size() <= 0 && _fd != -1)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Path not initialized yet");

			while (true)
			{
				nread = syscall(SYS_getdents, _fd, buf, MAXBUF);

				if (nread == -1 || nread == 0)
					break;

				for (long bpos = 0; bpos < nread;)
				{
					e = (struct linux_dirent *) (buf + bpos);
					d_type = *(buf + bpos + e->d_reclen - 1);

					if (_filter.size() == 0 || (_filter.size() > 0 && regex_match(e->d_name, _filter[0])))
					{
						struct stat st;
						struct entry file;

						file.name = e->d_name;
						file.et = axon::protocol::FILE;

						switch(d_type)
						{
							case DT_BLK:
								file.flag = axon::flags::BLOCK;
								break;
							
							case DT_CHR:
								file.flag = axon::flags::CHAR;
								break;
							
							case DT_DIR:
								file.flag = axon::flags::DIR;
								break;

							case DT_FIFO:
								file.flag = axon::flags::FIFO;
								break;

							case DT_LNK:
								file.flag = axon::flags::LINK;
								break;

							case DT_REG:
								file.flag = axon::flags::FILE;
								break;

							case DT_SOCK:
								file.flag = axon::flags::SOCKET;
								break;
							
							case DT_UNKNOWN:
								file.flag = axon::flags::UNKNOWN;
								break;
						}

						fstatat(_fd, e->d_name, &st, 0);
						file.size = st.st_size;
						file.st = st;

						count++;
						cbfn(file);
					}

					bpos += e->d_reclen;
				}
			}

			return count;
		}

		int file::list(std::vector<axon::entry> &vec)
		{
			return list([&](const axon::entry &e) mutable { vec.push_back(e); });
		}

		long long file::get(std::string src, std::string dest, bool compress)
		{
			std::string srcx, destx;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;
			
			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			DBGPRN("[%s] requested file::get() = %s", _id.c_str(), destx.c_str());
			return copy(srcx, destx, compress); // return size
		}

		long long file::put(std::string src, std::string dest, bool compress)
		{
			std::string srcx, destx;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;
			
			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			//ren(temp, dest);
			DBGPRN("[%s] requested file::put() src = %s, dest = %s", _id.c_str(), srcx.c_str(), dest.c_str());
			return copy(srcx, destx, compress); // return size
		}
	}
}