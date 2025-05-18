#include <iostream>

#include <axon.h>
#include <axon/kerberos.h>
#include <axon/util.h>

int main([[maybe_unused]]int argc, [[maybe_unused]] char* argv[], [[maybe_unused]] char* env[])
{
	std::string hostname, username, password, schema_registry, domain, ora_sid, krb5_keytab, krb5_cachepath, bootstrap, kafka_consumer_group, scylla_keyspace;

	for (int i=0; env[i]!=NULL; i++)
	{
		auto parts = axon::util::split(env[i], '=');

		if (parts[0] == "AXON_USERNAME")
			username = parts[1];
		else if (parts[0] == "AXON_PASSWORD")
			password = parts[1];
		else if (parts[0] == "AXON_HOSTNAME")
			hostname = parts[1];
		else if (parts[0] == "AXON_DOMAIN")
			domain = parts[1];
		else if (parts[0] == "AXON_SCHEMA_REGISTRY")
			schema_registry = parts[1];
		else if (parts[0] == "AXON_ORA_SID")
			ora_sid = parts[1];
		else if (parts[0] == "AXON_KRB5_KEYTAB")
			krb5_keytab = parts[1];
		else if (parts[0] == "AXON_KRB5_CACHEPATH")
			krb5_cachepath = parts[1];
		else if (parts[0] == "AXON_BOOTSTRAP")
			bootstrap = parts[1];
		else if (parts[0] == "AXON_KAFKA_CONSUMER_GROUP")
			kafka_consumer_group = parts[1];
		else if (parts[0] == "AXON_SCYLLA_KEYSPACE")
			scylla_keyspace = parts[1];
	}

	try {
		axon::authentication::kerberos krb(krb5_keytab, krb5_cachepath, domain, username);

		std::cout<<"keytab: "<<krb5_keytab<<std::endl;
		std::cout<<"cache: "<<krb5_cachepath<<std::endl;
		std::cout<<"cache principal: "<<krb.cachePrincipal()<<std::endl;
		
		if (!krb.isCacheValid())
		{
			std::cout<<"cache has expired"<<std::endl;

			if (!krb.isValidKeytab())
				std::cout<<"keytab does not contain desired entry!"<<std::endl;
			else
			{
				krb.authenticate();
				krb.isCacheValid();
			}
		}
		else
			std::cout<<"cache is still valid for the given principal"<<std::endl;
	} catch (axon::exception &e) {
		std::cout<<"error = "<<e.what()<<std::endl;
	}

	return 0;
}
