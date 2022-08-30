#ifndef NODE_H
#define NODE_H

#include <queue>

#include <boost/algorithm/string/replace.hpp>

#include <axon.h>
#include <axon/config.h>
#include <axon/log.h>

#include <axon/sqlite.h>
#include <axon/oracle.h>

#include <axon/util.h>

#define NODE_CFG_NAME 'X'

#define NODE_CFG_SHORTDESC 'A'
#define NODE_CFG_LONGDESC 'B'
#define NODE_CFG_SRC_IPADDRESS 'C'
#define NODE_CFG_SRC_USERNAME 'D'
#define NODE_CFG_SRC_PASSWORD 'E'
#define NODE_CFG_SRC_DOMAIN 'Q'
#define NODE_CFG_SRC_PRIVATE_KEY 'F'
#define NODE_CFG_DST_IPADDRESS 'S'
#define NODE_CFG_DST_USERNAME 'T'
#define NODE_CFG_DST_PASSWORD 'U'
#define NODE_CFG_DST_DOMAIN 'V'
#define NODE_CFG_DST_PRIVATE_KEY 'W'
#define NODE_CFG_BUCKET 'G'
#define NODE_CFG_PICKPATH 'H'
#define NODE_CFG_DROPPATH 'I'
#define NODE_CFG_FILEMASK 'J'
#define NODE_CFG_IGNORE 'K'
#define NODE_CFG_REMMASK 'L'
#define NODE_CFG_TRANSFORM 'M'
#define NODE_CFG_EXEC 'N'
#define NODE_CFG_PRERUN 'O'
#define NODE_CFG_POSTRUN 'P'
#define NODE_CFG_BUFFER 'R'

#define NODE_CFG_STATUS 1
#define NODE_CFG_CONFTYPE 2
#define NODE_CFG_SRC_PROTOCOL 3
#define NODE_CFG_SRC_MODE 4
#define NODE_CFG_SRC_AUTH 5
#define NODE_CFG_SRC_PORT 6
#define NODE_CFG_DST_PROTOCOL 7
#define NODE_CFG_DST_MODE 8
#define NODE_CFG_DST_AUTH 9
#define NODE_CFG_DST_PORT 10
#define NODE_CFG_COMPRESS 11
#define NODE_CFG_LOOKBACK 12
#define NODE_CFG_SLEEPTIME 13
#define NODE_CFG_TRIGGER 14
#define NODE_CFG_TRIM 15
#define NODE_CFG_PARALLEL 16

#define NODE_CFG_PID 17
#define NODE_CFG_PPID 18

#define NODE_MAX_PARALLEL 5

class node {

	std::string _name, _shortdesc, _longdesc, _pickpath[5], _droppath, _bucket, _filemask, _ignore, _remmask, _transform, _exec, _prerun, _postrun, _buffer;
	std::string _src_ipaddress, _src_username, _src_password, _src_domain, _src_privatekey, _dst_ipaddress, _dst_username, _dst_password, _dst_domain, _dst_privatekey;
	int _conftype, _status, _compress, _parallel, _lookback, _sleeptime, _trim, _trigger;
	int _src_protocol, _src_mode, _src_auth, _dst_protocol, _dst_mode, _dst_auth;

	bool isdead, _canrun, _running, _sleeping;
	int *serial;
	int _pid, _ppid;

	std::shared_ptr<axon::database::interface> _db;
	axon::log *_log, dummy;
	struct dbconf _dbc;

	std::thread _th;
	std::string _id;
	// std::condition_variable _cv;
	std::queue<std::string> _pipe;
	std::mutex _safety;

public:

	node();
	~node();
	node(const node&) = delete;

	std::string operator[] (char);
	int operator[] (int);
	std::string get(char);
	int get(int);

	bool set(char, std::string);
	bool set(int, int);
	bool set(axon::log&);
	bool set(dbconf&);

	int reset();
	void enable();
	void disable();
	bool kill();

	int sleep();
	int monitor();
	bool enabled();
	bool running();
	bool wait();

	bool start();
	int run();

	void print(int);

	std::string pop();
};

#endif