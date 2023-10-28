#include <axon.h>
#include <axon/socket.h>
#include <axon/connection.h>
#include <axon/ftp.h>

int main()
{
	try {

		std::vector<axon::entry> v;

		axon::transport::transfer::ftp conn("10.151.51.84", "raagent", "rapasswd");

		//conn.init();
		conn.connect();
		//conn.login();
		//conn.pwd();
		conn.chwd("/SGSN");
		//conn.chwd("/bakmed/data/srcbak/HRT-MSC");
		//conn.ren("ftp.test", "mark.test");
		//conn.del("cantdel");
		conn.list(&v);
		conn.get("gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", 
					"gzAP32070003315946_Voxtel_SGSN_GGSN_20150906100140_5194048.DAT", false);

		//for (int i = 0; i < v.size(); i++)
		//	std::cout<<v[i].name<<std::endl;

	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}
