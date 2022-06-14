#include <sys/fsuid.h>

#include <boost/regex.hpp>

#include <axon.h>
#include <axon/config.h>
#include <axon/log.h>

#include <main.h>
#include <cluster.h>

cluster::cluster()
{
	_canrun = true;
	_running = false;
	_log = &dummy;
	_uid = 0;
	reset();
}

cluster::~cluster()
{
	_log->print("INFO", "cluster clearing memory..");
	while(!_node.empty())
	{
		node *temp = _node.back();
		delete temp;
		_node.pop_back();
	}
	_log->print("INFO", "cluster shutting down now.. good bye!");
}

void cluster::reset()
{
	while(!_node.empty())
	{
		node *temp = _node.back();
		delete temp;
		_node.pop_back();
	}
}

bool cluster::set(char i, std::string value)
{
	if (i == CLUSTER_CFG_BUFFER)
		_buffer = value;

	return true;
}

void cluster::load(axon::config &cfg)
{
	for (int i = 0; i < cfg.size(); i++)
	{
		node *tnode = new node;
		std::string stemp;
		std::string name;
		int tint;

		name = cfg.name(i);
		if (name.size() > 8) name.erase(8, std::string::npos);
		for (auto & c: name) c = std::toupper(c);

		_log->print("INFO", "%s - loading configuration parameters", name);
		
		if (_log != &dummy)
			tnode->set(*_log);
		
		if (_dbc.path.size() > 0 || _dbc.address.size() > 0 )
			tnode->set(_dbc);

		tnode->set(NODE_CFG_NAME, name);
		tnode->set(NODE_CFG_BUFFER, _buffer);

		cfg.open(cfg.name(i));

		try {
			cfg.get("status", tint);
			tnode->set(NODE_CFG_STATUS, tint);

			if (!(tint >= 0 && tint <= 1))
			{
				_log->print("ERROR", "%s - parameter [status] value is unacceptable; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}

		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [status] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("conftype", tint);
			tnode->set(NODE_CFG_CONFTYPE, tint);

			if (!(tint >= 0 && tint <= 3))
			{
				_log->print("ERROR", "%s - parameter [conftype] value is unacceptable; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}

		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [conftype] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("shortdesc", stemp);
			tnode->set(NODE_CFG_SHORTDESC, stemp);
		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [shortdesc] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("longdesc", stemp);
			tnode->set(NODE_CFG_LONGDESC, stemp);
		} catch (axon::exception &e) {
			// ignore
		}

		try {
			cfg.get("proto", tint);
			tnode->set(NODE_CFG_PROTOCOL, tint);

			if (!(tint >= 0 && tint <= 3))
			{
				_log->print("ERROR", "%s - parameter [conftype] value is unacceptable; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}

		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [proto] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("mode", tint);
			tnode->set(NODE_CFG_MODE, tint);

			if (!(tint >= 0 && tint <= 1) && (*tnode)[NODE_CFG_PROTOCOL] == 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value for [conftype] not acceptable");

		} catch (axon::exception &e) {

			if ((*tnode)[NODE_CFG_PROTOCOL] == 0)
			{
				_log->print("ERROR", "%s - mandatory parameter [mode] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("auth", tint);
			tnode->set(NODE_CFG_AUTH, tint);

			if (!(tint >= 0 && tint <= 1) && (*tnode)[NODE_CFG_PROTOCOL] == 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value for [auth] not acceptable");

		} catch (axon::exception &e) {

			if ((*tnode)[NODE_CFG_PROTOCOL] == 0)
			{
				_log->print("ERROR", "%s - mandatory parameter [auth] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("ipaddress", stemp);
			tnode->set(NODE_CFG_IPADDRESS, stemp);
		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [ipadress] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("port", tint);
			tnode->set(NODE_CFG_PORT, tint);

		} catch (axon::exception &e) {

			switch((*tnode)[NODE_CFG_PROTOCOL])
			{
				case 0:
					tnode->set(NODE_CFG_PORT, 22);
					break;
				case 1:
					tnode->set(NODE_CFG_PORT, 23);
					break;
			}
		}

		try {
			cfg.get("username", stemp);
			tnode->set(NODE_CFG_USERNAME, stemp);
		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [username] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("password", stemp);
			tnode->set(NODE_CFG_PASSWORD, stemp);
		} catch (axon::exception &e) {

			if ((*tnode)[NODE_CFG_AUTH] != 1)
			{
				_log->print("ERROR", "%s - mandatory parameter [password] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("privkey", stemp);
			tnode->set(NODE_CFG_PRIVATE_KEY, stemp);
		} catch (axon::exception &e) {
			if ((*tnode)[NODE_CFG_PROTOCOL] == 0 && (*tnode)[NODE_CFG_AUTH] == 1)
			{
				_log->print("ERROR", "%s - mandatory parameter [privkey] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("domain", stemp);
			tnode->set(NODE_CFG_DOMAIN, stemp);
		} catch (axon::exception &e) {
			if ((*tnode)[NODE_CFG_PROTOCOL] == 3)
			{
				_log->print("ERROR", "%s - mandatory parameter [domain] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("pickpath", stemp);
			tnode->set(NODE_CFG_PICKPATH, stemp);
		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [pickpath] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("droppath", stemp);
			tnode->set(NODE_CFG_DROPPATH, stemp);
		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [droppath] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("filemask", stemp);
			const boost::regex mask(stemp);
			tnode->set(NODE_CFG_FILEMASK, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		} catch (const boost::regex_error& e) {
			_log->print("ERROR", "%s - parameter [filemask] needs to be a valid regular expression; %s is invalid; disabling [%s]", name, stemp, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("remmask", stemp);
			tnode->set(NODE_CFG_REMMASK, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		}

		try {
			cfg.get("transform", stemp);
			tnode->set(NODE_CFG_TRANSFORM, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		}

		try {
			cfg.get("ignore", stemp);
			tnode->set(NODE_CFG_IGNORE, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		}

		try {
			cfg.get("compress", tint);
			tnode->set(NODE_CFG_COMPRESS, tint);

			if (!(tint >= 0 && tint <= 1))
			{
				_log->print("ERROR", "%s - parameter [compress] value is unacceptable; disabling [%s] compressing.", name, name);
				tnode->set(NODE_CFG_COMPRESS, 0);
			}

		} catch (axon::exception &e) {
			tnode->set(NODE_CFG_COMPRESS, 0);
		}

		try {
			cfg.get("lookback", tint);
			tnode->set(NODE_CFG_LOOKBACK, tint);

			if (tint <= 5)
			{
				_log->print("ERROR", "%s - parameter [lookback] value is unacceptable; disabling [%s].", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}

		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [lookback] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("sleeptime", tint);
			tnode->set(NODE_CFG_SLEEPTIME, tint);

			if (tint <= 5)
			{
				_log->print("ERROR", "%s - parameter [sleeptime] value is unacceptable; disabling [%s].", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}

		} catch (axon::exception &e) {
			_log->print("ERROR", "%s - mandatory parameter [sleeptime] missing; disabling [%s]", name, name);
			tnode->set(NODE_CFG_STATUS, 0);
		}

		try {
			cfg.get("trigger", tint);
			tnode->set(NODE_CFG_TRIGGER, tint);

		} catch (axon::exception &e) {
			if ((*tnode)[NODE_CFG_CONFTYPE] == 2)
			{
				_log->print("ERROR", "%s - mandatory parameter [domain] missing; disabling [%s]", name, name);
				tnode->set(NODE_CFG_STATUS, 0);
			}
		}

		try {
			cfg.get("prerun", stemp);
			tnode->set(NODE_CFG_PRERUN, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		}

		try {
			cfg.get("postrun", stemp);
			tnode->set(NODE_CFG_POSTRUN, stemp);
		} catch (axon::exception &e) {
			// ignore if missing
		}

		_node.push_back(tnode);
		cfg.close();
	}
}

void cluster::push(node &item)
{
	// node *elm = new node(item);
	// _node.push_back(item);
}

bool cluster::set(axon::log &log)
{
	_log = &log;
	return true;
}

bool cluster::set(dbconf &dbc)
{
	_dbc = dbc;
	return true;
}

unsigned int cluster::size()
{
	return _node.size();
}

int cluster::killall()
{
	_running = false;
	
	for (unsigned int i = 0; i < _node.size(); i++)
	{
		(*_node[i]).kill();
	}

	return true;
}

bool cluster::reload()
{
	for (unsigned int i = 0; i < _node.size(); i++)
	{
		(*_node[i]).kill();
	}

	while (_node.size() > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return true;
}

bool cluster::running()
{
	return _running;
}

bool cluster::start(uid_t &uid)
{
	_uid = uid;
	setfsuid(uid);
	setfsgid(uid);
	return start();
}

bool cluster::start()
{
	_log->print("INFO", "cluster monitor initializing..");

	for (unsigned int i = 0; i < _node.size(); i++)
	{
		// th = std::thread(&node::monitor, std::ref(_node[i]));
		// th.detach();
		try {
			_node[i]->start();
		} catch (axon::exception &e) {
			_log->print("CLUSTER", "Exception: %s", e.what());
		}
	}

	return true;
}

bool cluster::pool()
{
	_running = true;

	while (_node.size() > 0 || _running)
	{
		for (unsigned int i = 0; i < _node.size(); i++)
		{
			if (!_node[i]->enabled() && !_node[i]->running())
			{
				_log->print("INFO", "waiting %s to die", _node[i]->get(NODE_CFG_NAME));
				_node[i]->wait();
				delete _node[i];
				_node.erase(_node.begin()+i);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}

	return true;
}

void cluster::print()
{
	unsigned short nsz = 0;
	
	for (unsigned int i = 0; i < _node.size(); i++)
	{
		if (_node[i]->get(NODE_CFG_NAME).size() > nsz)
			nsz = _node[i]->get(NODE_CFG_NAME).size();
	}

	for (unsigned int i = 0; i < _node.size(); i++)
		_node[i]->print(nsz);
}