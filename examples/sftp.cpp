#include <axon.h>
#include <axon/socket.h>
#include <axon/connection.h>
#include <axon/ssh.h>

int main()
{
	try {

		std::vector<axon::entry> v;

		axon::transport::transfer::sftp conn("10.0.0.0", "username", "PASSWORD_HERE", 22);

		conn.connect();
		//conn.login();
		//conn.pwd();
		conn.chwd("/home/username");
		conn.mkdir("/tmp/testing123");
		conn.copy("/tmp/filename.tar.bz2", "/home/username/");
		//conn.chwd("/bakmed/data/srcbak/HRT-MSC");
		//conn.ren("ftp.test", "mark.test");
		//conn.del("cantdel");
		if (conn.list(v))
		{
			for (auto &elm : v)
				if (elm.flag == axon::flags::FILE)
					std::cout<<elm.name<<std::endl;
		}
		//printf("Total file list num = %d\n", cnt);
		// conn.get("gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", 
		// 			"gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", false);


		// conn.get("kibana.working.20180507.tar", "kibana.working.20180507.tar", false);
		conn.disconnect();

	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}
