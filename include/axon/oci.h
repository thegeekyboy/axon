#ifndef AXON_OCI_H_
#define AXON_OCI_H_

#include <oci.h>

namespace axon 
{
	namespace database2r
	{
		namespace oci {

			struct column
			{
				std::string name;           // column name

				ub4 position;               // position in the select statement
				ub4 type;                   // data type
				sb1 scale { 0 };			// number of digits to the right of the decimal point
				size_t size { 0 };          // size/length of the field returned by OCI_ATTR_DATA_SIZE
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

			class environment
			{
				std::string _id;
				static OCIEnv *handle;
				static std::mutex lock;
				static int count;

				public:
					environment();
					~environment();

					static OCIEnv *get()
					{
						if (handle == (OCIEnv *) 0)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "environment not ready");
						return handle;
					}

					operator OCIEnv*() { return get(); }
			};

			class error {

				std::string _id;
				OCIError *_pointer;
				int _retcode;

				public:
					error();
					~error();

					bool failed();

					axon::database2r::oci::error& operator= (int retcode)
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

					static std::string checker(OCIError*, sword);
			};

			class context
			{
				std::string _id;
				OCISvcCtx *_pointer { nullptr };

				public:
					context()
					{
						BENCHMARK;

						_id = axon::util::uuid();

						if (OCIHandleAlloc((dvoid *) axon::database2r::oci::environment::get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
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
				std::string _id;
				OCIServer *_pointer;

				public:
					server()
					{
						BENCHMARK;

						_pointer = (OCIServer *) 0;
						_id = axon::util::uuid();

						if (OCIHandleAlloc((dvoid *) axon::database2r::oci::environment::get(), (dvoid **) &_pointer, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
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
				std::string _id;
				bool _connected { false };
				OCISession *_pointer { nullptr };

				std::shared_ptr<axon::database2r::oci::context> _context;
				axon::database2r::oci::error _error;
				
				std::string _username, _password;

				public:
					session(std::shared_ptr<axon::database2r::oci::context>);
					~session();

					session(const session&) = delete;
					session& operator=(const session&) = delete;

					OCISession *get();

					operator OCISession*() { return get(); }

					void connect(std::string_view, std::string_view);
					void disconnect();
			};

			class connection
			{
				std::shared_ptr<axon::database2r::oci::context> _context;

				axon::database2r::oci::error _error;
				axon::database2r::oci::server _server;
				axon::database2r::oci::session _session;

				std::string _id;
				std::string _connection_string;

				bool _connected { false };
				bool _connect(std::string_view, std::string_view, std::string_view);

				public:
					connection();
					~connection();

					connection(const connection&) = delete;
					connection& operator=(const connection&) = delete;

					void connect(const std::string&, const std::string&, const std::string&, const uint16_t, const std::string&);
					void connect(const std::string&, const std::string&, const std::string&);
					void disconnect();

					OCISvcCtx *ctx() { return _context->get(); }
					OCIError *error() { return _error.get(); }
					std::shared_ptr<axon::database2r::oci::context> context() { return _context; }

					bool connected() const { return _connected; }
			};

			class statement
			{
				std::string _id, _sql;
				bool _prepared;

				OCIStmt *_pointer;

				std::shared_ptr<axon::database2r::oci::context> _context;
				axon::database2r::oci::error _error;

				std::vector<int> _bool_temps;

				public:
					statement() = delete;
					statement(std::shared_ptr<axon::database2r::oci::context>);
					~statement();

					OCIStmt *get();
					operator OCIStmt*() { return get(); }

					void prepare(std::string);
					void bind(OCISubscription *);
					int bind(std::vector<axon::database2r::bind>&);

					void execute(axon::database2r::exec_type);
					void reset();
			};
		}
	}
}

#endif