#include <axon.h>
#include <connection.h>
#include <file.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			file::~file()
			{
			}

			bool file::init()
			{
				return true;
			}

			bool file::connect()
			{

				return true;
			}

			bool file::disconnect()
			{

				return true;
			}

			bool file::login()
			{

				return true;
			}

			bool file::chwd(std::string path)
			{

				return false;
			}

			std::string file::pwd()
			{

				return _path;
			}

			bool file::ren(std::string from, std::string to)
			{

				return false;
			}

			bool file::del(std::string target)
			{

				return false;
			}

			int file::list(callback)
			{

				return true;
			}

			int file::list(std::vector<axon::entry> *vec)
			{

				return true;
			}

			long long file::get(std::string src, std::string dest, bool compress = false)
			{

				return 0; // return the size
			}

			long long file::put(std::string src, std::string dest)
			{

				//ren(temp, dest);

				return 0; // return size
			}
		}
	}
}

