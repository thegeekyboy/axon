#include <iostream>
#include <mutex>

#include <axon.h>
#include <axon/connection.h>
#include <axon/samba.h>

int main()
{
	std::string hostname, username, password, schema;

	for(int i=0;env[i]!=NULL;i++)
	{
    	auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_HOSTNAME")
			address = parts[1];
		else if (parts[0] == "AXON_DOMAIN")
			schema = parts[1];
	}

	axon::transfer::samba smb(hostname, username, password, 0);

	smb.set(AXON_TRANSFER_SAMBA_DOMAIN, domain);
	smb.set(AXON_TRANSFER_SAMBA_SHARE, "SHARENAME");

	smb.connect();
	smb.chwd("/SHARENAME/Blah");

	smb.mkdir("TEST2");
	// OK smb.list([](axon::entry &x) { std::cout<<x.name<<std::endl; });
	// OK smb.ren("/SHARENAME/Blah/TEST.txt", "/SHARENAME/Blah/TEST2.txt");
	// OK smb.del("/SHARENAME/Blah/TEST2.txt");
	// OK smb.get("/EDA/SEDP.rar", "SEDP.rar");
	// OK smb.put("CMakeCache.txt", "/EDA/CMakeCache.txt", true);

	return 0;
}