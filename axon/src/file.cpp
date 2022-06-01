#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/fsuid.h>

#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>

#include <boost/filesystem.hpp>

#include <axon.h>
#include <axon/connection.h>
#include <axon/file.h>
#include <axon/util.h>


namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			file::~file()
			{
				disconnect();
			}

			bool file::init()
			{
				return true;
			}

			bool file::connect()
			{
				// char   *pw_name;       /* username */
				// char   *pw_passwd;     /* user password */
				// uid_t   pw_uid;        /* user ID */
				// gid_t   pw_gid;        /* group ID */
				// char   *pw_gecos;      /* user information */
				// char   *pw_dir;        /* home directory */
				// char   *pw_shell;      /* shell program */

				struct passwd *pw = getpwnam(_username.c_str());

				if (pw == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not lookup UID using username");

				if (setfsuid(-1) != (int) pw->pw_uid)
				{
					setfsuid(pw->pw_uid);
					// std::cout<<"the previous uid = "<<code<<", requested uid = "<<pw->pw_uid<<std::endl;

					if (setfsuid(-1) != (int) pw->pw_uid)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not change fs uid with 'setfsuid'");
				}
				// else
				// 	std::cout<<"same user id as effective user"<<std::endl;

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
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Could not change directory - " + std::string(strerror(errno)));

				_path = path;
					
				return false;
			}

			std::string file::pwd()
			{
				if (_path.size() <= 0 && _fd != -1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path not initialized yet");
				
				return _path;
			}

			bool file::ren(std::string src, std::string dest)
			{
				std::string srcx, destx;
				std::string parent, remainder;

				if (_path.size() <= 0 && _fd != -1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path not initialized yet");
				
				if (src[0] == '/')
					srcx = src;
				else
					srcx = _path + "/" + src;
				
				if (dest[0] == '/')
					destx = dest;
				else
					destx = _path + "/" + dest;

				std::tie(parent, remainder) = axon::splitpath(destx);
				boost::filesystem::create_directories(parent);

				if (std::rename(srcx.c_str(), destx.c_str()))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "File rename failed - " + std::string(strerror(errno)));
				
				return true;
			}

			bool file::del(std::string target)
			{
				std::string targetx;

				if (_path.size() <= 0 && _fd != -1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path not initialized yet");
				
				if (target[0] == '/')
					targetx = target;
				else
					targetx = _path + "/" + target;

				if (std::remove(targetx.c_str()))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "File rename failed - " + std::string(strerror(errno)));

				return false;
			}

			int file::list(const axon::transport::transfer::cb &cbfn)
			{
				struct linux_dirent *e;
				char buf[MAXBUF], d_type;
				long nread, count = 0;

				if (_path.size() <= 0 && _fd != -1)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path not initialized yet");

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
							file.et = axon::entrytypes::FILE;

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
				return list([&](const axon::entry &e) mutable {
					
					vec.push_back(e);
				});
			}

			long long file::get(std::string src, std::string dest, bool compress = false)
			{
				return 0; // return the size
			}

			long long file::put(std::string src, std::string dest, bool decompress = false)
			{
				//ren(temp, dest);

				return 0; // return size
			}
		}
	}
}

