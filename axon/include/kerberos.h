#ifndef KERBEROS_H_
#define KERBEROS_H_

#include <string>
#include <krb5/krb5.h>

namespace axon {

	typedef unsigned char krb_t;

	struct KRB {
		static const krb_t KT = 1;
		static const krb_t CC = 2;
	};

	class kerberos {

		std::string ktfile, ccfile, domain, username;

		int nprinc;
		char *principal_name;
		
		krb5_context ctx;
		krb5_keytab keytab;
		krb5_kt_cursor cursor;
		krb5_keytab_entry entry;
		krb5_ccache cache;
		krb5_creds *creds;

		krb5_principal *principal_list;
		krb5_principal principal;

	public:
		kerberos() = delete;
		kerberos(std::string, std::string, std::string);
		~kerberos();

		void eprint(const int, const int);

		void reset();
		bool init();

		bool resolve(krb_t);
		bool extract();
		bool renew();
		bool valid();
		bool issue();

	};
}

#endif //KERBEROS_H_