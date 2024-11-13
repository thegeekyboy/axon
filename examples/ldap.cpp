#include <axon.h>
#include <axon/ldap.h>

int main(int argc, char ** argv)
{
	axon::directory::ldap ldp;

	std::string dn = "OU=Users,OU=Technology,OU=bKash,DC=bKash,DC=com";
	std::string filter = "(userPrincipalName=amirul.islam@bkash.com)";

	ldp.set(LDAP_OPT_PROTOCOL_VERSION, LDAP_VERSION3);
	ldp.connect("ldap://ad-dc-1.bKash.com", "dc=bKash,dc=com");
	ldp.bind();
	std::cout<<ldp.whoami()<<std::endl;
	// ldp.search("OU=bKash,DC=bKash,DC=com", "(userPrincipalName=amirul.islam@bkash.com)");
	// ldp.search("OU=Users,OU=CXO,OU=bKash,DC=bKash,DC=com", "");
	// ldp.search("OU=Users,OU=Technology,OU=bKash,DC=bKash,DC=com", "");

	if (argc > 1) dn = argv[1];
	if (argc > 2) filter = argv[2];

	std::unique_ptr<axon::directory::ldap::recordset> rc = ldp.search(dn, filter);
	
	std::cout<<AXONVERSION<<"<count: "<<rc->count()<<std::endl;

	while(rc->next()) {
		auto retval = rc->get();

		for (auto &value : retval)
		{
			for (auto &vv : value.values)
			{
				std::for_each(vv.begin(), vv.end(), [](char& c) { c = (std::isprint(c))?c:'?'; });
				std::cout<<value.name<<": "<<vv<<std::endl;
			}
		}
	}

	return 0;
}