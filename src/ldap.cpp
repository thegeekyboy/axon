#include <stdio.h>

#include <stdlib.h>

#include <axon.h>
#include <axon/ldap.h>

namespace axon {

	namespace directory {

		int genericSaslInteract(LDAP *ld, unsigned flags, void *defaults, void *sin)
		{
			axon::directory::LDAPAuth *ldap_inst;
			sasl_interact_t *interact;

			if (!(ld))
				return(LDAP_PARAM_ERROR);

			if (!(defaults))
				return(LDAP_PARAM_ERROR);

			if (!(sin))
				return(LDAP_PARAM_ERROR);

			switch(flags)
			{
				case LDAP_SASL_AUTOMATIC:
				case LDAP_SASL_INTERACTIVE:
				case LDAP_SASL_QUIET:
				default:
					break;
			};

			ldap_inst = static_cast<axon::directory::LDAPAuth*>(defaults);

			for(interact = static_cast<sasl_interact_t*>(sin); (interact->id != SASL_CB_LIST_END); interact++)
			{
				interact->result = NULL;
				interact->len	= 0;

				switch(interact->id)
				{
					case SASL_CB_GETREALM:
					// fprintf(stderr, "SASL Data: SASL_CB_GETREALM (%s)\n", ldap_inst->realm ? ldap_inst->realm : "");
					interact->result = ldap_inst->realm ? ldap_inst->realm : "";
					interact->len	= (unsigned) strlen(static_cast<const char*>(interact->result));
					break;

					case SASL_CB_AUTHNAME:
					// fprintf(stderr, "SASL Data: SASL_CB_AUTHNAME (%s)\n", ldap_inst->authuser ? ldap_inst->authuser : "");
					interact->result = ldap_inst->authuser ? ldap_inst->authuser : "";
					interact->len	= (unsigned) strlen(static_cast<const char*>(interact->result));
					break;

					case SASL_CB_PASS:
					// fprintf(stderr, "SASL Data: SASL_CB_PASS (%s) => %s\n", ldap_inst->cred.bv_val ? ldap_inst->cred.bv_val : "", ldap_inst->password);
					// interact->result = ldap_inst->cred.bv_val ? ldap_inst->cred.bv_val : "";
					// interact->len	= (unsigned) ldap_inst->cred.bv_len;
					interact->result = ldap_inst->password ? ldap_inst->password : "";
					interact->len	= (unsigned) strlen(static_cast<const char*>(interact->result));
					break;

					case SASL_CB_USER:
					// fprintf(stderr, "SASL Data: SASL_CB_USER set (%s)\n", ldap_inst->username ? ldap_inst->username : "");
					// fprintf(stderr, "SASL Data: SASL_CB_USER get (%s)\n", interact->result);
					interact->result = ldap_inst->username ? ldap_inst->username : "";
					interact->len	= (unsigned) strlen(static_cast<const char*>(interact->result));
					break;

					case SASL_CB_NOECHOPROMPT:
					// fprintf(stderr, "SASL Data: SASL_CB_NOECHOPROMPT\n");
					break;

					case SASL_CB_ECHOPROMPT:
					// fprintf(stderr, "SASL Data: SASL_CB_ECHOPROMPT\n");
					break;

					default:
					// fprintf(stderr, "SASL Data: unknown option: %lu\n", interact->id);
					break;
				};
			};

			return LDAP_SUCCESS;
		}

		ldap::ldap(): _ldap(NULL), _mechanism("GSSAPI"), _connected(false)
		{

		}

		ldap::~ldap()
		{
			if (_ldap) ldap_destroy(_ldap);
			_connected = false;
		}

		void ldap::connect(std::string uri, uint16_t port, std::string dn)
		{
			int rc;

			if (uri.size() < 8)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "uri is too short");

			if (port == 0)
				_port = 389;

			if (dn.size() <= 3)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid distinguished name (dn)");

			if ((rc = ldap_initialize(&_ldap, uri.c_str())) != LDAP_SUCCESS)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, std::string("could not create LDAP session handle - ") + ldap_err2string(rc));

			_uri = uri;
			_dn = dn;

			_connected = true;
		}

		void ldap::connect(std::string uri, std::string dn)
		{
			connect(uri, 389, dn);
		}

		void ldap::connect(std::string uri, uint16_t port)
		{
			connect(uri, port, std::string());
		}

		void ldap::connect(std::string uri)
		{
			connect(uri, 389);
		}

		void ldap::bind()
		{
			static axon::directory::LDAPAuth auth;

			// auth.username
			// auth.password

			int rc = ldap_sasl_interactive_bind_s(
				_ldap,
				_dn.c_str(),
				_mechanism.c_str(),
				NULL,
				NULL,
				LDAP_SASL_QUIET,
				axon::directory::genericSaslInteract,
				&auth
			);

			error err(_ldap, rc);
			if (err) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, err.get());
		}

		std::unique_ptr<axon::directory::ldap::recordset> ldap::search(std::string query, std::string filter)
		{
			LDAPMessage *response;//, *entry;

			int rc = ldap_search_ext_s(
				_ldap,										// LDAP            * ld
				query.c_str(),								// char            * base
				2,											// int               scope
				(filter.size()>0)?filter.c_str():NULL,		// char            * filter
				NULL,										// char            * attrs[]
				0,											// int               attrsonly
				NULL,										// LDAPControl    ** serverctrls
				NULL,										// LDAPControl    ** clientctrls
				(timeval*) NULL,							// struct timeval  * timeout
				0,											// int               sizelimit
				&response									// LDAPMessage    ** res
			);

			error err(_ldap, rc);
			if (err) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, err.get());

			return std::make_unique<axon::directory::ldap::recordset>(this, response);
		}

		void ldap::set(int option, int value)
		{
			if (ldap_set_option(_ldap, option, &value) != LDAP_OPT_SUCCESS)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "could not set option");
		}

		std::string ldap::whoami()
		{
			BerValue *authzid;
			int rc;

			if ((rc = ldap_whoami_s(_ldap, &authzid, NULL, NULL)) != LDAP_SUCCESS)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, std::string("could not set option") + ldap_err2string(rc));

			std::string retval = authzid->bv_val;
			ber_bvfree(authzid);

			return retval;
		}
	};
};
