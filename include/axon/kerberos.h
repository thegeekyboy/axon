#ifndef KERBEROS_H_
#define KERBEROS_H_

#include <string>
#include <krb5/krb5.h>

#include <axon/util.h>

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

			class context {
				krb5_context _pointer;

				public:
					context(): _pointer(nullptr) {
						if (krb5_init_context(&_pointer)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize context");
					}
					context(const context&) = delete;
					context& operator=(const context&) = delete;
					context(context&&) = delete;
					context& operator=(context&&) = delete;
					~context() { if (_pointer != nullptr) krb5_free_context(_pointer); }
					krb5_context get() {
						if (_pointer == nullptr)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "subscription not ready");
						return _pointer;
					}
					operator krb5_context() { return get(); }
			};

			class keytab {
				krb5_keytab _pointer;
				context &_context;

				public:
					keytab() = delete;
					keytab(context &ctx, std::string ktfile): _pointer(nullptr), _context(ctx) {
						// if (!axon::util::exists(ktfile)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "keytab file is not accessible");
						if (krb5_kt_resolve(ctx.get(), ktfile.c_str(), &_pointer)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize context");
					}
					keytab(keytab&&) = delete;
					keytab(const keytab&) = delete;
					keytab& operator=(keytab&&) = delete;
					keytab& operator=(const keytab&) = delete;
					~keytab() { if (_pointer != nullptr) krb5_kt_close(_context.get(), _pointer); }
					krb5_keytab get() {
						if (_pointer == nullptr)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "keytab not ready");
						return _pointer;
					}
					operator krb5_keytab() { return get(); }

					bool find(std::string);
			};

			class cache {
				krb5_ccache _pointer;
				context &_context;

				public:
					cache() = delete;
					cache(context &ctx, std::string ccfile): _pointer(nullptr), _context(ctx) {
						// if (!axon::util::iswritable(ccfile)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cache file is not accessible " + ccfile);
						if (krb5_cc_resolve(ctx.get(), ccfile.c_str(), &_pointer)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize context");
					}
					cache(cache&&) = delete;
					cache(const cache&) = delete;
					cache& operator=(cache&&) = delete;
					cache& operator=(const cache&) = delete;
					~cache() { if (_pointer != nullptr) krb5_cc_close(_context.get(), _pointer); }
					krb5_ccache get() {
						if (_pointer == nullptr)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "keytab not ready");
						return _pointer;
					}
					operator krb5_ccache() { return get(); }

					bool find(std::string, std::string);
			};

			class principal {

				std::string _name;
				krb5_principal _pointer;
				context &_context;

				public:
					principal() = delete;

					principal(context& ctx): _pointer(nullptr), _context(ctx) { }
					principal(context& ctx, krb5_principal princ): _pointer(nullptr), _context(ctx) { if (!krb5_copy_principal(_context.get(), princ, &_pointer)) throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot copy printipal"); }
					principal(context& ctx, cache& ccache): _pointer(nullptr), _context(ctx) { krb5_cc_get_principal(_context.get(), ccache.get(), &_pointer); }

					principal(const principal&) = delete;
					principal& operator=(const principal&) = delete;
					principal(principal&&) = delete;
					principal& operator=(principal&&) = delete;
					~principal() { if (_pointer != nullptr) krb5_free_principal(_context.get(), _pointer); }
					
					krb5_principal get() {
						if (_pointer == nullptr) throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "principal not ready");
						return _pointer;
					}
					krb5_principal* getp() {
						if (_pointer == nullptr) throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "principal not ready");
						return &_pointer;
					}
					std::string name();
			};

			std::string _id;

			std::string _realm, _principal;

			context _context;
			keytab _keytab;
			cache _cache;

		public:
			kerberos() = delete;
			kerberos(std::string, std::string, std::string, std::string);
			~kerberos();

			void print(const int, const int);

			std::string cachePrincipal();
			bool isCacheValid();
			bool isValidKeytab();

			bool authenticate();
			bool authenticate(std::string);
		};
	}
}

#endif //KERBEROS_H_
