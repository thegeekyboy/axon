#ifndef NODE_H
#define NODE_H

#define MAX_NODE 32

#define NODE_CFG_NAME 'X'

#define NODE_CFG_SHORTDESC 'A'
#define NODE_CFG_LONGDESC 'B'
#define NODE_CFG_IPADDRESS 'C'
#define NODE_CFG_USERNAME 'D'
#define NODE_CFG_PASSWORD 'E'
#define NODE_CFG_PRIVATE_KEY 'F'
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
#define NODE_CFG_DOMAIN 'Q'

#define NODE_CFG_STATUS 1
#define NODE_CFG_CONFTYPE 2
#define NODE_CFG_PROTOCOL 3
#define NODE_CFG_MODE 4
#define NODE_CFG_AUTH 5
#define NODE_CFG_COMPRESS 6
#define NODE_CFG_LOOKBACK 7
#define NODE_CFG_SLEEPTIME 8
#define NODE_CFG_TRIGGER 9
#define NODE_CFG_PORT 10
#define NODE_CFG_TRIM 11

#define NODE_CFG_PID 11
#define NODE_CFG_PPID 12

class node {

	std::string _name, _shortdesc, _longdesc, _ipaddress, _username, _password, _privatekey, _bucket, _pickpath[5], _droppath, _filemask, _ignore, _remmask, _transform, _exec, _prerun, _postrun, _domain;
	int _conftype, _status, _proto, _mode, _compress, _lookback, _sleeptime, _trim, _trigger;

	sqlite3 *_db;
	
	bool isdead, _canrun, _running;
	int *serial;
	int _pid, _ppid;

public:

	node();
	~node();
	// node(const node &);

	std::string operator[] (char i);
	int operator[] (int i);

	std::string &set(char);
	int &set(int);

	int reset();
	int disable();
	int kill();

	int monitor();
	int run();
	int running() { return _running; }
};


/* **** *
 * **** */

class nodes {

	std::vector<node *> _node;
	bool _canrun;
	bool _running;

public:
	nodes();
	~nodes();
	int reset();
	int load(config_t);
	int push(node &);
	int killall();

	bool running();
	bool init();
};

#endif