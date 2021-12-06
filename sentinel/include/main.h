#ifndef SENTINEL_MAIN_H
#define SENTINEL_MAIN_H

#ifndef __PRI64_PREFIX
#ifdef WIN32
#define __PRI64_PREFIX "I64"
#else
#if __WORDSIZE == 64
#define __PRI64_PREFIX "l"
#else
#define __PRI64_PREFIX "ll"
#endif  
#endif  
#endif  
#ifndef PRIu64
#define PRIu64 __PRI64_PREFIX "u"
#endif

#define VERSION "v2.0.0"

#define MAX_INSTANCE 512
#define SZBUFFER 262144
#define MSGSIZE 2048

// Library Headers
#include <libconfig.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <bzlib.h>
#include <sqlite3.h>
#include <curl/curl.h>

// helper library
#include <axon.h>
#include <axon/config.h>

// local types
struct dbconf {

	std::string path;
	std::string username;
	std::string password;
	std::string gtt;
	std::string list;
	std::string error;

	void load(axon::config &cfg)
	{
		char *s;

		s = cfg.get("path");
		path = s;
		s = cfg.get("username");
		username = s;
		s = cfg.get("password");
		password = s;
		s = cfg.get("gtt");
		gtt = s;
		s = cfg.get("filelist");
		list = s;
		s = cfg.get("errors");
		error = s;
	};
};

template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

// local definitions
#include <interface.h>
#include <node.h>
#include <cluster.h>

#endif
