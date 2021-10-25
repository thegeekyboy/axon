#ifndef AXON_MASTER_H_
#define AXON_MASTER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdarg>
#include <exception>
#include <chrono>
#include <typeinfo>
#include <iomanip>
#include <atomic>
#include <stack>
#include <queue>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUF 1048576

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// AXON Namespace
namespace axon
{
	typedef unsigned int flags_t;
	typedef unsigned int entry_t;

	struct flags {

		static const flags_t DIR = 1;
		static const flags_t FILE = 2;
		static const flags_t LINK = 4;
		static const flags_t CHAR = 8;
		static const flags_t BLOCK = 16;
		static const flags_t FIFO = 32;
		static const flags_t SOCKET = 64;
	};

	struct entrytypes {

		static const entry_t FILE = 1;
		static const entry_t SFTP = 2;
		static const entry_t SCP = 4;
		static const entry_t FTP = 8;
		static const entry_t SAMBA = 16;
		static const entry_t AWS = 32;
		static const entry_t HDFS = 64;
		static const entry_t DATABASE = 128;
		static const entry_t KAFKA = 256;
	};

	struct entry {

		std::string name;
		int type;
		long long size;
		flags_t flag;
		entry_t et;
	};

	struct licensekey
	{
		char version[8];
		char key[64];
		char validity[64];
		int expiredate;
	};

	// This is the old one- for some reason this does not work sometimes!
	// class exception : public std::exception {

	// 	std::string m_filename, m_message, m_func;
	// 	int m_linenum;

	// public:
	// 	exception(std::string filename, int linenum, std::string func, std::string msg)
	// 	{
	// 		m_filename = filename;
	// 		m_message = msg;
	// 		m_func = func;
	// 		m_linenum = linenum;
	// 	};
	// 	~exception() throw() {};

	// 	virtual const char* what() const throw () {
			
	// 		std::string f_msg = m_filename + "(" + std::to_string(m_linenum) + ") in " + m_func + "(): " + m_message;
			
	// 		return f_msg.c_str();
	// 	};
	// };

	class exception : public std::exception {

		char _what[4096];
		char _msg[4096];

	public:
		exception(std::string filename, int linenum, std::string func, std::string msg)
		{
			sprintf(_what, "%s(%d) in %s(): %s", filename.c_str(), linenum, func.c_str(), msg.c_str());
			strcpy(_msg, msg.c_str());
		};

		~exception() throw() {};

		virtual const char* what() const throw () {
			
			return _what;
		};

		virtual const char* msg() const throw () {
			
			return _msg;
		};
	};

}

// Forward Definition

// Custom Headers
// #include <md5.h>
// #include <aes.h>
// #include <util.h>
// #include <dmi.h>
// #include <log.h>
// #include <config.h>
// #include <msg.h>
// #include <database.h>
// #include <node.h>
// #include <config.h>
// #include <connection.h>
// #include <ssh.h>
// #include <socket.h>
// #include <file.h>
// #include <ftplist.h>
// #include <ftp.h>
// #include <telecom.h>
// #include <mml.h>

// Global Variables


#endif
