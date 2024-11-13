#include <iostream>
#include <string>

#include <string.h>
#include <krb5/krb5.h>

#include <axon.h>
#include <axon/util.h>
#include <axon/kerberos.h>

namespace axon
{
	namespace authentication
	{
		kerberos::kerberos(std::string keytab, std::string cache, std::string realm, std::string principal)
		{
			if (!axon::util::exists(keytab))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "keytab file does not exist");

			_keytab_file = keytab;
			_cache_file = cache;
			_realm = realm;
			_principal = principal;

			_cache = NULL;
			_keytab = NULL;
			_ctx = NULL;

			_id = axon::util::uuid();
		}

		kerberos::~kerberos()
		{
			if (_cache)
				krb5_cc_close(_ctx, _cache);

			if (_keytab)
				krb5_kt_close(_ctx, _keytab);

			if (_ctx)
				krb5_free_context(_ctx);

			DBGPRN("[%s] %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		std::string kerberos::_errstr(long int i)
		{
			switch(i)
			{
				case KRB5KDC_ERR_NONE:  return "No error"; break;
				case KRB5KDC_ERR_NAME_EXP:  return "Client's entry in database has expired"; break;
				case KRB5KDC_ERR_SERVICE_EXP:  return "Server's entry in database has expired"; break;
				case KRB5KDC_ERR_BAD_PVNO:  return "Requested protocol version not supported"; break;
				case KRB5KDC_ERR_C_OLD_MAST_KVNO:  return "Client's key is encrypted in an old master key"; break;
				case KRB5KDC_ERR_S_OLD_MAST_KVNO:  return "Server's key is encrypted in an old master key"; break;
				case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN:  return "Client not found in Kerberos database"; break;
				case KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN:  return "Server not found in Kerberos database"; break;
				case KRB5KDC_ERR_PRINCIPAL_NOT_UNIQUE:  return "Principal has multiple entries in Kerberos database"; break;
				case KRB5KDC_ERR_NULL_KEY:  return "Client or server has a null key"; break;
				case KRB5KDC_ERR_CANNOT_POSTDATE:  return "Ticket is ineligible for postdating"; break;
				case KRB5KDC_ERR_NEVER_VALID:  return "Requested effective lifetime is negative or too short"; break;
				case KRB5KDC_ERR_POLICY:  return "KDC policy rejects request"; break;
				case KRB5KDC_ERR_BADOPTION:  return "KDC can't fulfill requested option"; break;
				case KRB5KDC_ERR_ETYPE_NOSUPP:  return "KDC has no support for encryption type"; break;
				case KRB5KDC_ERR_SUMTYPE_NOSUPP:  return "KDC has no support for checksum type"; break;
				case KRB5KDC_ERR_PADATA_TYPE_NOSUPP:  return "KDC has no support for padata type"; break;
				case KRB5KDC_ERR_TRTYPE_NOSUPP:  return "KDC has no support for transited type"; break;
				case KRB5KDC_ERR_CLIENT_REVOKED:  return "Clients credentials have been revoked"; break;
				case KRB5KDC_ERR_SERVICE_REVOKED:  return "Credentials for server have been revoked"; break;
				case KRB5KDC_ERR_TGT_REVOKED:  return "TGT has been revoked"; break;
				case KRB5KDC_ERR_CLIENT_NOTYET:  return "Client not yet valid - try again later"; break;
				case KRB5KDC_ERR_SERVICE_NOTYET:  return "Server not yet valid - try again later"; break;
				case KRB5KDC_ERR_KEY_EXP:  return "Password has expired"; break;
				case KRB5KDC_ERR_PREAUTH_FAILED:  return "Preauthentication failed"; break;
				case KRB5KDC_ERR_PREAUTH_REQUIRED:  return "Additional pre-authentication required"; break;
				case KRB5KDC_ERR_SERVER_NOMATCH:  return "Requested server and ticket don't match"; break;
				case KRB5PLACEHOLD_30:  return "KRB5 error code 30"; break;
				case KRB5KRB_AP_ERR_BAD_INTEGRITY:  return "Decrypt integrity check failed"; break;
				case KRB5KRB_AP_ERR_TKT_EXPIRED:  return "Ticket expired"; break;
				case KRB5KRB_AP_ERR_TKT_NYV:  return "Ticket not yet valid"; break;
				case KRB5KRB_AP_ERR_REPEAT:  return "Request is a replay"; break;
				case KRB5KRB_AP_ERR_NOT_US:  return "The ticket isn't for us"; break;
				case KRB5KRB_AP_ERR_BADMATCH:  return "Ticket/authenticator don't match"; break;
				case KRB5KRB_AP_ERR_SKEW:  return "Clock skew too great"; break;
				case KRB5KRB_AP_ERR_BADADDR:  return "Incorrect net address"; break;
				case KRB5KRB_AP_ERR_BADVERSION:  return "Protocol version mismatch"; break;
				case KRB5KRB_AP_ERR_MSG_TYPE:  return "Invalid message type"; break;
				case KRB5KRB_AP_ERR_MODIFIED:  return "Message stream modified"; break;
				case KRB5KRB_AP_ERR_BADORDER:  return "Message out of order"; break;
				case KRB5KRB_AP_ERR_ILL_CR_TKT:  return "Illegal cross-realm ticket"; break;
				case KRB5KRB_AP_ERR_BADKEYVER:  return "Key version is not available"; break;
				case KRB5KRB_AP_ERR_NOKEY:  return "Service key not available"; break;
				case KRB5KRB_AP_ERR_MUT_FAIL:  return "Mutual authentication failed"; break;
				case KRB5KRB_AP_ERR_BADDIRECTION:  return "Incorrect message direction"; break;
				case KRB5KRB_AP_ERR_METHOD:  return "Alternative authentication method required"; break;
				case KRB5KRB_AP_ERR_BADSEQ:  return "Incorrect sequence number in message"; break;
				case KRB5KRB_AP_ERR_INAPP_CKSUM:  return "Inappropriate type of checksum in message"; break;
				case KRB5KRB_AP_PATH_NOT_ACCEPTED:  return "Policy rejects transited path"; break;
				case KRB5KRB_ERR_RESPONSE_TOO_BIG:  return "Response too big for UDP, retry with TCP"; break;
				case KRB5PLACEHOLD_53:  return "KRB5 error code 53"; break;
				case KRB5PLACEHOLD_54:  return "KRB5 error code 54"; break;
				case KRB5PLACEHOLD_55:  return "KRB5 error code 55"; break;
				case KRB5PLACEHOLD_56:  return "KRB5 error code 56"; break;
				case KRB5PLACEHOLD_57:  return "KRB5 error code 57"; break;
				case KRB5PLACEHOLD_58:  return "KRB5 error code 58"; break;
				case KRB5PLACEHOLD_59:  return "KRB5 error code 59"; break;
				case KRB5KRB_ERR_GENERIC:  return "Generic error (see e-text)"; break;
				case KRB5KRB_ERR_FIELD_TOOLONG:  return "Field is too long for this implementation"; break;
				case KRB5PLACEHOLD_82:  return "KRB5 error code 82"; break;
				case KRB5PLACEHOLD_83:  return "KRB5 error code 83"; break;
				case KRB5PLACEHOLD_84:  return "KRB5 error code 84"; break;
				case KRB5PLACEHOLD_87:  return "KRB5 error code 87"; break;
				case KRB5PLACEHOLD_88:  return "KRB5 error code 88"; break;
				case KRB5PLACEHOLD_89:  return "KRB5 error code 89"; break;
				case KRB5PLACEHOLD_92:  return "KRB5 error code 92"; break;
				case KRB5PLACEHOLD_94:  return "KRB5 error code 94"; break;
				case KRB5PLACEHOLD_95:  return "KRB5 error code 95"; break;
				case KRB5PLACEHOLD_96:  return "KRB5 error code 96"; break;
				case KRB5PLACEHOLD_97:  return "KRB5 error code 97"; break;
				case KRB5PLACEHOLD_98:  return "KRB5 error code 98"; break;
				case KRB5PLACEHOLD_99:  return "KRB5 error code 99"; break;
				case KRB5PLACEHOLD_101:  return "KRB5 error code 101"; break;
				case KRB5PLACEHOLD_102:  return "KRB5 error code 102"; break;
				case KRB5PLACEHOLD_103:  return "KRB5 error code 103"; break;
				case KRB5PLACEHOLD_104:  return "KRB5 error code 104"; break;
				case KRB5PLACEHOLD_105:  return "KRB5 error code 105"; break;
				case KRB5PLACEHOLD_106:  return "KRB5 error code 106"; break;
				case KRB5PLACEHOLD_107:  return "KRB5 error code 107"; break;
				case KRB5PLACEHOLD_108:  return "KRB5 error code 108"; break;
				case KRB5PLACEHOLD_109:  return "KRB5 error code 109"; break;
				case KRB5PLACEHOLD_110:  return "KRB5 error code 110"; break;
				case KRB5PLACEHOLD_111:  return "KRB5 error code 111"; break;
				case KRB5PLACEHOLD_112:  return "KRB5 error code 112"; break;
				case KRB5PLACEHOLD_113:  return "KRB5 error code 113"; break;
				case KRB5PLACEHOLD_114:  return "KRB5 error code 114"; break;
				case KRB5PLACEHOLD_115:  return "KRB5 error code 115"; break;
				case KRB5PLACEHOLD_116:  return "KRB5 error code 116"; break;
				case KRB5PLACEHOLD_117:  return "KRB5 error code 117"; break;
				case KRB5PLACEHOLD_118:  return "KRB5 error code 118"; break;
				case KRB5PLACEHOLD_119:  return "KRB5 error code 119"; break;
				case KRB5PLACEHOLD_120:  return "KRB5 error code 120"; break;
				case KRB5PLACEHOLD_121:  return "KRB5 error code 121"; break;
				case KRB5PLACEHOLD_122:  return "KRB5 error code 122"; break;
				case KRB5PLACEHOLD_123:  return "KRB5 error code 123"; break;
				case KRB5PLACEHOLD_124:  return "KRB5 error code 124"; break;
				case KRB5PLACEHOLD_125:  return "KRB5 error code 125"; break;
				case KRB5PLACEHOLD_126:  return "KRB5 error code 126"; break;
				case KRB5PLACEHOLD_127:  return "KRB5 error code 127"; break;
				case KRB5_ERR_RCSID:  return "(RCS Id string for the krb5 error table)"; break;
				case KRB5_LIBOS_BADLOCKFLAG:  return "Invalid flag for file lock mode"; break;
				case KRB5_LIBOS_CANTREADPWD:  return "Cannot read password"; break;
				case KRB5_LIBOS_BADPWDMATCH:  return "Password mismatch"; break;
				case KRB5_LIBOS_PWDINTR:  return "Password read interrupted"; break;
				case KRB5_PARSE_ILLCHAR:  return "Illegal character in component name"; break;
				case KRB5_PARSE_MALFORMED:  return "Malformed representation of principal"; break;
				case KRB5_CONFIG_CANTOPEN:  return "Can't open/find Kerberos configuration file"; break;
				case KRB5_CONFIG_BADFORMAT:  return "Improper format of Kerberos configuration file"; break;
				case KRB5_CONFIG_NOTENUFSPACE:  return "Insufficient space to return complete information"; break;
				case KRB5_BADMSGTYPE:  return "Invalid message type specified for encoding"; break;
				case KRB5_CC_BADNAME:  return "Credential cache name malformed"; break;
				case KRB5_CC_UNKNOWN_TYPE:  return "Unknown credential cache type"; break;
				case KRB5_CC_NOTFOUND:  return "Matching credential not found"; break;
				case KRB5_CC_END:  return "End of credential cache reached"; break;
				case KRB5_NO_TKT_SUPPLIED:  return "Request did not supply a ticket"; break;
				case KRB5KRB_AP_WRONG_PRINC:  return "Wrong principal in request"; break;
				case KRB5KRB_AP_ERR_TKT_INVALID:  return "Ticket has invalid flag set"; break;
				case KRB5_PRINC_NOMATCH:  return "Requested principal and ticket don't match"; break;
				case KRB5_KDCREP_MODIFIED:  return "KDC reply did not match expectations"; break;
				case KRB5_KDCREP_SKEW:  return "Clock skew too great in KDC reply"; break;
				case KRB5_IN_TKT_REALM_MISMATCH:  return "Client/server realm mismatch in initial ticket request"; break;
				case KRB5_PROG_ETYPE_NOSUPP:  return "Program lacks support for encryption type"; break;
				case KRB5_PROG_KEYTYPE_NOSUPP:  return "Program lacks support for key type"; break;
				case KRB5_WRONG_ETYPE:  return "Requested encryption type not used in message"; break;
				case KRB5_PROG_SUMTYPE_NOSUPP:  return "Program lacks support for checksum type"; break;
				case KRB5_REALM_UNKNOWN:  return "Cannot find KDC for requested realm"; break;
				case KRB5_SERVICE_UNKNOWN:  return "Kerberos service unknown"; break;
				case KRB5_KDC_UNREACH:  return "Cannot contact any KDC for requested realm"; break;
				case KRB5_NO_LOCALNAME:  return "No local name found for principal name"; break;
				case KRB5_MUTUAL_FAILED:  return "Mutual authentication failed"; break;
				case KRB5_RC_TYPE_EXISTS:  return "Replay cache type is already registered"; break;
				case KRB5_RC_MALLOC:  return "No more memory to allocate (in replay cache code)"; break;
				case KRB5_RC_TYPE_NOTFOUND:  return "Replay cache type is unknown"; break;
				case KRB5_RC_UNKNOWN:  return "Generic unknown RC error"; break;
				case KRB5_RC_REPLAY:  return "Message is a replay"; break;
				case KRB5_RC_IO:  return "Replay I/O operation failed XXX"; break;
				case KRB5_RC_NOIO:  return "Replay cache type does not support non-volatile storage"; break;
				case KRB5_RC_PARSE:  return "Replay cache name parse/format error"; break;
				case KRB5_RC_IO_EOF:  return "End-of-file on replay cache I/O"; break;
				case KRB5_RC_IO_MALLOC:  return "No more memory to allocate (in replay cache I/O code)"; break;
				case KRB5_RC_IO_PERM:  return "Permission denied in replay cache code"; break;
				case KRB5_RC_IO_IO:  return "I/O error in replay cache i/o code"; break;
				case KRB5_RC_IO_UNKNOWN:  return "Generic unknown RC/IO error"; break;
				case KRB5_RC_IO_SPACE:  return "Insufficient system space to store replay information"; break;
				case KRB5_TRANS_CANTOPEN:  return "Can't open/find realm translation file"; break;
				case KRB5_TRANS_BADFORMAT:  return "Improper format of realm translation file"; break;
				case KRB5_LNAME_CANTOPEN:  return "Can't open/find lname translation database"; break;
				case KRB5_LNAME_NOTRANS:  return "No translation available for requested principal"; break;
				case KRB5_LNAME_BADFORMAT:  return "Improper format of translation database entry"; break;
				case KRB5_CRYPTO_INTERNAL:  return "Cryptosystem internal error"; break;
				case KRB5_KT_BADNAME:  return "Key table name malformed"; break;
				case KRB5_KT_UNKNOWN_TYPE:  return "Unknown Key table type"; break;
				case KRB5_KT_NOTFOUND:  return "Key table entry not found"; break;
				case KRB5_KT_END:  return "End of key table reached"; break;
				case KRB5_KT_NOWRITE:  return "Cannot write to specified key table"; break;
				case KRB5_KT_IOERR:  return "Error writing to key table"; break;
				case KRB5_NO_TKT_IN_RLM:  return "Cannot find ticket for requested realm"; break;
				case KRB5DES_BAD_KEYPAR:  return "DES key has bad parity"; break;
				case KRB5DES_WEAK_KEY:  return "DES key is a weak key"; break;
				case KRB5_BAD_ENCTYPE:  return "Bad encryption type"; break;
				case KRB5_BAD_KEYSIZE:  return "Key size is incompatible with encryption type"; break;
				case KRB5_BAD_MSIZE:  return "Message size is incompatible with encryption type"; break;
				case KRB5_CC_TYPE_EXISTS:  return "Credentials cache type is already registered."; break;
				case KRB5_KT_TYPE_EXISTS:  return "Key table type is already registered."; break;
				case KRB5_CC_IO:  return "Credentials cache I/O operation failed XXX"; break;
				case KRB5_FCC_PERM:  return "Credentials cache file permissions incorrect"; break;
				case KRB5_FCC_NOFILE:  return "No credentials cache found"; break;
				case KRB5_FCC_INTERNAL:  return "Internal credentials cache error"; break;
				case KRB5_CC_WRITE:  return "Error writing to credentials cache"; break;
				case KRB5_CC_NOMEM:  return "No more memory to allocate (in credentials cache code)"; break;
				case KRB5_CC_FORMAT:  return "Bad format in credentials cache"; break;
				case KRB5_INVALID_FLAGS:  return "Invalid KDC option combination (library internal error) [for dual tgt library calls]"; break;
				case KRB5_NO_2ND_TKT:  return "Request missing second ticket [for dual tgt library calls]"; break;
				case KRB5_NOCREDS_SUPPLIED:  return "No credentials supplied to library routine"; break;
				case KRB5_SENDAUTH_BADAUTHVERS:  return "Bad sendauth version was sent"; break;
				case KRB5_SENDAUTH_BADAPPLVERS:  return "Bad application version was sent (via sendauth)"; break;
				case KRB5_SENDAUTH_BADRESPONSE:  return "Bad response (during sendauth exchange)"; break;
				case KRB5_SENDAUTH_REJECTED:  return "Server rejected authentication (during sendauth exchange)"; break;
				case KRB5_PREAUTH_BAD_TYPE:  return "Unsupported preauthentication type"; break;
				case KRB5_PREAUTH_NO_KEY:  return "Required preauthentication key not supplied"; break;
				case KRB5_PREAUTH_FAILED:  return "Generic preauthentication failure"; break;
				case KRB5_RCACHE_BADVNO:  return "Unsupported replay cache format version number"; break;
				case KRB5_CCACHE_BADVNO:  return "Unsupported credentials cache format version number"; break;
				case KRB5_KEYTAB_BADVNO:  return "Unsupported key table format version number"; break;
				case KRB5_PROG_ATYPE_NOSUPP:  return "Program lacks support for address type"; break;
				case KRB5_RC_REQUIRED:  return "Message replay detection requires rcache parameter"; break;
				case KRB5_ERR_BAD_HOSTNAME:  return "Hostname cannot be canonicalized"; break;
				case KRB5_ERR_HOST_REALM_UNKNOWN:  return "Cannot determine realm for host"; break;
				case KRB5_SNAME_UNSUPP_NAMETYPE:  return "Conversion to service principal undefined for name type"; break;
				case KRB5KRB_AP_ERR_V4_REPLY:  return "Initial Ticket response appears to be Version 4 error"; break;
				case KRB5_REALM_CANT_RESOLVE:  return "Cannot resolve KDC for requested realm"; break;
				case KRB5_TKT_NOT_FORWARDABLE:  return "Requesting ticket can't get forwardable tickets"; break;
				case KRB5_FWD_BAD_PRINCIPAL:  return "Bad principal name while trying to forward credentials"; break;
				case KRB5_GET_IN_TKT_LOOP:  return "Looping detected inside krb5_get_in_tkt"; break;
				case KRB5_CONFIG_NODEFREALM:  return "Configuration file does not specify default realm"; break;
				case KRB5_SAM_UNSUPPORTED:  return "Bad SAM flags in obtain_sam_padata"; break;
				case KRB5_KT_NAME_TOOLONG:  return "Keytab name too long"; break;
				case KRB5_KT_KVNONOTFOUND:  return "Key version number for principal in key table is incorrect"; break;
				case KRB5_APPL_EXPIRED:  return "This application has expired"; break;
				case KRB5_LIB_EXPIRED:  return "This Krb5 library has expired"; break;
				case KRB5_CHPW_PWDNULL:  return "New password cannot be zero length"; break;
				case KRB5_CHPW_FAIL:  return "Password change failed"; break;
				case KRB5_KT_FORMAT:  return "Bad format in keytab"; break;
				case KRB5_NOPERM_ETYPE:  return "Encryption type not permitted"; break;
				case KRB5_CONFIG_ETYPE_NOSUPP:  return "No supported encryption types (config file error?)"; break;
				case KRB5_OBSOLETE_FN:  return "Program called an obsolete, deleted function"; break;
				case KRB5_EAI_FAIL:  return "unknown getaddrinfo failure"; break;
				case KRB5_EAI_NODATA:  return "no data available for host/domain name"; break;
				case KRB5_EAI_NONAME:  return "host/domain name not found"; break;
				case KRB5_EAI_SERVICE:  return "service name unknown"; break;
				case KRB5_ERR_NUMERIC_REALM:  return "Cannot determine realm for numeric host address"; break;
			}

			return "undefined error";
		}

		void kerberos::print(const int errnum, const int ln)
		{
			const char *errstr = krb5_get_error_message(_ctx, errnum);

			std::cerr<<ln<<": "<<errstr<<std::endl; // how to print this log?
			krb5_free_error_message(_ctx, errstr);
		}

		bool kerberos::init()
		{
			int retval;

			if (!axon::util::exists(_keytab_file))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "keytab file is not accessible");

			if (!axon::util::iswritable(_cache_file))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cache file is not accessible " + _cache_file);

			if ((retval = krb5_init_context(&_ctx)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize context - " + _errstr(retval));

			if ((retval = krb5_kt_resolve(_ctx, _keytab_file.c_str(), &_keytab)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot resolve keytab - " + _errstr(retval));

			if ((retval = krb5_cc_resolve(_ctx, _cache_file.c_str(), &_cache)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot open/initialize kerberos cache - " + _errstr(retval));

			return true;
		}

		std::string kerberos::cachePrincipal()
		{
			krb5_principal principal = 0;
			char *defname = NULL;
			std::string defprinc;

			krb5_cc_get_principal(_ctx, _cache, &principal);
			krb5_unparse_name(_ctx, principal, &defname);

			if (defname)
				defprinc = defname;
			else
				defprinc = "";

			krb5_free_unparsed_name(_ctx, defname);
			krb5_free_principal(_ctx, principal);

			return defprinc;
		}

		bool kerberos::isCacheValid()
		{
			krb5_cc_cursor cursor = NULL;
			krb5_creds creds = { };
			bool isValid = false;
			long int retval;
			time_t now = time(NULL);

			if ((retval = krb5_cc_start_seq_get(_ctx, _cache, &cursor)))
				return false;//throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _cache_file + "-cannot start iterate over cache cursor - " + _errstr(retval));

			std::string sprinc = "krbtgt/" + _realm + "@" + _realm;

			while (true)
			{
				krb5_free_cred_contents(_ctx, &creds);

				retval = krb5_cc_next_cred(_ctx, _cache, &cursor, &creds);

				if (retval == KRB5_CC_FORMAT)
					break;
				else if (retval == KRB5_CC_END)
					break;

				char *sName = NULL, *cName = NULL;

				krb5_unparse_name(_ctx, creds.server, &sName);
				krb5_unparse_name(_ctx, creds.client, &cName);

				if (sprinc == sName && _principal == cName && creds.times.endtime > now)
					isValid = true;

				DBGPRN("server: %s, client: %s, now: %ld, expire: %d", sName, cName, now, creds.times.endtime);

				krb5_free_unparsed_name(_ctx, sName);
				krb5_free_unparsed_name(_ctx, cName);
			}

			krb5_cc_end_seq_get(_ctx, _cache, &cursor);
			krb5_free_cred_contents(_ctx, &creds);

			return isValid;
		}

		bool kerberos::isValidKeytab()
		{
			long int retval = 0;
			bool isValid = false;
			std::string sprinc = "krbtgt/" + _realm + "@" + _realm;

			krb5_kt_cursor cursor = NULL;
			krb5_keytab_entry entry = { };

			if ((retval = krb5_kt_start_seq_get(_ctx, _keytab, &cursor)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot start iterate over keytab cursor - " + _errstr(retval));

			while ((retval = krb5_kt_next_entry(_ctx, _keytab, &entry, &cursor)) == 0)
			{
				char *name;
				krb5_unparse_name(_ctx, entry.principal, &name);

				if (_principal == name)
					isValid = true;

				krb5_free_unparsed_name(_ctx, name);
				krb5_free_keytab_entry_contents(_ctx, &entry);
			}

			krb5_kt_end_seq_get(_ctx, _keytab, &cursor);

			return isValid;
		}

		void kerberos::renew()
		{
			long int retval;

			krb5_principal principal;
			krb5_creds creds = { };

			// memset(&creds, 0, sizeof(creds));
			if ((retval = krb5_parse_name(_ctx, _principal.c_str(), &principal)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot parse principal string - " + _errstr(retval));

			if ((retval = krb5_get_init_creds_keytab(_ctx, &creds, principal, _keytab, 0, NULL, NULL)))
			{
				krb5_free_principal(_ctx, principal);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize keytab credentials - " + _errstr(retval));
			}

			if ((retval = krb5_cc_initialize(_ctx, _cache, principal)))
			{
				krb5_free_principal(_ctx, principal);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,"cannot initialize cache - " + _errstr(retval));
			}

			if ((retval = krb5_cc_store_cred(_ctx, _cache, &creds)))
			{
				krb5_free_principal(_ctx, principal);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot store credentials - " + _errstr(retval));
			}

			// krb5_free_creds(_ctx, &creds); <- donno why this dont work!! throws a double free exception.
			krb5_free_cred_contents(_ctx, &creds); // so using this instead, dont seem to get any memory leak
			krb5_free_principal(_ctx, principal);
		}
	}
}
