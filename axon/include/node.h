#ifndef AXON_NODE_H_
#define AXON_NODE_H_

#define MAX_NODE 32

#define NODE_CFG_NAME 'O'
#define NODE_CFG_SHORTDESC 'A'
#define NODE_CFG_LONGDESC 'B'
#define NODE_CFG_IPADDRESS 'C'
#define NODE_CFG_USERNAME 'D'
#define NODE_CFG_PASSWORD 'E'
#define NODE_CFG_PICKPATH 'F'
#define NODE_CFG_DROPPATH 'G'
#define NODE_CFG_FILEMASK 'H'
#define NODE_CFG_IGNORE 'I'
#define NODE_CFG_REMMASK 'J'
#define NODE_CFG_TRANSFORM 'K'
#define NODE_CFG_EXEC 'L'
#define NODE_CFG_PRERUN 'M'
#define NODE_CFG_POSTRUN 'N'

#define NODE_CFG_CONFTYPE 1
#define NODE_CFG_STATUS 2
#define NODE_CFG_PROTOCOL 3
#define NODE_CFG_MODE 4
#define NODE_CFG_COMPRESS 5
#define NODE_CFG_LOOKBACK 6
#define NODE_CFG_SLEEPTIME 7
#define NODE_CFG_TRIM 8
#define NODE_CFG_TRIGGER 9
#define NODE_CFG_PID 10
#define NODE_CFG_PPID 11

class node {

	
	std::string _name, _shortdesc, _longdesc, _ipaddress, _username, _password, _pickpath[5], _droppath, _filemask, _ignore, _remmask, _transform, _exec, _prerun, _postrun;
	sqlite3 *_db;
	bool isdead, _canrun, _running;
	int *num;

public:
	int _conftype, _status, _proto, _mode, _compress, _lookback, _sleeptime, _trim, _trigger, _pid, _ppid;

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