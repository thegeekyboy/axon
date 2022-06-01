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

// helper library
#include <axon.h>
#include <axon/config.h>

// local types
struct dbconf {

	int type;
	std::string path;
	std::string address;
	std::string username;
	std::string password;
	std::string gtt;
	std::string list;
	std::string error;

	void load(axon::config &cfg)
	{
		char *s;

		type = cfg.get("type");
		s = cfg.get("path");
		path = s;
		s = cfg.get("address");
		address = s;
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

struct mailconf {

	std::string server;
	std::string username;
	std::string password;

	std::string fault;
	std::string summary;
	std::string body;
	std::string logo;
	std::string title;

	void load(axon::config &cfg)
	{
		char *s;

		s = cfg.get("server");
		server = s;
		s = cfg.get("username");
		username = s;
		s = cfg.get("password");
		password = s;

		s = cfg.get("fault");
		fault = s;
		s = cfg.get("summary");
		summary = s;

		s = cfg.get("template");
		body = s;

		s = cfg.get("logo");
		title = s;

		s = cfg.get("title");
		title = s;
	};
};

// local definitions
#include <interface.h>
#include <node.h>
#include <cluster.h>

#endif
