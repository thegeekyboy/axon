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

	void reset();
	void load(axon::config&);
	void push(node &);

	bool set(axon::log&);
	bool set(dbconf&);

	int killall();

	bool running();
	bool init();

	void print();
};

#endif