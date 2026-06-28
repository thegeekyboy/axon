#include <memory>
#include <vector>
#include <any>

#include <axon.h>
#include <axon/util.h>
#include <axon/database2r.h>
#include <axon/oci.h>

namespace axon {

	namespace database2r {

		namespace oci {

			OCIEnv *environment::handle;
			std::mutex environment::lock;
			int environment::count = 0;

			environment::environment(): _id(axon::util::uuid())
			{
				BENCHMARK;

				{
					std::lock_guard<std::mutex> _lock(lock);
					if (count == 0)
					{
						handle = (OCIEnv *) 0;

						if (OCIEnvCreate((OCIEnv **) &handle, OCI_EVENTS | OCI_OBJECT | OCI_THREADED, nullptr, nullptr, nullptr, nullptr, 0, nullptr) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "[%s] cannot initialize environment", _id);
					}
					count++;
				}
			}

			environment::~environment()
			{
				BENCHMARK;

				std::lock_guard<std::mutex> _lock(lock);

				if (count == 1 && handle != (OCIEnv *) 0) OCIHandleFree(handle, (ub4) OCI_HTYPE_ENV);
				count--;
			}

			error::error()
			{
				BENCHMARK;

				_retcode = OCI_SUCCESS;
				_pointer = (OCIError *) 0;
				_id = axon::util::uuid();

				if (OCIHandleAlloc(axon::database2r::oci::environment::get(), (dvoid **) &_pointer, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate error object");
			}

			error::~error()
			{
				if (_pointer != (OCIError *) 0)
					OCIHandleFree(_pointer, (ub4) OCI_HTYPE_ERROR);
			}

			bool error::failed()
			{
				return (_retcode != OCI_SUCCESS && _retcode != OCI_SUCCESS_WITH_INFO);
			}

			std::string error::checker(OCIError *errhand, sword status)
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

			session::session(std::shared_ptr<axon::database2r::oci::context> ctx): _id(axon::util::uuid()), _context(ctx)
			{
				BENCHMARK;

				if (OCIHandleAlloc((dvoid *) axon::database2r::oci::environment::get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate session");
			}

			session::~session()
			{
				if (_pointer != (OCISession *) 0)
					OCIHandleFree((dvoid *) _pointer, (ub4) OCI_HTYPE_SESSION);
			}

			OCISession* session::get()
			{
				if (_pointer == (OCISession *) 0)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "session not allocated");

				return _pointer;
			}

			void session::connect(std::string_view username, std::string_view password)
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

				_connected = true;
			}

			void session::disconnect()
			{
				if (!_connected) return;

				if ((_error = OCISessionEnd(_context->get(), _error.get(), _pointer, (ub4) 0)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

				_connected = false;
			}

			connection::connection(): _context(std::make_shared<axon::database2r::oci::context>()), _session(_context), _id(axon::util::uuid())
			{
			}
 
			connection::~connection()
			{
				if (_connected)
				{
					try {
						disconnect();
					} catch (...) {
						ERRPRN("disconnect() failed in destructor");
					}
				}
			}

			bool connection::_connect(std::string_view dblink, std::string_view username, std::string_view password)
			{
				BENCHMARK;
				if ((_error = OCIServerAttach(_server.get(), _error.get(), (text*) dblink.data(), (sb4) dblink.size(), OCI_DEFAULT)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what().c_str());
 
				if ((_error = OCIAttrSet((dvoid*) _context->get(), OCI_HTYPE_SVCCTX, (dvoid*) _server.get(), 0, OCI_ATTR_SERVER, _error.get())).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
 
				_session.connect(username, password);
 
				_connected = true;

				return true;
			}

			void connection::connect(const std::string &tns, const std::string &username, const std::string &password)
			{
				if (_connected)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "already connected");
 
				if (!axon::util::validator::tns(tns) || !axon::util::validator::username(username) || password.empty())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "tns name, username and password required");

				_connect(tns, username, password);
			}
 
			void connection::connect(const std::string &hostname, const std::string &username, const std::string &password, const uint16_t port, const std::string &servicename)
			{
				if (_connected)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "already connected");
 
				if (hostname.empty() || username.empty() || password.empty() || servicename.empty())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "hostname, username and password required");

				std::string _dblink = "(DESCRIPTION=(ADDRESS=(PROTOCOL=tcp)(HOST=" + hostname + ")(PORT=" + std::to_string(port) + "))(CONNECT_DATA=(SERVICE_NAME=" + servicename + ")))";
				_connect(_dblink, username, password);
			}

			void connection::disconnect()
			{
				if (!_connected) return;
 
				try {
					_session.disconnect();
				} catch (axon::exception &e) {
					// ignore exception and continue to detach from server
				}
				WRNPRN("disconnecting from server");
				if ((_error = OCIServerDetach(_server.get(), _error.get(), OCI_DEFAULT)).failed())
					ERRPRN("OCIServerDetach: %s", _error.what().c_str());
 
				_connected = false;
			}

			statement::statement(std::shared_ptr<axon::database2r::oci::context> ctx): _id(axon::util::uuid()), _prepared(false), _pointer(nullptr), _context(ctx)
			{
				DBGPRN("[%s] %s", _id.c_str(), __PRETTY_FUNCTION__);
			}

			statement::~statement()
			{
				reset();
			}

			OCIStmt *statement::get()
			{
				if (_pointer == nullptr)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "statement not allocated");
				return _pointer;
			}

			void statement::prepare(std::string sql)
			{
				// https://docs.oracle.com/en/database/oracle/oracle-database/19/lnoci/statement-functions.html#GUID-DF585B90-58BA-45FC-B7CE-6F7F987C03B9
				if ((_error = OCIStmtPrepare2(_context->get(), &_pointer, _error.get(), (text*) sql.c_str(), sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				_prepared = true;
				_sql = sql;
			}

			int statement::bind(std::vector<axon::database2r::bind> &vars)
			{
				// READ: https://stackoverflow.com/questions/16883694/ocibindbypos-on-array-of-strings

				BENCHMARK;
				int index = 1, count = vars.size();

				for (auto &element : vars)
				{
					INFPRN("+ Index: %d of %d, Type Index: %s", index, count, element.type().name());

					if (element.type() == typeid(std::vector<std::string>))
					{
						// std::vector<std::string> data = std::any_cast<std::vector<std::string>>(element);
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "array bind not supported at index: %d", index); // temp until array bind is implemented
					}
					else if (element.type() == typeid(std::vector<double>))
					{
						// std::vector<double> data = std::any_cast<std::vector<double>>(element);
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "array bind not supported at index: %d", index); // temp until array bind is implemented
					}
					else if (element.type() == typeid(std::vector<int>))
					{
						// std::vector<int> data = std::any_cast<std::vector<int>>(element);
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "array bind not supported at index: %d", index); // temp until array bind is implemented
					}
					else if (element.type() == typeid(char*))
					{
						char *data = std::any_cast<char *>(element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(const char*))
					{
						const char *data = std::any_cast<const char *>(element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(unsigned char*))
					{
						unsigned char **data = std::any_cast<unsigned char*>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)(*data), strlen((char*)*data)+1, SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(std::string))
					{
						std::string *data = std::any_cast<std::string>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data->c_str(), data->size(), SQLT_CHR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(float))
					{
						float *data = std::any_cast<float>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(float), SQLT_FLT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(double))
					{
						double *data = std::any_cast<double>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(double), SQLT_BDOUBLE, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(int8_t))
					{
						int8_t *data = std::any_cast<int8_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int8_t), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					}
					else if (element.type() == typeid(int16_t))
					{
						int16_t *data = std::any_cast<int16_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int16_t), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					}
					else if (element.type() == typeid(int32_t))
					{
						int32_t *data = std::any_cast<int32_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int32_t), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					}
					else if (element.type() == typeid(uint32_t))
					{
						uint32_t *data = std::any_cast<uint32_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(uint32_t), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					}
					else if (element.type() == typeid(int64_t))
					{
						int64_t *data = std::any_cast<int64_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int64_t), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

					}
					else if (element.type() == typeid(uint64_t))
					{
						uint64_t *data = std::any_cast<uint64_t>(&element);
						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(uint64_t), SQLT_UIN, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else if (element.type() == typeid(bool))
					{
						// this is a hack to bind bool as unsigned char because OCI does not support bool type
						bool *data = std::any_cast<bool>(&element);
						_bool_temps.push_back(*data ? 1 : 0);
						int *intptr = &_bool_temps.back();

						OCIBind *bndp = nullptr;

						if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, intptr, sizeof(int), SQLT_INT, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
						
						// following is only supported from v23, need to implement some method to check version and use this if available
						// bool *data = std::any_cast<bool>(&element);
						// OCIBind *bndp = nullptr;

						// if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(bool), SQLT_BOL, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT)).failed())
						// 	throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
					}
					else
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);

					index++;
				}

				vars.clear();

				return count;
			}

			void statement::bind(OCISubscription *sbptr)
			{
				// Associate the statement with the subscription handle
				if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_STMT, sbptr, 0, OCI_ATTR_CHNF_REGHANDLE, _error)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
			}

			void statement::execute(axon::database2r::exec_type et)
			{
				BENCHMARK;

				if ((_error = OCIStmtExecute(_context->get(), _pointer, _error.get(), (et == exec_type::select)?0:1, 0, (OCISnapshot *) 0, (OCISnapshot *) 0, OCI_DEFAULT)).failed())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what() + " >> " + _sql);
			}

			void statement::reset()
			{
				if (_pointer != nullptr && _prepared)
				{
					OCIStmtRelease(_pointer, _error.get(), NULL, 0, OCI_DEFAULT);
					_pointer = nullptr;
					_prepared = false;
					_sql.clear();
					_bool_temps.clear();
				}
			}
		}
	}
}