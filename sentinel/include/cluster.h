#ifndef CLUSTER_H
#define CLUSTER_H

#define MAX_NODE 32

#define CLUSTER_CFG_BUFFER 'A'

class cluster {

	bool _canrun, _running;

	std::vector<node *> _node;
	std::string _buffer;

	axon::log *_log, dummy;

	struct dbconf _dbc;

	uid_t _uid;

public:
	cluster();
	~cluster();

	bool set(char, std::string);
	bool set(axon::log&);
	bool set(dbconf&);
	unsigned int size();
	void print();
	
	void reset();
	void load(axon::config&);
	void push(node &);

	int killall();
	bool reload();

	bool running();
	bool start();
	bool start(uid_t&);
	bool pool();
};

#endif