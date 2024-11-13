#pragma once

#include <mutex>
#include <cstring>
#include <cstdarg>

#include <oci.h>

#include <axon/util.h>
#include <axon/database.h>

#define AXON_DATABASE_ORACLE_PREFETCH 500

namespace axon {

	namespace database {

		struct colinfo {
			text *name;		// column name
			ub4 position;	// in the select statement the position
			ub4 type;		// data type
			ub4 size;		// size/length of the field
			ub4 memsize;	// how much memory (bytes) was allocated
			void *indicator;// indicator of null value in field
			void *data;		// pointer to the data
		};
		typedef colinfo colinfo;

		class environment {
			// OK RAII
			std::string _uuid;
			static OCIEnv *handle;
			static std::mutex lock;
			static int count;

			public:
				environment() {

					_uuid = axon::util::uuid();

					{
						std::lock_guard<std::mutex> _lock(lock);
						if (count == 0)
						{
							handle = (OCIEnv *) 0;

							if (OCIEnvCreate((OCIEnv **) &handle, OCI_EVENTS|OCI_OBJECT|OCI_THREADED, NULL, NULL, NULL, NULL, 0, NULL) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize environment");
							DBGPRN("\033[0;31m[%s] environment allocating\033[0m\n", axon::timer::iso8601().c_str());
						}
						count++;
					}

					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				~environment() {

					if (count == 1)
					{
						if (handle != (OCIEnv *) 0) {
							DBGPRN("\033[0;31m[%s] environment destroying\033[0m\n", axon::timer::iso8601().c_str());
							OCIHandleFree(handle, (ub4) OCI_HTYPE_ENV);
						}
					}
					count--;
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				OCIEnv *get() {

					if (handle == (OCIEnv *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "environment not ready");
					return handle;
				}
				operator OCIEnv*() { return get(); }
		};

		class error {

			std::string _uuid;
			OCIError *_pointer;
			int _retcode;

			public:
				error() {
					axon::database::environment env;

					_retcode = 0;
					_pointer = (OCIError *) 0;
					_uuid = axon::util::uuid();

					if (OCIHandleAlloc(env.get(), (dvoid **) &_pointer, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate error object");
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				~error() {
					if (_pointer != (OCIError *) 0)
						OCIHandleFree(_pointer, (ub4) OCI_HTYPE_ERROR);
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}

				bool failed() {
					return (_retcode != OCI_SUCCESS && _retcode != OCI_SUCCESS_WITH_INFO);
				}

				axon::database::error& operator= (int retcode) {
					_retcode = retcode;
					return *this;
				}

				bool operator==(int retcode) {
					return (_retcode == retcode);
				}

				bool operator!=(int retcode) {
					return (_retcode != retcode);
				}

				std::string what() { return checker(_pointer, _retcode); }
				OCIError *get() {
					if (_pointer == (OCIError *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "error not allocated");
					return _pointer;
				}
				operator OCIError*() { return get(); }
				std::string what(int errnum) { return checker(_pointer, errnum); }
				static std::string checker(OCIError *errhand, sword status)
				{
					text errbuf[512];
					sb4 errcode = 0;

					std::stringstream s;

					if (errhand == NULL)
					{
						s<<"OCIError is NULL so- nothing to see here!";
					}
					else
					{
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
								(void) OCIErrorGet((dvoid *) errhand, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
								errbuf[strlen((char*)errbuf)-1] = 0;
								s<<"Error - ("<<errcode<<") "<<errbuf;
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
					}

					return s.str();
				}
		};

		class context {
			// OK RAII
			std::string _uuid;
			OCISvcCtx *_pointer;
			public:
				context() {
					axon::database::environment env;

					_pointer = (OCISvcCtx *) 0;
					_uuid = axon::util::uuid();

					if (OCIHandleAlloc((dvoid *) env.get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
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
		};

		class server {
			// OK RAII
			std::string _uuid;
			OCIServer *_pointer;
			public:
				server() {
					axon::database::environment env;

					_pointer = (OCIServer *) 0;
					_uuid = axon::util::uuid();

					if (OCIHandleAlloc((dvoid *) env.get(), (dvoid **) &_pointer, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate server");
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				~server() {
					if (_pointer != (OCIServer *) 0)
						OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SERVER);
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				OCIServer *get() {
					if (_pointer == (OCIServer *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "server not allocated");
					return _pointer;
				}
				operator OCIServer*() { return get(); }
		};

		class session {
			// OK RAII
			std::string _uuid;
			OCISession *_pointer;
			axon::database::context *_context;
			axon::database::error _error;
			bool _connected;
			std::string _username, _password;

			public:
				session(axon::database::context &ctx)
				{
					axon::database::environment env;

					_pointer = (OCISession *) 0;
					_connected = false;
					_uuid = axon::util::uuid();
					_context = &ctx;

					if (OCIHandleAlloc((dvoid *) env.get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate session");

					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				~session() {
					if (_pointer != (OCISession *) 0)
						OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION);
					DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
				}
				OCISession *get() {
					if (_pointer == (OCISession *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "session not allocated");
				}
				operator OCISession*() { return get(); }
				void connect(std::string username, std::string password) {
					_username = username;
					_password = password;

					// set user attribute to session
					if ((_error = OCIAttrSet((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _username.c_str()), (ub4) _username.size(), OCI_ATTR_USERNAME, _error.get())).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					// set password attribute to session
					if ((_error = OCIAttrSet((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _password.c_str()), (ub4) _password.size(), OCI_ATTR_PASSWORD, _error.get())).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					// begin a session
					if ((_error = OCISessionBegin(_context->get(), _error.get(), _pointer, OCI_CRED_RDBMS, OCI_DEFAULT)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					// set session attribute to service context
					if ((_error = OCIAttrSet((dvoid *) _context->get(), (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _pointer, (ub4) 0, OCI_ATTR_SESSION, _error.get())).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				void disconnect() {
					if ((_error = OCISessionEnd(_context->get(), _error.get(), _pointer, (ub4) 0)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
		};

		class statement {
			// OK RAII
			std::string _uuid, _sql;
			bool _prepared;

			OCIStmt *_pointer;

			axon::database::environment _environment;
			axon::database::context *_context;
			axon::database::error _error;

			public:
				statement() = delete;
				statement(axon::database::context&);
				~statement();

				OCIStmt *get();
				operator OCIStmt*() { return get(); }

				void prepare(std::string);
				int bind(std::vector<axon::database::bind>&);
				void bind(OCISubscription *);
				void execute(axon::database::exec_type);
				void clear();
		};

		class resultset {

			std::string _uuid, _table;

			bool _dirty;
			int _row_count, _row_index, _col_count, _col_index, _prefetch, _fetched;
			std::vector<axon::database::colinfo> _columns;

			std::shared_ptr<axon::database::statement> _statement;
			axon::database::error _error;

			public:
				resultset() = delete;
				resultset(std::shared_ptr<axon::database::statement>);
				~resultset();

				bool next();
				void done();

				unsigned int column_count() { return _col_count; }
				int column_type(int position) { return _columns[position].type; }

				std::string get(int);

				int get_int(int);
				long get_long(int);
				float get_float(int);
				double get_double(int);
				std::string get_string(int);

				bool operator++(int) { return next(); }
		};

		class oracle:public interface {

			axon::database::environment _environment;
			axon::database::context _context;
			axon::database::error _error;
			axon::database::server _server;
			axon::database::session _session;

			std::shared_ptr<axon::database::statement> _statement;

			bool _connected, _running, _executed;

			std::string _hostname, _username, _password;

			int _rowidx, _colidx;

			int _vbind(int count, va_list *list, axon::database::bind *first);

			std::unique_ptr<axon::database::resultset> _resultset;

			protected:
				std::ostream& printer(std::ostream&) override;

			public:
				int _get_int(int) override;
				long _get_long(int) override;
				float _get_float(int) override;
				double _get_double(int) override;
				std::string _get_string(int) override;

				oracle();
				oracle(const axon::database::oracle&) = delete;
				~oracle();

				bool connect() override;
				bool connect(std::string, std::string, std::string) override;
				bool close() override;
				bool flush() override;

				bool ping() override;
				std::string version() override;

				bool transaction(axon::database::trans_t) override;

				bool execute(const std::string) override;

				bool query(const std::string) override;
				bool query(const std::string, std::vector<std::string>);
				bool next() override;
				void done() override;

				std::unique_ptr<axon::database::resultset> make_resultset();

				std::string get(unsigned int);
				axon::database::context *get_context() { return &_context; };

				std::string& operator[] (char);
				int& operator[] (int);

				oracle& operator<<(int) override;
				oracle& operator<<(long) override;
				oracle& operator<<(long long) override;
				oracle& operator<<(float) override;
				oracle& operator<<(double) override;
				oracle& operator<<(std::string&) override;
				oracle& operator<<(axon::database::bind&) override;

				oracle& operator>>(int&) override;
				oracle& operator>>(long&) override;
				oracle& operator>>(float&) override;
				oracle& operator>>(double&) override;
				oracle& operator>>(std::string&) override;
		};
	}
}
