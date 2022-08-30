#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/ssh.h>
#include <axon/socket.h>
#include <axon/ftp.h>
#include <axon/file.h>
#include <axon/s3.h>
#include <axon/samba.h>

#include <main.h>
#include <node.h>
#include <drone.h>

std::string drone::_speed(long long size, long usec)
{
	double speed;
	std::ostringstream out;

	out.precision(2);

	if ((speed = roundf(100*(size/(1073741824.00))/(usec/1000000.00))/100) > 1)
		out<<std::fixed<<speed<<"GB/sec";
	else if ((speed = roundf(100*(size/(1048576.00))/(usec/1000000.00))/100) > 1)
		out<<std::fixed<<speed<<"MB/sec";
	else if ((speed = roundf(100*(size/(1024.00))/(usec/1000000.00))/100) > 1)
		out<<std::fixed<<speed<<"KB/sec";
	else
		out<<std::fixed<<(roundf(100*(size/1024.00)/(usec/1000000.00))/100)<<"bytes/sec";

	return out.str();
}

drone::drone(node &n)
{
	_node = &n;

	switch (n.get(NODE_CFG_SRC_PROTOCOL))
	{
		case axon::protocol::FILE:
			{
				std::shared_ptr<axon::transport::transfer::file> p(new axon::transport::transfer::file(n.get(NODE_CFG_SRC_IPADDRESS), n.get(NODE_CFG_SRC_USERNAME), n.get(NODE_CFG_SRC_PASSWORD)));
				
				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::SFTP:
			{
				std::shared_ptr<axon::transport::transfer::sftp> p(new axon::transport::transfer::sftp(n.get(NODE_CFG_SRC_IPADDRESS), n.get(NODE_CFG_SRC_USERNAME), n.get(NODE_CFG_SRC_PASSWORD)));

				if (n.get(NODE_CFG_SRC_AUTH) == axon::authtypes::PRIVATEKEY)
				{
					p->set(AXON_TRANSFER_SSH_MODE, axon::transport::transfer::auth_methods::PRIVATEKEY);
					p->set(AXON_TRANSFER_SSH_PRIVATEKEY, n.get(NODE_CFG_SRC_PRIVATE_KEY));
				}

				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::FTP:
			{
				std::shared_ptr<axon::transport::transfer::ftp> p(new axon::transport::transfer::ftp(n.get(NODE_CFG_SRC_IPADDRESS), n.get(NODE_CFG_SRC_USERNAME), n.get(NODE_CFG_SRC_PASSWORD)));
				
				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::S3:
			{
				std::shared_ptr<axon::transport::transfer::s3> p(new axon::transport::transfer::s3(n.get(NODE_CFG_SRC_IPADDRESS), n.get(NODE_CFG_SRC_USERNAME), n.get(NODE_CFG_SRC_PASSWORD)));
				
				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::SAMBA:
			{
				std::shared_ptr<axon::transport::transfer::samba> p(new axon::transport::transfer::samba(n.get(NODE_CFG_SRC_IPADDRESS), n.get(NODE_CFG_SRC_USERNAME), n.get(NODE_CFG_SRC_PASSWORD)));

				p->set(AXON_TRANSFER_SAMBA_DOMAIN, n.get(NODE_CFG_SRC_DOMAIN));

				std::vector<std::string> parts = axon::split(n.get(NODE_CFG_PICKPATH), '/');
				p->set(AXON_TRANSFER_SAMBA_SHARE, parts[0]);

				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		default:
			_log->print("ERROR", "%s - not a valid protocol selected %d, cannot continue!", n.get(NODE_CFG_NAME).c_str(), n.get(NODE_CFG_SRC_PROTOCOL));
			break;
	}

	switch (n.get(NODE_CFG_DST_PROTOCOL))
	{
		case axon::protocol::FILE:
			{
				std::shared_ptr<axon::transport::transfer::file> p(new axon::transport::transfer::file(n.get(NODE_CFG_DST_IPADDRESS), n.get(NODE_CFG_DST_USERNAME), n.get(NODE_CFG_DST_PASSWORD)));
				
				_destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::SFTP:
			{
				std::shared_ptr<axon::transport::transfer::sftp> p(new axon::transport::transfer::sftp(n.get(NODE_CFG_DST_IPADDRESS), n.get(NODE_CFG_DST_USERNAME), n.get(NODE_CFG_DST_PASSWORD)));

				if (n.get(NODE_CFG_DST_AUTH) == axon::authtypes::PRIVATEKEY)
				{
					p->set(AXON_TRANSFER_SSH_MODE, axon::transport::transfer::auth_methods::PRIVATEKEY);
					p->set(AXON_TRANSFER_SSH_PRIVATEKEY, n.get(NODE_CFG_DST_PRIVATE_KEY));
				}

				_destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::FTP:
			{
				std::shared_ptr<axon::transport::transfer::ftp> p(new axon::transport::transfer::ftp(n.get(NODE_CFG_DST_IPADDRESS), n.get(NODE_CFG_DST_USERNAME), n.get(NODE_CFG_DST_PASSWORD)));
				
				_destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::S3:
			{
				std::shared_ptr<axon::transport::transfer::s3> p(new axon::transport::transfer::s3(n.get(NODE_CFG_DST_IPADDRESS), n.get(NODE_CFG_DST_USERNAME), n.get(NODE_CFG_DST_PASSWORD)));
				
				_destination = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		case axon::protocol::SAMBA:
			{
				std::shared_ptr<axon::transport::transfer::samba> p(new axon::transport::transfer::samba(n.get(NODE_CFG_DST_IPADDRESS), n.get(NODE_CFG_DST_USERNAME), n.get(NODE_CFG_DST_PASSWORD)));

				p->set(AXON_TRANSFER_SAMBA_DOMAIN, n.get(NODE_CFG_DST_DOMAIN));

				std::vector<std::string> parts = axon::split(n.get(NODE_CFG_DROPPATH), '/');
				p->set(AXON_TRANSFER_SAMBA_SHARE, parts[0]);

				_source = std::dynamic_pointer_cast<axon::transport::transfer::connection>(p);
			}
			break;

		default:
			_log->print("ERROR", "%s - not a valid protocol selected %d, cannot continue!", n.get(NODE_CFG_NAME).c_str(), n.get(NODE_CFG_DST_PROTOCOL));
			break;
	}

	_source->connect();
	_source->chwd(n.get(NODE_CFG_PICKPATH));
	_destination->connect();
	_destination->chwd(n.get(NODE_CFG_DROPPATH));
}

drone::~drone()
{
	if (_enabled)
		_th.join();
}

bool drone::get(std::string& filename)
{
	return true;
}

bool drone::put(std::string& name)
{
	if (!_busy.test_and_set())
		return false;

	_busy.clear();

	return true;
}

void drone::start()
{
	_th = std::thread([&] {

		std::string filename;
		
		_enabled = true;

		while ((filename = _node->pop()) != "")
		{
			char sqltext[512];
			axon::timer t1(filename);

			long long filesize = _source->get(filename, _node->get(NODE_CFG_BUFFER) + "/" + filename, _node->get(NODE_CFG_COMPRESS));
			long long putsize = _destination->put(_node->get(NODE_CFG_BUFFER) + "/" + filename, _node->get(NODE_CFG_DROPPATH) + "/" + filename, false);
			unlink(std::string(_node->get(NODE_CFG_BUFFER) + "/" + filename).c_str());

			_log->print("INFO", "%s - [%d/%d] Downloaded file %s, Size: %d, Speed: %s", _node->get(NODE_CFG_NAME), 0, 0, filename.c_str(), filesize, _speed(filesize,t1.now()).c_str());

			sprintf(sqltext, "INSERT INTO %s (FILENAME,FILESIZE,ELAPSDUR,STATUS) VALUES('%s',%lld, %ld, %d)", _dbc->list.c_str(), filename.c_str(), filesize, t1.now(), 1);
			_db->execute(sqltext);

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	});
}

void drone::stop()
{

}

bool drone::busy()
{
	if (!_busy.test_and_set())
		return true;
	_busy.clear();

	return false;
}