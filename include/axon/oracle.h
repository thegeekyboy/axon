#ifndef AXON_ORACLE_H_
#define AXON_ORACLE_H_

#include <mutex>
#include <algorithm>
#include <cstring>
#include <cstdarg>

#include <oci.h>

#include <axon/util.h>
#include <axon/database2r.h>

#define AXON_DATABASE_ORACLE_PREFETCH 500

namespace axon {

	namespace database2r {

		class environment
		{
			std::string _uuid;
			static OCIEnv *handle;
			static std::mutex lock;
			static int count;

			public:
				environment()
				{
					BENCHMARK;
					_uuid = axon::util::uuid();

					{
						std::lock_guard<std::mutex> _lock(lock);
						if (count == 0)
						{
							handle = (OCIEnv *) 0;

							if (OCIEnvCreate((OCIEnv **) &handle, OCI_EVENTS|OCI_OBJECT|OCI_THREADED, nullptr, nullptr, nullptr, nullptr, 0, nullptr) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot initialize environment");
						}
						count++;
					}
				}
				~environment()
				{
					BENCHMARK;

					std::lock_guard<std::mutex> _lock(lock);

					if (count == 1 && handle != (OCIEnv *) 0) OCIHandleFree(handle, (ub4) OCI_HTYPE_ENV);
					count--;
				}

				static OCIEnv *get()
				{
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
				error()
				{
					BENCHMARK;

					_retcode = OCI_SUCCESS;
					_pointer = (OCIError *) 0;
					_uuid = axon::util::uuid();

					if (OCIHandleAlloc(axon::database2r::environment::get(), (dvoid **) &_pointer, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate error object");
				}

				~error()
				{
					if (_pointer != (OCIError *) 0)
						OCIHandleFree(_pointer, (ub4) OCI_HTYPE_ERROR);
				}

				bool failed()
				{
					return (_retcode != OCI_SUCCESS && _retcode != OCI_SUCCESS_WITH_INFO);
				}

				axon::database2r::error& operator= (int retcode)
				{
					_retcode = retcode;
					return *this;
				}

				bool operator==(int retcode)
				{
					return (_retcode == retcode);
				}

				bool operator!=(int retcode)
				{
					return (_retcode != retcode);
				}

				std::string what() { return checker(_pointer, _retcode); }

				OCIError *get()
				{
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
					size_t len = 0;
					std::string retval;

					if (errhand == nullptr)
					{
						retval = "OCIError is nullptr so- nothing to see here!";
					}
					else
					{
						switch(status)
						{
							case OCI_SUCCESS_WITH_INFO:
								retval = "Error - OCI_SUCCESS_WITH_INFO";
								break;

							case OCI_NEED_DATA:
								retval = "Error - OCI_NEED_DATA";
								break;

							case OCI_NO_DATA:
								retval = "Error - OCI_NODATA";
								break;

							case OCI_ERROR:
								(void) OCIErrorGet((dvoid *) errhand, (ub4) 1, (text *) nullptr, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
								len = strlen((char*)errbuf);
								if (len) errbuf[len-1] = 0;
								retval = "Error - (" + std::to_string(errcode) + ") " + reinterpret_cast<char*>(errbuf);
								break;

							case OCI_INVALID_HANDLE:
								retval = "Error - OCI_INVALID_HANDLE";
								break;

							case OCI_STILL_EXECUTING:
								retval = "Error - OCI_STILL_EXECUTE";
								break;

							case OCI_CONTINUE:
								retval = "Error - OCI_CONTINUE";
								break;

							default:
								break;
						}
					}

					return retval;
				}
		};

		class context
		{
			std::string _uuid;
			OCISvcCtx *_pointer { nullptr };

			public:
				context()
				{
					BENCHMARK;

					_uuid = axon::util::uuid();

					if (OCIHandleAlloc((dvoid *) axon::database2r::environment::get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate context");
				}

				~context()
				{
					if (_pointer != (OCISvcCtx *) 0)
						OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SVCCTX);
				}

				OCISvcCtx *get()
				{
					if (_pointer == (OCISvcCtx *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "context not allocated");
					return _pointer;
				}

				operator OCISvcCtx*() { return get(); }
		};

		class server
		{
			std::string _uuid;
			OCIServer *_pointer;

			public:
				server()
				{
					BENCHMARK;

					_pointer = (OCIServer *) 0;
					_uuid = axon::util::uuid();

					if (OCIHandleAlloc((dvoid *) axon::database2r::environment::get(), (dvoid **) &_pointer, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate server");
				}

				~server()
				{
					if (_pointer != (OCIServer *) 0)
						OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SERVER);
				}

				OCIServer *get()
				{
					if (_pointer == (OCIServer *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "server not allocated");
					return _pointer;
				}

				operator OCIServer*() { return get(); }
		};

		class session
		{
			std::string _uuid;
			bool _connected { false };
			OCISession *_pointer { nullptr };
			std::shared_ptr<axon::database2r::context> _context;
			axon::database2r::error _error;
			std::string _username, _password;

			public:
				session(std::shared_ptr<axon::database2r::context> ctx)
				{
					BENCHMARK;

					_uuid = axon::util::uuid();
					_context = ctx;

					if (OCIHandleAlloc((dvoid *) axon::database2r::environment::get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate session");
				}

				~session()
				{
					if (_pointer != (OCISession *) 0)
						OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION);
				}

				OCISession *get()
				{
					if (_pointer == (OCISession *) 0)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "session not allocated");

					return _pointer;
				}

				operator OCISession*() { return get(); }

				void connect(std::string username, std::string password)
				{
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

				void disconnect()
				{
					if ((_error = OCISessionEnd(_context->get(), _error.get(), _pointer, (ub4) 0)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
		};

		class oracle: public connector {

			//// classes and structs
			struct column {
				std::string name;           // column name

				ub4 position;               // position in the select statement
				ub4 type;                   // data type
				sb1 scale { 0 };			// number of digits to the right of the decimal point
				size_t size { 0 };         // size/length of the field returned by OCI_ATTR_DATA_SIZE
				size_t count { 0 };         // record/row count

				std::vector<uint8_t> data;  // pointer to the data
				std::vector<sb2> indicator; // indicator of null value in field
				std::vector<ub2> rlen;      // actual byte length per row, set by OCI after fetch

				column(text *nn, ub4 nl, ub4 ps, ub4 tt, sb1 sk, ub4 sz): name(reinterpret_cast<const char*>(nn), nl), position(ps), type(tt), scale(sk), size(sz) {
					WRNPRN("pos: %2u, type: %3u, size: %4zu, name: %s", position, type, size, name.c_str());
				}

				~column() = default;

				void allocate(size_t ct) {

				    if (ct == 0) return;

					count = ct;

					size_t needed = (size + 1) * count;
					if (data.size() != needed)
					{
						data.resize(needed);
						indicator.resize(count);
						rlen.resize(count, 0);
					}

					reset();
				}

				void reset() {
					std::fill(data.begin(), data.end(), 0);
					std::fill(indicator.begin(), indicator.end(), 0);
					std::fill(rlen.begin(), rlen.end(), 0);
				}
			};

			class statement
			{
				std::string _uuid, _sql;
				bool _prepared;

				OCIStmt *_pointer;

				axon::database2r::environment _environment;
				std::shared_ptr<axon::database2r::context> _context;
				axon::database2r::error _error;

				public:
					statement() = delete;
					statement(std::shared_ptr<axon::database2r::context>);
					~statement();

					OCIStmt *get();
					operator OCIStmt*() { return get(); }

					void prepare(std::string);
					void bind(OCISubscription *);
					int bind(std::vector<axon::database2r::bind>&);

					void execute(axon::database2r::exec_type);
					void reset();
			};
			//// End Subclass

			axon::database2r::environment _environment;
			std::shared_ptr<axon::database2r::context> _context;
			axon::database2r::error _error;
			axon::database2r::server _server;
			axon::database2r::session _session;

			std::shared_ptr<axon::database2r::oracle::statement> _statement;

			bool _connected, _running, _executed;

			std::string _hostname, _username, _password;

			std::vector<oracle::column> _columns;

			inline const oracle::column& _col(size_t position) const {
				if (position >= _columns.size())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");
				return _columns[position];
			}

			void _get_column_info();
			void _get_column_details(uint16_t);
			axon::column_type _attach_column_data(oracle::column&, uint16_t);

			int _get_int(size_t, int);
			bool _get_bool(size_t, int);
			long _get_long(size_t, int);
			float _get_float(size_t, int);
			double _get_double(size_t, int);
			std::string _get_string(size_t, int);

			public:

				oracle();
				oracle(const axon::database2r::oracle&) = delete;
				~oracle();

				bool connect() override;
				bool connect(std::string, std::string, std::string) override;
				bool close() override;
				bool flush() override;

				bool ping() override;
				std::string version() override;

				bool transaction(axon::database2r::trans_t) override;

				bool execute(const std::string) override;
				bool query(const std::string) override;
				void fetch(axon::recordset2r&, int) override;
				void done() override;

				std::string& operator[] (char);
				int& operator[] (int);
		};
	}
}

#endif
