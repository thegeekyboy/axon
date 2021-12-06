#include <iostream>
#include <string>

#include <string.h>
#include <krb5/krb5.h>

#include <axon/kerberos.h>

namespace axon  {

	kerberos::kerberos(std::string ktf, std::string ccf, std::string dn)
	{
		reset();

		ktfile = ktf;
		ccfile = ccf;
		domain = dn;

		principal_name = NULL;

		ctx = 0;
		creds = NULL;

		principal_list = NULL;
		principal = NULL;
	}

	kerberos::~kerberos()
	{
		if (principal_name)
			krb5_free_unparsed_name(ctx, principal_name);

		if (creds)
			krb5_free_creds(ctx, creds);

		if (ctx)
			krb5_free_context(ctx);
	}

	void kerberos::eprint(const int errnum, const int ln)
	{
		const char *errstr = krb5_get_error_message(ctx, errnum);

		// printlog("EROR", "(%d) %s", ln, (char*) errstr); // how to print this log?
		krb5_free_error_message(ctx, errstr);
	}

	void kerberos::reset()
	{
	}

	bool kerberos::init()
	{
		int retval;
		
		if ((retval = krb5_init_context(&ctx)))
		{
			eprint(retval, __LINE__);
			return false;
		}

		return true;
	}

	bool kerberos::resolve(krb_t which)
	{
		int retval;

		if (which == KRB::KT)
		{
			if ((retval = krb5_kt_resolve(ctx, ktfile.c_str(), &keytab)))
			{
				eprint(retval, __LINE__);
				return false;
			}
		}
		else if (which == KRB::CC)
		{
			if ((retval = krb5_cc_resolve(ctx, ccfile.c_str(), &cache)))
			{
				eprint(retval, __LINE__);
				return false;
			}
		}

		return true;
	}

	bool kerberos::extract()
	{
		int retval = 0;

		if ((retval = krb5_kt_start_seq_get(ctx, keytab, &cursor)))
		{
			eprint(retval, __LINE__);
			return false;
		}

		while ((retval = krb5_kt_next_entry(ctx, keytab, &entry, &cursor)) == 0)
		{
			//printf("principle: %s\n", krb5_princ_realm(ctx, entry.principal)->data);

			if (!strcasecmp(domain.c_str(), krb5_princ_realm(ctx, entry.principal)->data))
			{
				retval = krb5_unparse_name(ctx, entry.principal, &principal_name);

				if (retval)
				{
					eprint(retval, __LINE__);
					krb5_free_keytab_entry_contents(ctx, &entry);

					return false;
				}
				
				//printf("principal_name: %s\n", principal_name);
			}

			retval = krb5_free_keytab_entry_contents(ctx, &entry);
		}

		retval = krb5_kt_end_seq_get(ctx, keytab, &cursor);

		if (principal_name)
			return true;
		
		return false;
	}

	bool kerberos::renew()
	{
		int retval = 0;
			
		if ((retval = krb5_parse_name(ctx, principal_name, &principal)))
		{
			eprint(retval, __LINE__);
			return false;
		}
		
		creds = (krb5_creds*) malloc(sizeof(*creds));
		memset(creds, 0, sizeof(*creds));

		if ((retval = krb5_get_init_creds_keytab(ctx, creds, principal, keytab, 0, NULL, NULL)))
		{
			eprint(retval, __LINE__);
			return false;
		}

		if ((retval = krb5_cc_initialize(ctx, cache, principal)))
		{
			// eprint(retval, __LINE__);
			return false;
		}

		if ((retval = krb5_cc_store_cred(ctx, cache, creds)))
		{
			eprint(retval, __LINE__);
			return false;
		}

		krb5_cc_close(ctx, cache);

		return true;
	}

	bool kerberos::valid()
	{
		int retval = 0;
		krb5_cc_cursor _cursor;
		bool isvalid = false;
		
		if ((retval = krb5_cc_start_seq_get(ctx, cache, &_cursor)))
		{
			eprint(retval, __LINE__);
			return false;
		}

		krb5_error_code _code;
		krb5_creds _creds;

		while (!isvalid)
		{
			_code = krb5_cc_next_cred(ctx, cache, &_cursor, &_creds);

			if (_code)
				break;

			if ((_creds.times.endtime > time(NULL)) && strcasecmp(_creds.server->realm.data, domain.c_str()) == 0)
				isvalid = true;

			krb5_free_cred_contents(ctx, &_creds);
		}

		krb5_cc_end_seq_get(ctx, cache, &_cursor);

		return isvalid;
	}

	bool kerberos::issue()
	{
		this->init();

		if (this->resolve(axon::KRB::CC))
		{
			if (!this->valid())
			{
				// std::cout<<"issuing new ticket"<<std::endl;

				if (!this->resolve(axon::KRB::KT))
					return false;

				if (!this->extract())
					return false;

				if (!this->renew())
					return false;

				// std::cout<<"new ticket issued"<<std::endl;
			}
			// else
			// 	std::cout<<"ticket already valid"<<std::endl;
		}
		else
			return false;

		return true;
	}
}