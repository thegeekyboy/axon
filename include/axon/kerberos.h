#ifndef KERBEROS_H_
#define KERBEROS_H_

#include <string>
#include <krb5/krb5.h>

namespace axon
{
	namespace authentication
	{
		typedef unsigned char krb_t;

		struct KRB {
			static const krb_t KT = 1;
			static const krb_t CC = 2;
		};

		class kerberos {

			std::string _keytab_file, _cache_file, _realm, _principal;

			krb5_context _ctx;
			krb5_keytab _keytab;
			krb5_ccache _cache;

			std::string _id;

			std::string _errstr(long int);

		public:
			kerberos() = delete;
			kerberos(std::string, std::string, std::string, std::string);
			~kerberos();

			void print(const int, const int);

			bool init();
			std::string cachePrincipal();
			bool isCacheValid();
			bool isValidKeytab();

			void renew();
		};
	}
}

#endif //KERBEROS_H_