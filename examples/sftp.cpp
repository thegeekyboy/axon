#include <iostream>

#include <axon.h>
#include <axon/socket.h>
#include <axon/connection.h>
#include <axon/ssh.h>
#include <axon/util.h>

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[], char* env[])
{
	std::string address, username, password, schema;

	for(int i=0;env[i]!=NULL;i++)
	{
    	auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_ADDRESS")
			address = parts[1];
		else if (parts[0] == "AXON_SCHEMA")
			schema = parts[1];
	}

	try {

		std::vector<axon::entry> v;

		axon::transfer::sftp conn(address, username, password, 22);

		conn.connect();
		conn.set(AXON_TRANSFER_SSH_USE_SCP, true);
		// conn.login();
		// conn.pwd();
		// conn.chwd("/home/username");
		// conn.mkdir("/tmp/testing123");
		conn.get("README.md", "README.md.bz2", true);
		// conn.copy("/tmp/filename.tar.bz2", "/home/username/");
		// conn.chwd("/bakmed/data/srcbak/HRT-MSC");
		// conn.ren("ftp.test", "mark.test");
		// conn.del("cantdel");
		if (conn.list(v))
		{
			for (auto &elm : v)
				if (elm.flag == axon::flags::FILE)
					std::cout<<elm.name<<std::endl;
		}
		// printf("Total file list num = %d\n", cnt);
		// conn.get("gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", 
		// 			"gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", false);


		// conn.get("kibana.working.20180507.tar", "kibana.working.20180507.tar", false);
		conn.disconnect();

	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}
