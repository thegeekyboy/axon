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
#include <condition_variable>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/thread/thread.hpp>

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

#include <axon.h>
 
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

// Custome Headers
#include <interface.h>
#include <node.h>
#include <cluster.h>

#endif
