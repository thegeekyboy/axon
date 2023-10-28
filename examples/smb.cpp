#include <iostream>
#include <mutex>

#include <axon.h>
#include <axon/connection.h>
#include <axon/samba.h>

int main()
{
	axon::transport::transfer::samba smb("10.66.13.20", "username", "password", 0);

	smb.set(AXON_TRANSFER_SAMBA_DOMAIN, "domain.com");
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