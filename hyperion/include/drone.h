#ifndef DRONE_H
#define DRONE_H
#include <axon/connection.h>
class drone {

	std::atomic_flag _busy = ATOMIC_FLAG_INIT;
	int _serial;

	std::shared_ptr<axon::transport::transfer::connection> _source = nullptr, _destination = nullptr; // need to change it to unique_ptr
	std::shared_ptr<axon::database::interface> _db;

	axon::log *_log, dummy;
	struct dbconf *_dbc;
	node *_node;

	std::thread _th;
	bool _enabled = false;

	std::string _speed(long long, long);

public:
	drone() = delete;
	drone(node &);
	~drone();

	bool get(std::string&);
	bool put(std::string&);

	void start();
	void stop();
	bool busy();

	void set(axon::log* l) { _log = l; };
	void set(std::shared_ptr<axon::database::interface> p) { _db = p; };
	void set(struct dbconf *p) { _dbc = p; };
	void set(int i) { _serial = i; };

	std::shared_ptr<axon::transport::transfer::connection> source() const { return _source; }
	std::shared_ptr<axon::transport::transfer::connection> destination() const { return _destination; }
};

#endif