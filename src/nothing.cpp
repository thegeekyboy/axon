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

			bool nothing::chwd(std::string) { return true; };
			std::string nothing::pwd()  { return "/"; };
			bool nothing::mkdir(std::string) { return true; };
			int nothing::list(const axon::transport::transfer::cb &) { return 0; };
			int nothing::list(std::vector<entry> &) { return 0; };
			long long nothing::copy(std::string , std::string , bool ) { return 0L; };
			bool nothing::ren(std::string , std::string ) { return true; };
			bool nothing::del(std::string ) { return true; };

			int nothing::cb(const struct entry *) { return 0; };

			long long nothing::get(std::string , std::string , bool ) { return 0L; };
			long long nothing::put(std::string , std::string , bool ) { return 0L; };

		}
	}
}
