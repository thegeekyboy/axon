#include <iostream>

#include <axon.h>
#include <axon/kerberos.h>


int main()
{
	try {
		axon::authentication::kerberos krb("keytab.kt", "/tmp/ticket.cache", "DOMAIN.COM", "username@DOMAIN.COM");

		krb.init();
		std::cout<<"cache principal: "<<krb.cachePrincipal()<<std::endl;
		
		if (!krb.isCacheValid())
		{
			if (!krb.isValidKeytab())
				std::cout<<"keytab does not contain desired entry!"<<std::endl;
			else
				krb.renew();
		}
		else
			std::cout<<"cache is still valid for the given principal"<<std::endl;
	} catch (axon::exception &e) {
		std::cout<<"error = "<<e.what()<<std::endl;
	}

	return 0;
}