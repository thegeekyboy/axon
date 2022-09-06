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

#define NODE_CFG_NAME            'Z'

#define NODE_CFG_SHORTDESC       'A'
#define NODE_CFG_LONGDESC        'B'

#define NODE_CFG_SRC_IPADDRESS   'C'
#define NODE_CFG_SRC_USERNAME    'D'
#define NODE_CFG_SRC_PASSWORD    'E'
#define NODE_CFG_SRC_DOMAIN      'F'
#define NODE_CFG_SRC_PRIVATE_KEY 'G'
#define NODE_CFG_SRC_PATH        'H'
#define NODE_CFG_SRC_BUCKET      'I'

#define NODE_CFG_DST_IPADDRESS   'J'
#define NODE_CFG_DST_USERNAME    'K'
#define NODE_CFG_DST_PASSWORD    'L'
#define NODE_CFG_DST_DOMAIN      'M'
#define NODE_CFG_DST_PRIVATE_KEY 'N'
#define NODE_CFG_DST_PATH        'O'
#define NODE_CFG_DST_BUCKET      'P'

#define NODE_CFG_FILEMASK        'Q'
#define NODE_CFG_IGNORE          'R'
#define NODE_CFG_REMMASK         'S'
#define NODE_CFG_TRANSFORM       'T'
#define NODE_CFG_EXEC            'U'
#define NODE_CFG_PRERUN          'V'
#define NODE_CFG_POSTRUN         'W'
#define NODE_CFG_BUFFER          'X'

#define NODE_CFG_STATUS           0x00
#define NODE_CFG_CONFTYPE         0x01
#define NODE_CFG_SRC_PROTOCOL     0x02
#define NODE_CFG_SRC_MODE         0x03
#define NODE_CFG_SRC_AUTH         0x04
#define NODE_CFG_SRC_PORT         0x05
#define NODE_CFG_DST_PROTOCOL     0x06
#define NODE_CFG_DST_MODE         0x07
#define NODE_CFG_DST_AUTH         0x08
#define NODE_CFG_DST_PORT         0x09
#define NODE_CFG_COMPRESS         0x0A
#define NODE_CFG_LOOKBACK         0x0B
#define NODE_CFG_SLEEPTIME        0x0C
#define NODE_CFG_TRIGGER          0x0D
#define NODE_CFG_TRIM             0x0E
#define NODE_CFG_PARALLEL         0x0F

#define NODE_CFG_PID              0x10
#define NODE_CFG_PPID             0x11

#define NODE_MAX_PARALLEL         5

struct dlobj {

	unsigned int index;
	unsigned int total;
	std::string filename;
};

class node {

	std::string _name, _shortdesc, _longdesc, _filemask, _ignore, _remmask, _transform, _exec, _prerun, _postrun, _buffer;
	std::string _src_ipaddress, _src_username, _src_password, _src_domain, _src_privatekey, _src_path[5], _src_bucket;
	std::string _dst_ipaddress, _dst_username, _dst_password, _dst_domain, _dst_privatekey, _dst_path[5], _dst_bucket;
	
	int _conftype, _status, _compress, _parallel, _lookback, _sleeptime, _trim, _trigger;
	int _src_protocol, _src_mode, _src_auth, _dst_protocol, _dst_mode, _dst_auth;

	bool isdead, _canrun, _running, _sleeping;
	int *serial;
	int _pid, _ppid;

	std::shared_ptr<axon::database::interface> _db;
	axon::log *_log, dummy;
	struct dbconf _dbc;

	std::string _id;
	std::queue<struct dlobj> _pipe;

	std::thread _th;
	std::mutex _safety;
	
	std::string _protoname(int);

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

	struct dlobj pop();
};

#endif