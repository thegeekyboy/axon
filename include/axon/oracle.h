#pragma once

#include <mutex>
#include <cstring>
#include <cstdarg>

#include <oci.h>

#include <axon/database.h>

#define AXON_ORACLE_PREFETCH 100
//#define DBERR(rc, errhand, funcsig, funcoci) axon::exception(__FILE__, __LINE__, funcsig, getErrorMessage(rc, errhand))
// #define DBERR(rc, errhand, funcsig, funmsg) axon::exception(__FILE__, __LINE__, funcsig, std::string("nothing"))

namespace axon {

	namespace database {


		enum operation {
			none = 0x0,
			startup = 0x1,
			shutdown = 0x2,
			shutany = 0x3,
			dropdb = 0x4,
			unregister = 0x5,
			object = 0x6,
			querychange = 0x7
		};

		enum change {
			allops = 0x0,
			allrows = 0x1,
			insert = 0x2,
			update = 0x4,
			remove = 0x8,
			alter = 0x10,
			drop = 0x20,
			unknown = 0x40
		};

		typedef void (*ndfn) (dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
		typedef void (*cbfn) (operation, change, std::string, std::string);

		struct context {
			char sid[256];
			char username[256];
			char password[256];
			cbfn callback;
		};
		typedef context context;

		struct _colinfo {
			text *name;	  // column name
			ub4 position;	// in the select statement the position
			ub4 type;		// data type
			ub4 size;		// size/length of the field
			ub4 memsize;	 // how much memory (bytes) was allocated
			void *indicator; // indicator of null value in field
			void *data;	  // pointer to the data
		};

		class oracle:public interface {

			OCISvcCtx *_context;
			OCIError *_error;
			OCISession *_session;
			OCIStmt *_statement;
			OCIEnv *_environment;
			OCIServer *_server;
			OCISubscription *_subscription;

			std::string _sid, _username, _password;

			bool _connected, _subscribing, _running;
			std::string _uuid;
			unsigned int _prefetch;

			std::mutex _lock;

			int _port;
			context _ctx;

			bool _dirty;
			bool _executed;
			unsigned int _index;
			unsigned int _colidx;
			unsigned int _rownum;
			unsigned int _colcnt;
			unsigned int _fetched;
			std::vector<_colinfo> _col;

			struct rc {
				sword retval;
				sword sqlcode;
				std::string reason;
				OCIError *errhand;

				void check()
				{
					char errbuf[512];
					sb4 ec = 0;

					if (errhand == NULL)
					{
						reason = "OCIError is NULL so- nothing to see here!";
					}
					else
					{
						switch(retval)
						{
							case OCI_SUCCESS:
								reason = "Operation successful - no error detected";
								break;

							case OCI_SUCCESS_WITH_INFO:
								reason = "Error - OCI_SUCCESS_WITH_INFO";
								break;

							case OCI_NEED_DATA:
								reason = "Error - OCI_NEED_DATA";
								break;

							case OCI_NO_DATA:
								reason = "Error - OCI_NODATA";
								break;

							case OCI_ERROR:
								// (void) OCIErrorGet((dvoid *) errhand, (ub4) 1, (text *) NULL, &ec, (text*)errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
								reason = errbuf;
								sqlcode = ec;
								break;

							case OCI_INVALID_HANDLE:
								reason = "Error - OCI_INVALID_HANDLE";
								break;

							case OCI_STILL_EXECUTING:
								reason = "Error - OCI_STILL_EXECUTE";
								break;

							case OCI_CONTINUE:
								reason = "Error - OCI_CONTINUE";
								break;

							default:
								break;
						}
					}
				};

				inline rc& operator=(sword ec)
				{
					retval = ec;
					return *this;
				};

				inline bool operator!=(sword ec)
				{
					return retval != ec;
				};
			} _rc;

			using cb_helper = lambda<50, void, dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4>;

			bool build();
			int vbind(OCIStmt*, const std::string&, va_list*, axon::database::bind*);

			protected:
				std::ostream& printer(std::ostream&);

			public:
				oracle();
				~oracle();

				oracle(const oracle &);

				OCIError *getError();
				OCIEnv *getEnvironment();
				OCISvcCtx *getService();

				void checker(sword);
				static std::string checker(OCIError *, sword);

				bool connect();
				bool connect(std::string, std::string, std::string);
				bool close();
				bool flush();

				bool ping();
				std::string version();

				bool watch(std::string);
				bool watch(std::string, cbfn);
				bool unwatch();

				bool transaction(trans_t);

				bool execute(const std::string);
				bool execute(const std::string, axon::database::bind&, ...);

				bool query(const std::string);
				bool query(const std::string, axon::database::bind&, ...);
				bool query(const std::string, std::vector<std::string>);

				bool next();
				void done();

				std::string get(unsigned int);

				std::string& operator[] (char);
				int& operator[] (int);

				oracle& operator<<(int);
				oracle& operator<<(std::string&);

				oracle& operator>>(int&);
				oracle& operator>>(double&);
				oracle& operator>>(std::string&);
				oracle& operator>>(long&);

				static void driver(dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
		};
	}
}