#pragma once

#include <mutex>
#include <cstring>
#include <cstdarg>
#include <variant>

#include <oci.h>

#include <axon/util.h>
#include <axon/database.h>

#define AXON_ORACLE_PREFETCH 100

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

		struct columninfo {
			text *name;	  // column name
			ub4 position;	// in the select statement the position
			ub4 type;		// data type
			ub4 size;		// size/length of the field
			ub4 memsize;	 // how much memory (bytes) was allocated
			void *indicator; // indicator of null value in field
			void *data;	  // pointer to the data
		};

		struct environment {
			// OK RAII
			environment() {
				_pointer = (OCIEnv *) 0;
				_uuid = axon::util::uuid();

				if (OCIEnvCreate((OCIEnv **) &_pointer, OCI_EVENTS|OCI_OBJECT|OCI_THREADED, NULL, NULL, NULL, NULL, 0, NULL) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize environment");
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~environment() {
				if (_pointer != (OCIEnv *) 0)
					OCIHandleFree(_pointer, (ub4) OCI_HTYPE_ENV);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCIEnv *get() {
				if (_pointer == (OCIEnv *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "environment not ready");
				return _pointer;
			}
			operator OCIEnv*() { return get(); }
			private:
				std::string _uuid;
				OCIEnv *_pointer;
		};

		struct context {
			// OK RAII
			context(environment *env) {
				_pointer = (OCISvcCtx *) 0;
				_uuid = axon::util::uuid();
				if (OCIHandleAlloc((dvoid *) env->get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~context() {
				if (_pointer != (OCISvcCtx *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SVCCTX);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCISvcCtx *get() {
				if (_pointer == (OCISvcCtx *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "context not allocated");
				return _pointer;
			}
			operator OCISvcCtx*() { return get(); }
			private:
				std::string _uuid;
				OCISvcCtx *_pointer;
		};

		struct error {
			// OK RAII
			error(environment *env) {
				_pointer = (OCIError *) 0;
				_uuid = axon::util::uuid();
				if (OCIHandleAlloc((dvoid *) env->get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~error() {
				if (_pointer != (OCIError *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_ERROR);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCIError *get() {
				if (_pointer == (OCIError *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "error not allocated");
				return _pointer;
			}
			operator OCIError*() { return get(); }
			std::string what(int status) {
				text errbuf[OCI_ERROR_MAXMSG_SIZE2];
				sb4 errcode = 0;

				std::stringstream s;

				switch(status)
				{
					case OCI_SUCCESS:
						s<<"Operation successful - no error detected";
						break;

					case OCI_SUCCESS_WITH_INFO:
						s<<"Error - OCI_SUCCESS_WITH_INFO";
						break;

					case OCI_NEED_DATA:
						s<<"Error - OCI_NEED_DATA";
						break;

					case OCI_NO_DATA:
						s<<"Error - OCI_NODATA";
						break;

					case OCI_ERROR:
						(void) OCIErrorGet((dvoid *) _pointer, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) OCI_ERROR_MAXMSG_SIZE2, OCI_HTYPE_ERROR);
						errbuf[strlen((char*)errbuf)-1] = 0;
						s<<"("<<errcode<<") "<<errbuf;
						break;

					case OCI_INVALID_HANDLE:
						s<<"Error - OCI_INVALID_HANDLE";
						break;

					case OCI_STILL_EXECUTING:
						s<<"Error - OCI_STILL_EXECUTE";
						break;

					case OCI_CONTINUE:
						s<<"Error - OCI_CONTINUE";
						break;

					default:
						break;
				}

				return s.str();
			}
			private:
				std::string _uuid;
				OCIError *_pointer;
		};

		struct server {
			// OK RAII
			server(environment *env) {
				_pointer = (OCIServer *) 0;
				_uuid = axon::util::uuid();
				if (OCIHandleAlloc((dvoid *) env->get(), (dvoid **) &_pointer, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~server() {
				if (_pointer != (OCIServer *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SERVER);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCIServer *get() {
				if (_pointer == (OCIServer *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "error not allocated");
				return _pointer;
			}
			operator OCIServer*() { return get(); }
			private:
				std::string _uuid;
				OCIServer *_pointer;
		};

		struct session {
			// OK RAII
			session(environment *env) {
				_pointer = (OCISession *) 0;
				_uuid = axon::util::uuid();
				if (OCIHandleAlloc((dvoid *) env->get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~session() {
				if (_pointer != (OCISession *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCISession *get() {
				if (_pointer == (OCISession *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "error not allocated");
				return _pointer;
			}
			operator OCISession*() { return get(); }
			private:
				std::string _uuid;
				OCISession *_pointer;
		};

		struct statement {
			// OK RAII
			statement(environment *env) {
				_pointer = (OCIStmt *) 0;
				_uuid = axon::util::uuid();
				if (OCIHandleAlloc((dvoid *) env->get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_STMT, 0, NULL) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				_addr = &_pointer;
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			~statement() {
				if (_pointer != (OCIStmt *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION);
				DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			}
			OCIStmt *get() {
				if (_pointer == (OCIStmt *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "error not allocated");
				return _pointer;
			}
			OCIStmt **ptr() { return _addr; }
			operator OCIStmt*() { return _pointer; }
			private:
				std::string _uuid;
				OCIStmt *_pointer;
				OCIStmt **_addr;
		};

		class oracle: public interface {

			environment _environment;
			context _context;
			error _error;
			server _server;
			session _session;
			statement _statement;

			std::string _uuid;

			std::string _hostname, _username, _password;
			int _port;

			bool _connected, _running, _dirty, _executed;
			unsigned int _prefetch, _fetched, _col_index, _col_count, _row_index, _row_count;
			std::vector<columninfo> _columns;

			std::vector<axon::database::bind> _bind;

			std::mutex _lock;

			void build();

			int prepare(int, va_list*, axon::database::bind*);
			void vbind(statement*, std::vector<axon::database::bind>&);

			protected:
				std::ostream& printer(std::ostream&);

			public:
				int _get_int(int) { return 0; };
				long _get_long(int) { return 0; };
				float _get_float(int) { return 0; };
				double _get_double(int) { return 0; };
				std::string _get_string(int) { return "D"; };

				oracle();
				oracle(const oracle &);
				~oracle();

				bool connect();
				bool connect(std::string, std::string, std::string);
				bool close();
				bool flush();

				bool ping();
				std::string version();

				bool transaction(trans_t);

				bool execute(const std::string);
				bool execute(const std::string, axon::database::bind, ...);

				bool query(const std::string);
				bool query(const std::string, axon::database::bind, ...);
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