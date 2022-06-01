#ifndef CLUSTER_H
#define CLUSTER_H

#define MAX_NODE 32

class cluster {

	std::vector<node *> _node;
	bool _canrun;
	bool _running;

	axon::log *_log, dummy;

	struct dbconf _dbc;

public:
	cluster();
	~cluster();

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
	bool pool();
};

#endif