#ifndef TCN_MAIN_H
#define TCN_MAIN_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdarg>
#include <exception>
#include <chrono>
#include <typeinfo>
#include <iomanip>
#include <atomic>
#include <queue>

#include <boost/format.hpp>
#include <boost/regex.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/stat.h>

// Library Headers
#include <libconfig.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <bzlib.h>
#include <sqlite3.h>
#include <curl/curl.h>
 
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

#define VERSION "v0.0.1"

#define MAX_INSTANCE 64
#define MAXBUF 262144
#define MSGSIZE 2048

// Custom Data Type
struct cException : public std::exception {

	std::string m_filename, m_message, m_func;
	int m_linenum;

public:
	cException(std::string filename, int linenum, std::string func, std::string msg) : m_filename(filename), m_linenum(linenum), m_func(func), m_message(msg) { };
	~cException() throw() {};

	const char *what () const throw () {
		std::string f_msg = m_filename + "(" + std::to_string(m_linenum) + ") in " + m_func + "(): " + m_message;
		return f_msg.c_str();
	};
};

// Forward Defenition
struct msgque;

// TCN Namespace
namespace tcn
{
	struct entry {

		std::string name;
		int type;
		long long size;
	};
}

// Custome Headers
#include <node.h>
#include <socket.h>
#include <config.h>
#include <logger.h>
#include <connection.h>
#include <database.h>
#include <file.h>
#include <ftp.h>
#include <ftplist.h>
#include <main.h>
#include <msg.h>
#include <ssh.h>
#include <util.h>

// Global Variables
extern config cfg;
// extern logger lg;
// extern tcn::database::sqlite db;
// extern nodes tree;

// extern std::mutex mtxdb;
//extern mutex mtxlg;

//extern static boost::atomic_uint __libshh2_session_count;
//extern static mutex __libshh2_session_count_mutex;

#endif
