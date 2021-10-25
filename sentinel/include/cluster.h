#ifndef CLUSTER_H
#define CLUSTER_H

#define MAX_NODE 32

class cluster {

	std::vector<node *> _node;
	bool _canrun;
	bool _running;

	axon::log *_log, dummy;

public:
	cluster();
	~cluster();

	void reset();
	void load(axon::config&);
	void push(node &);

	bool set(axon::log&);

	int killall();

	bool running();
	bool init();
};

#endif