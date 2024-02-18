#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include <axon.h>
#include <axon/connection.h>
#include <axon/s3.h>


int main()
{
	axon::transport::transfer::s3 conn("ap-southeast-1", "ACCESS KEY", "SECRET KEY", 8080);
	std::vector<axon::entry> v;

	conn.set(AXON_TRANSFER_S3_PROXY, "10.0.0.0:4951");

	try {
		
		conn.connect();
		conn.chwd("/bucket-name/");
		conn.pwd();
		// conn.login();
		// conn.ren("ftp.test", "mark.test");
		// conn.del("cantdel");

		if (conn.list(v))
		{
			for (auto &elm : v)
				if (elm.flag == axon::flags::FILE)
					std::cout<<"elm.name<<std::endl";
		}

		conn.disconnect();
	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}