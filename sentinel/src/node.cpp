#include <main.h>
#include <node.h>

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
	else if (i == NODE_CFG_SRC_IPADDRESS)
		return _src_ipaddress;
	else if (i == NODE_CFG_SRC_USERNAME)
		return _src_username;
	else if (i == NODE_CFG_SRC_PASSWORD)
		return _src_password;
	else if (i == NODE_CFG_SRC_DOMAIN)
		return _src_domain;
	else if (i == NODE_CFG_DST_IPADDRESS)
		return _dst_ipaddress;
	else if (i == NODE_CFG_DST_USERNAME)
		return _dst_username;
	else if (i == NODE_CFG_DST_PASSWORD)
		return _dst_password;
	else if (i == NODE_CFG_DST_DOMAIN)
		return _dst_domain;
	else if (i == NODE_CFG_PICKPATH)
		return _pickpath[0];
	else if (i == NODE_CFG_DROPPATH)
		return _droppath;
	else if (i == NODE_CFG_FILEMASK)
		return _filemask;
	else if (i == NODE_CFG_IGNORE)
		return _ignore;
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
	else if (i == NODE_CFG_BUFFER)
		return _buffer;

	return 0;
}

int node::operator[] (int i)
{
	if (i == NODE_CFG_CONFTYPE)
		return _conftype;
	else if (i == NODE_CFG_STATUS)
		return _status;
	else if (i == NODE_CFG_SRC_PROTOCOL)
		return _src_protocol;
	else if (i == NODE_CFG_SRC_MODE)
		return _src_mode;
	else if (i == NODE_CFG_SRC_AUTH)
		return _src_auth;
	else if (i == NODE_CFG_DST_PROTOCOL)
		return _dst_protocol;
	else if (i == NODE_CFG_DST_MODE)
		return _dst_mode;
	else if (i == NODE_CFG_DST_AUTH)
		return _dst_auth;
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
	else if (i == NODE_CFG_SRC_IPADDRESS)
		return _src_ipaddress;
	else if (i == NODE_CFG_SRC_USERNAME)
		return _src_username;
	else if (i == NODE_CFG_SRC_PASSWORD)
		return _src_password;
	else if (i == NODE_CFG_SRC_DOMAIN)
		return _src_domain;
	else if (i == NODE_CFG_DST_IPADDRESS)
		return _dst_ipaddress;
	else if (i == NODE_CFG_DST_USERNAME)
		return _dst_username;
	else if (i == NODE_CFG_DST_PASSWORD)
		return _dst_password;
	else if (i == NODE_CFG_DST_DOMAIN)
		return _dst_domain;
	else if (i == NODE_CFG_PICKPATH)
		return _pickpath[0];
	else if (i == NODE_CFG_DROPPATH)
		return _droppath;
	else if (i == NODE_CFG_FILEMASK)
		return _filemask;
	else if (i == NODE_CFG_IGNORE)
		return _ignore;
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
	else if (i == NODE_CFG_BUFFER)
		return _buffer;

	return 0;
}

int node::get(int i)
{
	if (i == NODE_CFG_CONFTYPE)
		return _conftype;
	else if (i == NODE_CFG_STATUS)
		return _status;
	else if (i == NODE_CFG_SRC_PROTOCOL)
		return _src_protocol;
	else if (i == NODE_CFG_SRC_MODE)
		return _src_mode;
	else if (i == NODE_CFG_SRC_AUTH)
		return _src_auth;
	else if (i == NODE_CFG_DST_PROTOCOL)
		return _dst_protocol;
	else if (i == NODE_CFG_DST_MODE)
		return _dst_mode;
	else if (i == NODE_CFG_DST_AUTH)
		return _dst_auth;
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
	else if (i == NODE_CFG_SRC_IPADDRESS)
		_src_ipaddress = value;
	else if (i == NODE_CFG_SRC_USERNAME)
		_src_username = value;
	else if (i == NODE_CFG_SRC_PASSWORD)
		_src_password = value;
	else if (i == NODE_CFG_SRC_PRIVATE_KEY)
		_src_privatekey = value;
	else if (i == NODE_CFG_SRC_DOMAIN)
		_src_domain = value;
	else if (i == NODE_CFG_DST_IPADDRESS)
		_dst_ipaddress = value;
	else if (i == NODE_CFG_DST_USERNAME)
		_dst_username = value;
	else if (i == NODE_CFG_DST_PASSWORD)
		_dst_password = value;
	else if (i == NODE_CFG_DST_PRIVATE_KEY)
		_dst_privatekey = value;
	else if (i == NODE_CFG_DST_DOMAIN)
		_dst_domain = value;
	else if (i == NODE_CFG_PICKPATH)
		_pickpath[0] = value;
	else if (i == NODE_CFG_DROPPATH)
		_droppath = value;
	else if (i == NODE_CFG_FILEMASK)
		_filemask = value;
	else if (i == NODE_CFG_IGNORE)
		_ignore = value;
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
	else if (i == NODE_CFG_BUFFER)
		_buffer = value;

	return true;
}

bool node::set(int i, int value)
{
	if (i == NODE_CFG_CONFTYPE)
		_conftype = value;
	else if (i == NODE_CFG_STATUS)
		_status = value;
	else if (i == NODE_CFG_SRC_PROTOCOL)
		_src_protocol = value;
	else if (i == NODE_CFG_SRC_MODE)
		_src_mode = value;
	else if (i == NODE_CFG_SRC_AUTH)
		_src_auth = value;
	else if (i == NODE_CFG_DST_PROTOCOL)
		_dst_protocol = value;
	else if (i == NODE_CFG_DST_MODE)
		_dst_mode = value;
	else if (i == NODE_CFG_DST_AUTH)
		_dst_auth = value;
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
	_src_protocol = -1;
	_src_mode = -1;
	_src_auth = -1;
	_dst_protocol = -1;
	_dst_mode = -1;
	_dst_auth = -1;
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
	_src_ipaddress = "";
	_src_username = "";
	_src_password = "";
	_src_domain = "";
	_src_privatekey = "";
	_dst_ipaddress = "";
	_dst_username = "";
	_dst_password = "";
	_dst_domain = "";
	_dst_privatekey = "";
	_pickpath[0] = "";
	_pickpath[1] = "";
	_pickpath[2] = "";
	_pickpath[3] = "";
	_pickpath[4] = "";
	_droppath = "";
	_bucket = "";
	_filemask = "";
	_ignore = "";
	_remmask = "";
	_transform = "";
	_exec = "";
	_prerun = "";
	_postrun = "";

	isdead = false;

	return true;
}

void node::disable()
{
	_status = false;
}

void node::enable()
{
	_status = true;
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
		if (_dbc.type == 0)
		{
			std::shared_ptr<axon::database::sqlite> sqlite(new axon::database::sqlite());
			_db = std::dynamic_pointer_cast<axon::database::interface>(sqlite);

			_db->connect(_dbc.path+"/"+_name+".dbf", std::string("username"), std::string("password"));
			_db->execute("CREATE TABLE IF NOT EXISTS " + _dbc.gtt + " (LISTDATE DATE DEFAULT (STRFTIME('%s','NOW')), FILENAME VARCHAR(512) PRIMARY KEY, STATUS INT DEFAULT 0)");
			_db->execute("CREATE TABLE IF NOT EXISTS " + _dbc.list + " (LISTDATE DATE DEFAULT (STRFTIME('%s','NOW')), FILENAME VARCHAR(512) PRIMARY KEY, FILESIZE BIG INT, ELAPSDUR BIG INT, STATUS INT DEFAULT 0)");
		}
		else
		{
			std::shared_ptr<axon::database::oracle> oracle(new axon::database::oracle());
			_db = std::dynamic_pointer_cast<axon::database::interface>(oracle);

			_db->connect(_dbc.address, _dbc.username, _dbc.password);

			_dbc.list = _name + "_" + _dbc.list;
			_dbc.gtt = _name + "_" + _dbc.gtt;

			try {
				_db->execute("CREATE TABLE " + _dbc.list + " (LISTDATE DATE DEFAULT SYSDATE, FILENAME VARCHAR(512) PRIMARY KEY, FILESIZE NUMBER(12), ELAPSDUR NUMBER(8), STATUS NUMBER(1) DEFAULT 0)");
			} catch (axon::exception &e) {
				_log->print("WARN", "%s - %s", _name, e.msg());
			}

			try {
				_db->execute("CREATE TABLE " + _dbc.gtt + " (LISTDATE DATE DEFAULT SYSDATE, FILENAME VARCHAR(512) PRIMARY KEY, STATUS NUMBER(1) DEFAULT 0)");
			} catch (axon::exception &e) {
				_log->print("WARN", "%s - %s", _name, e.msg());
			}
		}

		_th = std::thread(&node::monitor, this);
	}
	else
		_log->print("INFO", "%s - service is disabled.", _name);

	return true;
}

int node::run()
{
	// actual stuff is being done here :) welcome to the world of spaghetti coding

	_log->print("INFO", "%s - booting up > %d", _name, _conftype);

	std::shared_ptr<axon::transport::transfer::connection> source, destination;

	try {

		switch (_src_protocol)
		{
			case axon::protocol::FILE:
				{
					std::shared_ptr<axon::transport::transfer::file> p(new axon::transport::transfer::file(_src_ipaddress, _src_username, _src_password));
					
					source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			case axon::protocol::SFTP:
				{
					std::shared_ptr<axon::transport::transfer::sftp> p(new axon::transport::transfer::sftp(_src_ipaddress, _src_username, _src_password));

					if (_src_auth == axon::authtypes::PRIVATEKEY)
					{
						p->set(AXON_TRANSFER_SSH_MODE, axon::transport::transfer::auth_methods::PRIVATEKEY);
						p->set(AXON_TRANSFER_SSH_PRIVATEKEY, _src_privatekey);
					}

					source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			case axon::protocol::FTP:
				{
					std::shared_ptr<axon::transport::transfer::ftp> p(new axon::transport::transfer::ftp(_src_ipaddress, _src_username, _src_password));
					
					source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			default:
				_log->print("ERROR", "%s - not a valid protocol selected %d, cannot continue!", _name, _src_protocol);
				return 0;
				break;
		}

		switch (_dst_protocol)
		{
			case axon::protocol::FILE:
				{
					std::shared_ptr<axon::transport::transfer::file> p(new axon::transport::transfer::file(_dst_ipaddress, _dst_username, _dst_password));
					
					destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			case axon::protocol::SFTP:
				{
					std::shared_ptr<axon::transport::transfer::sftp> p(new axon::transport::transfer::sftp(_dst_ipaddress, _dst_username, _dst_password));

					if (_dst_auth == axon::authtypes::PRIVATEKEY)
					{
						p->set(AXON_TRANSFER_SSH_MODE, axon::transport::transfer::auth_methods::PRIVATEKEY);
						p->set(AXON_TRANSFER_SSH_PRIVATEKEY, _dst_privatekey);
					}

					destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			case axon::protocol::FTP:
				{
					std::shared_ptr<axon::transport::transfer::ftp> p(new axon::transport::transfer::ftp(_dst_ipaddress, _dst_username, _dst_password));
					
					destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
				}
				break;

			default:
				_log->print("ERROR", "%s - not a valid protocol selected %d, cannot continue!", _name, _dst_protocol);
				return 0;
				break;
		}
	} catch (...) {
		_log->print("FATAL", "%s - error creating object", _name);
	}


	// try {

		if (source)
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
			strftime(buffer, PATH_MAX, _filemask.c_str(), xtstamp); filemask = buffer;

			_db->execute("DELETE FROM " + _dbc.gtt);
			_db->flush();

			source->connect();
			source->chwd(_pickpath[0]);

			destination->connect();
			destination->chwd(_droppath);

			if (source->list(v))
			{
				// _db->execute("BEGIN TRANSACTION;");

				for (auto &elm : v)
					if (elm.flag == axon::flags::FILE)
					{
						try {
							DBGPRN("source->list [] => %s", elm.name.c_str());
							_db->execute("INSERT INTO " + _dbc.gtt + "(FILENAME) VALUES (TRIM('" + elm.name + "'))");
						} catch (axon::exception &e) {
		
							_log->print("FATAL", "%s", e.msg());
						}
						count++;
					}

				// _db->execute("END TRANSACTION;");
			}
			_db->flush();

			_log->print("INFO", "%s - latest file get complete, processed %d of %d downloaded records.", _name, count, v.size());

			if (_ignore.size() > 0)
				_db->query("SELECT FILENAME FILENAME FROM (SELECT A.* FROM " + _dbc.gtt + " A LEFT JOIN " + _dbc.list + " B ON A.FILENAME = B.FILENAME WHERE B.FILENAME IS NULL) A LEFT JOIN DAT_FILEBOT_FILELIST B ON REPLACE(A.FILENAME, '" + _ignore + "', '') = B.FILENAME WHERE B.FILENAME IS NULL");
			else
				_db->query("SELECT FILENAME FILENAME FROM (SELECT A.FILENAME FILENAME FROM " + _dbc.gtt + " A LEFT JOIN " + _dbc.list + " B ON A.FILENAME = B.FILENAME WHERE B.FILENAME IS NULL AND A.FILENAME IS NOT NULL)");

			if (_prerun.size() > 0)
			{
				std::string prerun = _prerun;

				boost::replace_all(prerun, "%PATH%", droppath.c_str());

				_log->print("INFO", "%s - running pre-script (%s)", _name, prerun.c_str());
				
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
			while (_db->next())
			{
				list.push_back(_db->get(0));
				count++;
			}
			_db->done();

			if (count > 0)
			{
				_log->print("INFO", "%s - preparing to down %d files...", _name, count);

				for (int current = 0; current < count; current++)
				{
					char sqltext[4096];
					
					const boost::regex mask(filemask);

					if (filemask.size() == 0 || boost::regex_match(list[current], mask))
					{
						std::thread th_tmp([&]() {

							axon::timer t1("file download");

							DBGPRN("queuing [%d] %s", current, list[current].c_str());
							long long filesize = source->get(list[current], _buffer + "/" + list[current], _compress);
							long long putsize = destination->put(_buffer + "/" + list[current], _droppath + "/" + list[current], false);
							unlink(std::string(_buffer + "/" + list[current]).c_str());
							
							_log->print("INFO", "%s - [%d/%d] Downloaded file %s, Size: %d <> %d, Speed: %.2fkb/sec", _name, current, count, list[current].c_str(), filesize, putsize, ((filesize/1000.00)/(t1.now()/1000000.00)));

							sprintf(sqltext, "INSERT INTO %s (FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES('%s',%lld, %ld, %d)", _dbc.list.c_str(), list[current].c_str(), filesize, t1.now(), 1);
							//sprintf(sqltext, "INSERT INTO %s (LISTDATE,FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES(%lu,'%s',%lld, %ld, %d)", _dbc.list.c_str(), std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()).count(), list[current].c_str(), filesize, since(start).count(), 1);
							_db->execute(sqltext);
						});
						th_tmp.join();
					}
					else
					{
						sprintf(sqltext, "INSERT INTO %s (FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES('%s',%lld, %ld, %d)", _dbc.list.c_str(), list[current].c_str(), 0ULL, 0L, 2);
						_db->execute(sqltext);
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

			source->disconnect();
			if (_conftype == 1)
				destination->disconnect();
		}
	// } catch (axon::exception &e) {
	// 	_log->print("ERROR", "%s(%d): %s - %s", __FILENAME__, __LINE__, _name, e.msg());
	// }

	return true;
}

bool node::enabled()
{
	return _status;
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
	std::cout<<std::right<<std::setw(width)<<_name.substr(0, width)<<"─┬─ "<<_shortdesc<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          description ── "<<_longdesc<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─               ip/url ── "<<_src_ipaddress<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             username ── "<<_src_username<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             password ── "<<_src_password<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           public key ── "<<_src_privatekey<<std::endl;
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
	std::cout<<padding.substr(0, width+1)<<"├─               domain ── "<<_src_domain<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─   configuration type ── "<<_conftype<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─               status ── "<<_status<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─             protocol ── "<<_src_protocol<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─                 mode ── "<<_src_mode<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─  authentication type ── "<<_src_auth<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─          compression ── "<<_compress<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─       look back time ── "<<_lookback<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─           sleep time ── "<<_sleeptime<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"├─       trim extension ── "<<_trim<<std::endl;
	std::cout<<padding.substr(0, width+1)<<"└─         trigger time ── "<<_trigger<<std::endl<<std::endl;;
}