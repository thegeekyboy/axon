#include <axon.h>
#include <axon/connection.h>
#include <axon/nothing.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
			nothing::~nothing() {};

			bool nothing::set(char, std::string) { return true; };

			bool nothing::connect() { return true; };
			bool nothing::disconnect() { return true; };

			bool nothing::chwd(std::string path) { return true; };
			std::string nothing::pwd()  { return "/"; };
			bool nothing::mkdir(std::string path) { return true; };
			int nothing::list(const axon::transport::transfer::cb &f) { return 0; };
			int nothing::list(std::vector<entry> &list) { return 0; };
			long long nothing::copy(std::string src, std::string dest, bool compress) { return 0L; };
			bool nothing::ren(std::string src, std::string dest) { return true; };
			bool nothing::del(std::string target) { return true; };

			int nothing::cb(const struct entry *e) { return 0; };

			long long nothing::get(std::string src, std::string dest, bool compress) { return 0L; };
			long long nothing::put(std::string src, std::string dest, bool compress) { return 0L; };

		}
	}
}
