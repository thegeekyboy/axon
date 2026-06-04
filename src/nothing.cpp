#include <axon.h>
#include <axon/connection.h>
#include <axon/nothing.h>

namespace axon
{
	namespace transfer
	{
		bool nothing::push(axon::transfer::connection&) { return false; };

		nothing::nothing(std::string hostname, std::string username, std::string password, uint16_t port) : connection(hostname, username, password, port) { };
		nothing::nothing(std::string hostname, std::string username, std::string password): connection(hostname, username, password) {  };
		nothing::nothing(const nothing& rhs) : connection(rhs) {  };
		nothing::~nothing() {};

		bool nothing::set(char, std::string) { return true; };

		bool nothing::connect() { return true; };
		bool nothing::disconnect() { return true; };

		bool nothing::chwd(std::string) { return true; };
		std::string nothing::pwd()  { return "/"; };
		bool nothing::mkdir(std::string) { return true; };
		size_t nothing::list(const axon::transfer::cb &) { return 0; };
		size_t nothing::list(std::vector<entry> &) { return 0; };
		off_t nothing::copy(std::string , std::string , bool ) { return 0L; };
		off_t nothing::copy(std::string src, std::string dest) { return copy(src, dest, false); };
		bool nothing::ren(std::string , std::string ) { return true; };
		bool nothing::del(std::string ) { return true; };

		int nothing::cb(const struct entry *) { return 0; };

		off_t nothing::get(std::string , std::string , bool ) { return 0L; };
		off_t nothing::put(std::string , std::string , bool ) { return 0L; };

		bool nothing::open(std::string, std::ios_base::openmode) { return true; }
		bool nothing::close() { return true; }

		ssize_t nothing::read(char*, size_t) { return 0L; }
		ssize_t nothing::write(const char*, size_t) { return 0L; }
	}
}
