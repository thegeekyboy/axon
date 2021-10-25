#include <main.h>

node::node()
{
	//lg.print("DBUG", "%s - node class constructing", _name);
	_canrun = true;
	_running = false;

	reset();
}

node::~node()
{
	//lg.print("DBUG", "%s - node class destructing", _name);
}

std::string node::operator[](char i)
{
	if (i == NODE_CFG_NAME)
		return _name;
	else if (i == NODE_CFG_SHORTDESC)
		return _shortdesc;
	else if (i == NODE_CFG_LONGDESC)
		return _longdesc;
	else if (i == NODE_CFG_IPADDRESS)
		return _ipaddress;
	else if (i == NODE_CFG_USERNAME)
		return _username;
	else if (i == NODE_CFG_PASSWORD)
		return _password;
	else if (i == NODE_CFG_PICKPATH)
		return _pickpath[0];
	else if (i == NODE_CFG_DROPPATH)
		return _droppath;
	else if (i == NODE_CFG_FILEMASK)
		return _filemask;
	else if (i == NODE_CFG_IGNORE)
		return _ipaddress;
	else if (i == NODE_CFG_REMMASK)
		return _remmask;
	else if (i == NODE_CFG_TRANSFORM)
		return _transform;
	else if (i == NODE_CFG_EXEC)
		return _exec;
	else if (i == NODE_CFG_PRERUN)
		return _prerun;
	else if (i == NODE_CFG_POSTRUN)
		return _postrun;

	return 0;
}

int node::operator[] (int i)
{
	if (i == NODE_CFG_CONFTYPE)
		return _conftype;
	else if (i == NODE_CFG_STATUS)
		return _status;
	else if (i == NODE_CFG_PROTOCOL)
		return _proto;
	else if (i == NODE_CFG_MODE)
		return _mode;
	else if (i == NODE_CFG_COMPRESS)
		return _compress;
	else if (i == NODE_CFG_LOOKBACK)
		return _lookback;
	else if (i == NODE_CFG_SLEEPTIME)
		return _sleeptime;
	else if (i == NODE_CFG_TRIM)
		return _trim;
	else if (i == NODE_CFG_TRIGGER)
		return _trigger;
	else if (i == NODE_CFG_PID)
		return _pid;
	else if (i == NODE_CFG_PPID)
		return _ppid;

	return -1;
}

std::string &node::set(char i)
{
	if (i == NODE_CFG_NAME)
		return _name;
	else if (i == NODE_CFG_SHORTDESC)
		return _shortdesc;
	else if (i == NODE_CFG_LONGDESC)
		return _longdesc;
	else if (i == NODE_CFG_IPADDRESS)
		return _ipaddress;
	else if (i == NODE_CFG_USERNAME)
		return _username;
	else if (i == NODE_CFG_PASSWORD)
		return _password;
	else if (i == NODE_CFG_PICKPATH)
		return _pickpath[0];
	else if (i == NODE_CFG_DROPPATH)
		return _droppath;
	else if (i == NODE_CFG_FILEMASK)
		return _filemask;
	else if (i == NODE_CFG_IGNORE)
		return _ipaddress;
	else if (i == NODE_CFG_REMMASK)
		return _remmask;
	else if (i == NODE_CFG_TRANSFORM)
		return _transform;
	else if (i == NODE_CFG_EXEC)
		return _exec;
	else if (i == NODE_CFG_PRERUN)
		return _prerun;
	else if (i == NODE_CFG_POSTRUN)
		return _postrun;
}

int &node::set(int i)
{
	if (i == NODE_CFG_CONFTYPE)
		return _conftype;
	else if (i == NODE_CFG_STATUS)
		return _status;
	else if (i == NODE_CFG_PROTOCOL)
		return _proto;
	else if (i == NODE_CFG_MODE)
		return _mode;
	else if (i == NODE_CFG_COMPRESS)
		return _compress;
	else if (i == NODE_CFG_LOOKBACK)
		return _lookback;
	else if (i == NODE_CFG_SLEEPTIME)
		return _sleeptime;
	else if (i == NODE_CFG_TRIM)
		return _trim;
	else if (i == NODE_CFG_TRIGGER)
		return _trigger;
	else if (i == NODE_CFG_PID)
		return _pid;
	else if (i == NODE_CFG_PPID)
		return _ppid;
}

int node::reset()
{
	_conftype = 0;
	_status = 0;
	_proto = 0;
	_mode = 0;
	_compress = 0;
	_lookback = 0;
	_sleeptime = 0;
	_trim = 0;
	_trigger = 0;
	_pid = 0;
	_ppid = 0;

	_name = "";
	_shortdesc = "";
	_longdesc = "";
	_ipaddress = "";
	_username = "";
	_password = "";
	_pickpath[0] = "";
	_pickpath[1] = "";
	_pickpath[2] = "";
	_pickpath[3] = "";
	_pickpath[4] = "";
	_droppath = "";
	_filemask = "";
	_ignore = "";
	_remmask = "";
	_transform = "";
	_exec = "";
	_prerun = "";
	_postrun = "";

	isdead = false;
	// if (_db)
	// 	sqlite3_close(_db);
	// _db = NULL;

	return true;
}

int node::disable()
{
	_status = 0;
}

int node::kill()
{
	//lg.print("DBUG", "%s - Caught kill request!", _name);
}

int node::monitor()
{
	int x = (rand() % 10)+2;
	_running = true;

	//lg.print("DBUG", "%s - Sleeping for %d", _name, x);
	sleep(x);
	//lg.print("DBUG", "%s - Thread Completed in %d", _name, x);

	_running = false;
	return true;
}

int node::run()
{

	return true;
}

/* ******************************************************
 *
 * Node Collection Manager
 *
 * ******************************************************/
nodes::nodes()
{
	_canrun = true;
	_running = false;
	reset();
}

nodes::~nodes()
{
	while(!_node.empty())
	{
		node *temp = _node.close();
		delete temp;
		_node.pop_back();
	}
}

int nodes::reset()
{
	while(!_node.empty())
	{
		node *temp = _node.close();
		delete temp;
		_node.pop_back();
	}
}

int nodes::load(config_t cfg)
{
	config_setting_t *setting;

	if((setting = config_lookup(&cfg, "nodes")) != NULL)
	{
		int cnt = config_setting_length(setting);

		for(int i = 0; i < cnt; i++)
		{
			bool failed = false;
			node *temp = new node;
			node &elm = *temp;

			const char *shortdesc, *longdesc, *ipaddress, *username, *password, *pickpath, *droppath, *filemask, *ignore, *remmask, *transform, *setname, *exec, *prerun, *postrun;

			config_setting_t *nodes = config_setting_get_elem(setting, i);

			if ((setname = config_setting_name(nodes)) != NULL)	
				elm.set(NODE_CFG_NAME) = setname;

			if (!config_setting_lookup_int(nodes, "status", &elm.set(NODE_CFG_STATUS)))
			{ 
				//lg.print("EROR", "%s - Mandatory configuration missing 'status', disabling...", setname);
				elm.disable();
			}

			if (!config_setting_lookup_int(nodes, "conftype", &elm.set(NODE_CFG_CONFTYPE)))
			{
				//lg.print("EROR", "%s - Mandatory configuration missing 'conftype', disabling...", setname);
				elm.disable();
				
			}

			if (!config_setting_lookup_int(nodes, "proto", &elm.set(NODE_CFG_PROTOCOL)))
			{
				//lg.print("EROR", "%s - Mandatory configuration missing 'proto', disabling...", setname);
				failed = true;
			}

			config_setting_lookup_int(nodes, "mode", &elm.set(NODE_CFG_MODE));
			config_setting_lookup_int(nodes, "compress", &elm.set(NODE_CFG_COMPRESS));
			config_setting_lookup_int(nodes, "lookback", &elm.set(NODE_CFG_LOOKBACK));
			config_setting_lookup_int(nodes, "trim", &elm.set(NODE_CFG_TRIM));
			
			if (!config_setting_lookup_int(nodes, "sleeptime", &elm.set(NODE_CFG_SLEEPTIME)))
			{
				//lg.print("EROR", "%s - Mandatory configuration missing 'sleeptime', disabling...", setname);
				elm.disable();
			}
			
			if (!config_setting_lookup_int(nodes, "trigger", &elm.set(NODE_CFG_TRIGGER)))
			{
				if (elm[NODE_CFG_CONFTYPE] == 2)
				{
					//lg.print("EROR", "%s - Mandatory configuration missing 'trigger', disabling...", setname);
					elm.disable();
				}
			}

			if (config_setting_lookup_string(nodes, "shortdesc", &shortdesc))
				elm.set(NODE_CFG_SHORTDESC) = shortdesc;
			else {
				//lg.print("EROR", "%s - Mandatory configuration missing 'shortdesc', disabling...", setname); 
				elm.disable();
			}

			if (config_setting_lookup_string(nodes, "longdesc", &longdesc))
				elm.set(NODE_CFG_LONGDESC) = longdesc;
			
			if (config_setting_lookup_string(nodes, "ipaddress", &ipaddress))
				elm.set(NODE_CFG_IPADDRESS) = ipaddress;
			else {
				
				//lg.print("EROR", "%s - Mandatory configuration missing 'ipaddress', disabling...", setname);
				elm.disable();
			}
			
			if (config_setting_lookup_string(nodes, "username", &username))
				elm.set(NODE_CFG_USERNAME) = username;
			else {

				//lg.print("EROR", "%s - Mandatory configuration missing 'username', disabling...", setname); 
				elm.disable();
			}

			if (config_setting_lookup_string(nodes, "password", &password))
				elm.set(NODE_CFG_PASSWORD) = password;
			else {

				//lg.print("EROR", "%s - Mandatory configuration missing 'password', disabling...", setname); 
				elm.disable();
			}
			
			if (config_setting_lookup_string(nodes, "pickpath", &pickpath))
				elm.set(NODE_CFG_PICKPATH) = pickpath;
			else {

				//lg.print("EROR", "%s - Mandatory configuration missing 'pickpath', disabling...", setname);
				elm.disable();
			}
			// if (config_setting_lookup_string(nodes, "pickpath2", &pickpath)) strcpy(_inst[i].pickpath2, pickpath);
			// if (config_setting_lookup_string(nodes, "pickpath3", &pickpath)) strcpy(_inst[i].pickpath3, pickpath);
			// if (config_setting_lookup_string(nodes, "pickpath4", &pickpath)) strcpy(_inst[i].pickpath4, pickpath);
			// if (config_setting_lookup_string(nodes, "pickpath5", &pickpath)) strcpy(_inst[i].pickpath5, pickpath);
			
			if (config_setting_lookup_string(nodes, "droppath", &droppath))
				elm.set(NODE_CFG_DROPPATH) = droppath;
			else {

				//lg.print("EROR", "%s - Mandatory configuration missing 'droppath', disabling...", setname);
				elm.disable();
			}
			
			if (config_setting_lookup_string(nodes, "filemask", &filemask))
				elm.set(NODE_CFG_FILEMASK) = filemask;
			else {

				//lg.print("EROR", "%s - Mandatory configuration missing 'filemask', disabling...", setname);
				elm.disable();
			}
			
			if (config_setting_lookup_string(nodes, "ignore", &ignore)) elm.set(NODE_CFG_IGNORE) = ignore;
			if (config_setting_lookup_string(nodes, "remmask", &remmask)) elm.set(NODE_CFG_REMMASK) = remmask;
			if (config_setting_lookup_string(nodes, "transform", &transform)) elm.set(NODE_CFG_TRANSFORM) = transform;
			if (config_setting_lookup_string(nodes, "exec", &exec)) elm.set(NODE_CFG_EXEC) = exec;
			if (config_setting_lookup_string(nodes, "prerun", &prerun)) elm.set(NODE_CFG_PRERUN) = prerun;
			if (config_setting_lookup_string(nodes, "postrun", &postrun)) elm.set(NODE_CFG_POSTRUN) = postrun;

			if (elm[NODE_CFG_STATUS] == 0)
			{

				//lg.print("INFO", "%s - Node is disabled.", elm[NODE_CFG_NAME]);
			} else {

				//lg.print("INFO", "%s - Node is enabled.", elm[NODE_CFG_NAME]);
				_node.push_back(temp);
			}
		}
	}
}

int nodes::push(node &item)
{
	//node *elm = new node(item);
	//_node.push_back(item);
}

int nodes::killall()
{
	for (int i = 0; i < _node.size(); i++)
	{
		(*_node[i]).kill();
	}

	return true;
}

bool nodes::running()
{
	return _running;
}

bool nodes::init()
{
	std::thread th;
	int cnt = 0, thr = 0;

	for (int i = 0; i < _node.size(); i++)
	{
		th = std::thread(&node::monitor, std::ref(_node[i]));
		th.detach();

		cnt++;
	}

	while (cnt>0)
	{
		cnt = 0;

		for (int i = 0; i < _node.size(); i++)
		{
			if (_node[i]->running())
				cnt++;
		}

		//lg.print("XBG", "Running: %d processes.", thr);
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		sleep(0.2);
	}


	return true;
}