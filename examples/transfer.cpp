#include <iostream>
#include <fstream>
#include <cstdlib>

#include <axon.h>
#include <axon/util.h>

#include <axon/connection.h>

#include <axon/socket.h>
#include <axon/ssh.h>

#include <axon/file.h>
#include <axon/hdfs.h>
#include <axon/s3.h>

#include <axon/kerberos.h>

int main([[maybe_unused]]int argc, [[maybe_unused]] char* argv[], [[maybe_unused]] char* env[])
{
	const char *envp;
	std::string hostname, username, password, schema_registry, domain, ora_sid, krb5_keytab, krb5_cachepath, bootstrap, kafka_consumer_group, scylla_keyspace, proxy;

	if ((envp = std::getenv("http_proxy")) != nullptr) proxy = envp;
	if ((envp = std::getenv("AXON_DOMAIN")) != nullptr) domain = envp;
	if ((envp = std::getenv("AXON_ORA_SID")) != nullptr) ora_sid = envp;
	if ((envp = std::getenv("AXON_USERNAME")) != nullptr) username = envp;
	if ((envp = std::getenv("AXON_PASSWORD")) != nullptr) password = envp;
	if ((envp = std::getenv("AXON_HOSTNAME")) != nullptr) hostname = envp;
	if ((envp = std::getenv("AXON_BOOTSTRAP")) != nullptr) bootstrap = envp;
	if ((envp = std::getenv("AXON_KRB5_KEYTAB")) != nullptr) krb5_keytab = envp;
	if ((envp = std::getenv("AXON_KRB5_CACHEPATH")) != nullptr) krb5_cachepath = envp;
	if ((envp = std::getenv("AXON_SCHEMA_REGISTRY")) != nullptr) schema_registry = envp;
	if ((envp = std::getenv("AXON_SCYLLA_KEYSPACE")) != nullptr) scylla_keyspace = envp;
	if ((envp = std::getenv("AXON_KAFKA_CONSUMER_GROUP")) != nullptr) kafka_consumer_group = envp;
	
	std::ofstream tempfile("axon_test");
	for (int i = 0; i < (512*1024*2); i++) tempfile<<(char)('!' + rand()%92);
	tempfile.close();

	try {
		std::vector<axon::entry> v;

		axon::transfer::connection *conn;
		// axon::transfer::hdfs conn(hostname, username, password, 8020);
		axon::transfer::s3 xs3(hostname, username, password);
		conn = &xs3;

		std::cout<<"hostname: "<<hostname<<", username: "<<username<<", password: "<<password<<std::endl;

		if (false) { // if we need kerberos authentication
			axon::authentication::kerberos krb(krb5_keytab, krb5_cachepath, domain, username);

			std::cout<<"domain: "<<domain<<", keytab: "<<krb5_keytab<<", ccache: "<<krb5_cachepath<<std::endl;

			if (!krb.isCacheValid())
			{
				std::cout<<"cache has expired"<<std::endl;

				if (!krb.isValidKeytab())
					std::cout<<"keytab:"<<krb5_keytab<<" does not contain desired entry: "<<username+"@"+domain<<"!"<<std::endl;
				else
				{
					krb.authenticate();
					krb.isCacheValid();
				}
			}
			else
				std::cout<<"cache is still valid for the given principal"<<std::endl;
			conn->set(AXON_TRANSFER_HDFS_AUTHTYPE, "kerberos");
			conn->set(AXON_TRANSFER_HDFS_CACHEPATH, krb5_cachepath);
			conn->set(AXON_TRANSFER_HDFS_ENCRYPT, "true");
			conn->set(AXON_TRANSFER_HDFS_AUTHALGO, "3des");
		}

		if (proxy.size() > 5) conn->set(AXON_TRANSFER_S3_PROXY, proxy);

		// conn->set(AXON_TRANSFER_SSH_USE_SCP, true); // for ssh connector

		// conn->filter(".*2025-05-25-15-5.*");
		
		conn->connect();
		// conn->login(); // depricated
		
		std::cout<<"working directory: "<<conn->pwd()<<std::endl;
		// conn->chwd("/tmp/");
		conn->chwd("/uat-bkash-next-sentinel-eventstream/tmp");
		// conn->mkdir("/tmp/axon");
		// conn->chwd("/tmp/axon");

		// conn->get("README.md", "README.md.bz2", true); // depricated
		conn->put("axon_test");
		conn->copy("axon_test", "axon_text.copy");
		conn->ren("axon_test", "/uat-bkash-sentinel-persona/tmp/axon_test.ren");
		conn->del("axon_text.copy");
		conn->del("/uat-bkash-sentinel-persona/tmp/axon_test.ren");
/*

		// conn->copy("/tmp/filename.tar.bz2", "/home/username/filename.tar.bz2");
		// conn->copy("/tmp/filename.tar.bz2", "/home/username/");

		
		// conn->open("/uat-bkash-sentinel-persona/tmp/axon_test.ren", std::ios::in);
		// conn->close();

		// conn->filter(".*2025-05-25-15-5.*");
		if (conn->list(v))
		{
			for (auto &elm : v)
			{
				if (elm.flag == axon::flags::FILE)
					std::cout<<"F=> "<<elm.name;
				else if (elm.flag == axon::flags::DIR)
					std::cout<<"D=> "<<elm.name;
				else if (elm.flag == axon::flags::LINK)
					std::cout<<"L=> "<<elm.name;

				std::cout<<" - "<<elm.size<<std::endl;
			}
			std::cout<<"+++ total elements list = "<<v.size()<<std::endl;
		}
*/
		// conn->put("axon_test", "axon_test_get");

		// axon::transfer::file conn2(hostname, "amirul.islam", password);
		// conn2.connect();
		// conn2.open("/home/amirul.islam/development/axon/build/axon_test_get", std::ios::out);
		// conn->open("/uat-bkash-next-sentinel-eventstream/tmp/axon_test_get", std::ios::in);
		// conn>>conn2;
		// // conn<<conn2;
		// conn->close();
		// conn2.close();
		// conn2.disconnect();

		// conn->del("axon_test_get");

		conn->disconnect();

	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
	}

	return 0;
}
