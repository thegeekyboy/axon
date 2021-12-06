#include <main.h>

#include <algorithm>
#include <string>
#include <chrono>
#include <ctime>
#include <boost/regex.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <axon/connection.h>
#include <axon/ssh.h>
#include <axon/socket.h>
#include <axon/ftp.h>
#include <axon/file.h>
#include <axon/util.h>

node::node()
{
	//lg.print("DBUG", "%s - node class constructing", _name);
	_canrun = true;
	_running = false;
	_sleeping = false;

	reset();
}

node::~node()
{
	_log->print("INFO", "%s - process terminating", _name);
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

std::string node::get(char i)
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

int node::get(int i)
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

bool node::set(char i, std::string value)
{
	if (i == NODE_CFG_NAME)
		_name = value;
	else if (i == NODE_CFG_SHORTDESC)
		_shortdesc = value;
	else if (i == NODE_CFG_LONGDESC)
		_longdesc = value;
	else if (i == NODE_CFG_IPADDRESS)
		_ipaddress = value;
	else if (i == NODE_CFG_USERNAME)
		_username = value;
	else if (i == NODE_CFG_PASSWORD)
		_password = value;
	else if (i == NODE_CFG_PICKPATH)
		_pickpath[0] = value;
	else if (i == NODE_CFG_DROPPATH)
		_droppath = value;
	else if (i == NODE_CFG_FILEMASK)
		_filemask = value;
	else if (i == NODE_CFG_IGNORE)
		_ipaddress = value;
	else if (i == NODE_CFG_REMMASK)
		_remmask = value;
	else if (i == NODE_CFG_TRANSFORM)
		_transform = value;
	else if (i == NODE_CFG_EXEC)
		_exec = value;
	else if (i == NODE_CFG_PRERUN)
		_prerun = value;
	else if (i == NODE_CFG_POSTRUN)
		_postrun = value;
	else if (i == NODE_CFG_PRIVATE_KEY)
		_privatekey = value;

	return true;
}

bool node::set(int i, int value)
{
	if (i == NODE_CFG_CONFTYPE)
		_conftype = value;
	else if (i == NODE_CFG_STATUS)
		_status = value;
	else if (i == NODE_CFG_PROTOCOL)
		_proto = value;
	else if (i == NODE_CFG_MODE)
		_mode = value;
	else if (i == NODE_CFG_AUTH)
		_auth = value;
	else if (i == NODE_CFG_COMPRESS)
		_compress = value;
	else if (i == NODE_CFG_LOOKBACK)
		_lookback = value;
	else if (i == NODE_CFG_SLEEPTIME)
		_sleeptime = value;
	else if (i == NODE_CFG_TRIM)
		_trim = value;
	else if (i == NODE_CFG_TRIGGER)
		_trigger = value;
	else if (i == NODE_CFG_PID)
		_pid = value;
	else if (i == NODE_CFG_PPID)
		_ppid = value;

	return true;
}

bool node::set(axon::log &log)
{
	_log = &log;
	return true;
}

bool node::set(dbconf &dbc)
{
	_dbc = dbc;
	return true;
}

int node::reset()
{
	_conftype = -1;
	_status = -1;
	_proto = -1;
	_mode = -1;
	_auth = -1;
	_compress = -1;
	_lookback = -1;
	_sleeptime = -1;
	_trim = -1;
	_trigger = -1;
	
	_pid = -1;
	_ppid = -1;

	_name = "";
	_shortdesc = "";
	_longdesc = "";
	_ipaddress = "";
	_username = "";
	_password = "";
	_privatekey = "";
	_bucket = "";
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
	_domain = "";

	isdead = false;

	return true;
}

int node::disable()
{
	return _status = 0;
}

bool node::kill()
{
	_log->print("WARN", "%s - received kill request", _name);
	_canrun = false;
	_sleeping = false;
	_status = false;

	return true;
}

int node::sleep()
{
	int remaining = _sleeptime*5;

	_sleeping = true;
	while (_canrun && _sleeping && remaining > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		remaining--;
	}

	return remaining;
}

int node::monitor()
{
	_running = true;

	while (_status & _canrun)
	{
		run();

		_log->print("INFO", "%s - cycle completed going to sleep for [%d] seconds", _name, _sleeptime);
		sleep();
	}

	_log->print("INFO", "%s - all operations done, monitor exiting.", _name);
	_running = false;

	return true;
}

bool node::start()
{
	if (_status)
	{
		_db.open(_dbc.path+"/"+_name+".dbf");
		_db.execute("CREATE TABLE IF NOT EXISTS " + _dbc.gtt + " (LISTDATE DATE DEFAULT (STRFTIME('%s','NOW')), FILENAME VARCHAR(512) PRIMARY KEY, STATUS INT DEFAULT 0)");
		_db.execute("CREATE TABLE IF NOT EXISTS " + _dbc.list + " (LISTDATE DATE DEFAULT (STRFTIME('%s','NOW')), FILENAME VARCHAR(512) PRIMARY KEY, FILESIZE BIG INT, ELAPSDUR BIG INT, STATUS INT DEFAULT 0)");
		_th = std::thread(&node::monitor, this);
	}
	else
		_log->print("INFO", "%s - service is disabled.", _name);

	return true;
}
int blah(const axon::entry *e)
{
	std::cout<<e->name<<std::endl;
	return 0;
}
int node::run()
{
	try {
		axon::transport::transfer::file fp(_ipaddress, _username, _password);

		fp.filter(_filemask);
		fp.connect();
		fp.chwd(_pickpath[0]);
		// fp.ren("mark", "dork");
		// fp.del("dork");
		fp.list(&blah);
	} catch (axon::exception &e) {
		_log->print("DEBUG", std::string(e.what()));
	}
	
	return 0;
	// actual stuff is being done here :) welcome to the world of spaghetti coding

	std::shared_ptr<axon::transport::transfer::connection> conn;

	try {

		switch (_proto)
		{
			case 0:
				{
					std::shared_ptr<axon::transport::transfer::sftp> p(new axon::transport::transfer::sftp(_ipaddress, _username, _password));
					conn = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);

					if (_auth == 1)
					{
						p->set(AXON_TRANSFER_SSH_MODE, axon::transport::transfer::auth_methods::PRIVATEKEY);
						p->set(AXON_TRANSFER_SSH_PRIVATEKEY, _privatekey);
						std::cout<<"_privatekey = "<<_privatekey<<std::endl;
					}
				}
				break;

			case 1:
				{
					std::shared_ptr<axon::transport::transfer::ftp> p(new axon::transport::transfer::ftp(_ipaddress, _username, _password));
					conn = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			default:
				_log->print("ERROR", "%s - not a valid protocol selected %d", _name, _proto);
				break;
		}

	} catch (...) {
		_log->print("FATAL", "%s - error creating object", _name);
	}

	try {

		if (conn)
		{
			std::string pickpath, droppath, filemask;
			std::vector<axon::entry> v;
			int count = 0;
			char buffer[PATH_MAX];

			time_t xctime;
			time(&xctime);
			xctime -= (_lookback*60);
			struct tm *xtstamp = localtime(&xctime);
			
			strftime(buffer, PATH_MAX, _pickpath[0].c_str(), xtstamp); pickpath = buffer;
			strftime(buffer, PATH_MAX, _droppath.c_str(), xtstamp); droppath = buffer;
			strftime(buffer, 1023, _filemask.c_str(), xtstamp); filemask = buffer;

			_db.execute("DELETE FROM " + _dbc.gtt);

			conn->connect();
			conn->chwd(_pickpath[0]);

			if (conn->list(&v))
			{
				_db.execute("BEGIN TRANSACTION;");

				for (auto &elm : v)
					if (elm.flag == axon::flags::FILE)
					{
						try {
							_db.execute("INSERT INTO " + _dbc.gtt + "(FILENAME) VALUES (TRIM('" + elm.name + "'))");
						} catch (axon::exception &e) {
		
							_log->print("FATAL", e.what());
						}
						count++;
					}

				_db.execute("END TRANSACTION;");
			}

			_log->print("INFO", "%s - latest file get complete, processed %d of %d downloaded records.", _shortdesc, count, v.size());

			if (_ignore.size() > 0)
				_db.query("SELECT FILENAME FILENAME FROM (SELECT A.* FROM " + _dbc.gtt + " A LEFT JOIN " + _dbc.list + " B ON A.FILENAME = B.FILENAME WHERE B.FILENAME IS NULL) A LEFT JOIN DAT_FILEBOT_FILELIST B ON REPLACE(A.FILENAME, '" + _ignore + "', '') = B.FILENAME WHERE B.FILENAME IS NULL");
			else
				_db.query("SELECT FILENAME FILENAME FROM (SELECT A.FILENAME FILENAME FROM " + _dbc.gtt + " A LEFT JOIN " + _dbc.list + " B ON A.FILENAME = B.FILENAME WHERE B.FILENAME IS NULL AND A.FILENAME IS NOT NULL)");

			if (_prerun.size() > 0)
			{
				std::string prerun = _prerun;

				boost::replace_all(prerun, "%PATH%", droppath.c_str());

				_log->print("INFO", "%s - running pre-script (%s)", _shortdesc, prerun.c_str());
				
				if (axon::execmd(prerun.c_str(), _name.c_str()))
				{
					_log->print("DEBUG", "%s - successfully completed running pre-script", _name);
				}
				else
				{
					_log->print("ERROR", "%s - there was an error running the pre-script", _name);
				}
			}

			count = 0;
			std::vector<std::string> list;
			while (_db.next())
			{
				list.push_back(_db.get(0));
				count++;
			}
			_db.done();

			if (count > 0)
			{
				_log->print("INFO", "%s - preparing to down %d files...", _name, count);

				for (int current = 0; current < count; current++)
				{
					char sqltext[4096];
					auto start = std::chrono::steady_clock::now();
					
					const boost::regex mask(filemask);

					if (boost::regex_match(list[current], mask))
					{

						long long filesize = conn->get(list[current], list[current], true);
						
						_log->print("INFO", "%s - [%d/%d] Downloaded file %s, Size: %d, Speed: %.2fkb/sec", _name, current, count, list[current].c_str(), filesize, ((filesize/1000.00)/(since(start).count()/1000.00)));

						sprintf(sqltext, "INSERT INTO %s (LISTDATE,FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES(%lu,'%s',%lld, %ld, %d);", _dbc.list.c_str(), std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count(), list[current].c_str(), filesize, since(start).count(), 1);
						_db.execute(sqltext);
					}
					else
					{
						sprintf(sqltext, "INSERT INTO %s (LISTDATE,FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES(%lu,'%s',%lld, %ld, %d);", _dbc.list.c_str(), std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count(), list[current].c_str(), 0ULL, 0L, 2);
						_db.execute(sqltext);
					}
				}

				if (_postrun.size() > 0)
				{
					boost::replace_all(_postrun, "%PATH%", droppath.c_str());
					_log->print("INFO", "%s - running post-script (%s)", _name, _postrun);
					
					if (axon::execmd(_postrun.c_str(), _name.c_str()))
					{
						_log->print("DEBUG", "%s - successfully completed running post-script", _name);
					}
					else
						_log->print("ERROR", "%s - there was an error running the post-script", _name);
				}
			}
			else
				_log->print("INFO", "%s - no new files to download", _name);

			conn->disconnect();
		}
	} catch (axon::exception &e) {
		_log->print("ERROR", "%s - %s", _name, e.what());
	}

	return true;
}

bool node::running()
{
	return _running;
}

bool node::wait()
{
	if (_th.joinable())
		_th.join();
	else
		return false;

	return true;
}

void node::print(int width)
{
	if (_status == 0)
		return;

	std::string padding = "                                                    ";
	std::cout<<std::right<<std::setw(width)<<_name.substr(0, width)<<"─┬────"<<_shortdesc<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          description ── "<<_longdesc<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─               ip/url ── "<<_ipaddress<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             username ── "<<_username<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             password ── "<<_password<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           public key ── "<<_privatekey<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─            S3 Bucket ── "<<_bucket<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          pickup path ── "<<_pickpath[0]<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─            drop path ── "<<_droppath<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          filter mask ── "<<_filemask<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          ignore mask ── "<<_ignore<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           alter name ── "<<_remmask<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─       name transform ── "<<_transform<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           run script ── "<<_exec<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─  pre-download script ── "<<_prerun<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─ post-download script ── "<<_postrun<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─               domain ── "<<_domain<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─   configuration type ── "<<_conftype<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─               status ── "<<_status<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             protocol ── "<<_proto<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─                 mode ── "<<_mode<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─  authentication type ── "<<_auth<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          compression ── "<<_compress<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─       look back time ── "<<_lookback<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           sleep time ── "<<_sleeptime<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─       trim extension ── "<<_trim<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"└─         trigger time ── "<<_trigger<<std::endl<<std::endl;;
}