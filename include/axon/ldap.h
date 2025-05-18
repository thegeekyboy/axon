#ifndef AXON_LDAP3_H_
#define AXON_LDAP3_H_

#define STRINGIZE(x) STRINGIZE_A(x)
#define STRINGIZE_A(x) #x

#include <memory>
#include <vector>
#include <sstream>

#include STRINGIZE(LDAP_INCLUDEDIR/ldap.h)
#include <sasl/sasl.h>

#include <axon.h>

namespace axon {

	namespace directory {

		typedef struct _LDAPAuth LDAPAuth;
		struct _LDAPAuth
		{
			const char *dn;        ///< DN to use for simple bind
			const char *saslmech;  ///< SASL mechanism to use for authentication
			const char *authuser;  ///< user to authenticate
			const char *username;  ///< pre-authenticated user
			const char *password;  ///< user password
			const char *realm;     ///< SASL realm used for authentication
			BerValue cred;         ///< the credentials of "user" (i.e. password)
		};


		class ldap {

			LDAP *_ldap;

			std::string _uri, _dn, _mechanism;
			uint16_t _port;

			bool _connected;

			public:

			struct error {

				std::stringstream message;
				bool isError;

				error(LDAP *ldap, int rc) {
					char *msg;
					isError = false;
					if (rc != LDAP_SUCCESS) {
						ldap_get_option(ldap, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&msg);
						message<<ldap_err2string(rc)<<" - "<<msg;
						ldap_memfree(msg);
						isError = true;
					}
					else
						message<<"no error";
				}
				std::string get() { return message.str(); }
				operator bool() { return isError; }
			};

			struct record {

				uint16_t count;
				std::string name;
				std::vector<std::string> values;
			};

			class recordset {

				axon::directory::ldap *_ldap;
				LDAPMessage *_response, *_entry;
				bool _first;

				public:
				recordset() = delete;
				recordset(ldap *l, LDAPMessage *r): _ldap(l), _response(r), _first(false) {
					if ((_entry = ldap_first_entry(_ldap->get(), _response)) == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot extract entry");
					_first = true;
				};
				~recordset() {
					ldap_msgfree(_entry);
					if (!_first) ldap_msgfree(_response);
				};

				uint16_t count() { return ldap_count_entries(_ldap->get(), _response); /* TODO: need error handling */ }

				bool next() {
					if (_first) {
						_first = false;
						return true;
					}

					if ((_entry = ldap_next_entry(_ldap->get(), _entry)))
						return true;
					return false;
				};

				std::vector<record> get()
				{
					std::vector<record> retval;

					BerElement *element;
					char *attribute;

					for (attribute = ldap_first_attribute(_ldap->get(), _entry, &element); attribute; attribute = ldap_next_attribute(_ldap->get(), _entry, element))
					{
						int count = 0;
						record rc;

						rc.name = attribute;

						BerValue **values = ldap_get_values_len(_ldap->get(), _entry, attribute);

						for (count = 0; values[count] != NULL; count++)
							rc.values.push_back(values[count]->bv_val);

						rc.count = count;

						retval.push_back(std::move(rc));

						ldap_value_free_len(values);
						ldap_memfree(attribute);
					}

					if (element) ber_free(element, 0);

					return retval;
				}
			};

			ldap();
			~ldap();

			void connect(std::string, uint16_t, std::string);
			void connect(std::string, std::string);
			void connect(std::string, uint16_t);
			void connect(std::string);

			void bind();
			std::unique_ptr<axon::directory::ldap::recordset> search(std::string, std::string);

			void set(int, int);
			LDAP *get() const { return _ldap; }

			std::string whoami();
		};
	};
};

#endif
