#include <regex>

#include <axon.h>
#include <axon/oracle.h>

namespace axon {

	namespace database {

		void oracle::build()
		{
			int rc = 0;
			
			_prefetch = 200;
			_fetched = 0;

			_row_index = 0;
			_row_count = 0;
			_col_index = 0;
			_col_count = 0;

			ub4 paramcnt;
			OCIParam *_param = (OCIParam *) 0;

			DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);

			if ((rc = OCIAttrGet((CONST dvoid *) _statement.get(), (ub4) OCI_HTYPE_STMT, (void *) &paramcnt, (ub4 *) 0, (ub4) OCI_ATTR_PARAM_COUNT, _error.get())) < OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			DBGPRN("Number of columns in query is %d", paramcnt);
			_col_count = paramcnt;
			_dirty = true;

			for (ub4 i = 1; i <= paramcnt; i++)
			{
				OCIDefine *dptr = (OCIDefine *) 0;
				ub4 namelen = 128;

				_columns.push_back({NULL, i, 0, 0, 0, NULL, NULL});

				if ((rc = OCIParamGet((dvoid *) _statement.get(), (ub4) OCI_HTYPE_STMT, (OCIError *) _error.get(), (dvoid **) &_param, (ub4) i)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid **) &_columns[i-1].name, (ub4 *) &namelen, (ub4) OCI_ATTR_NAME, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, &_columns[i-1].type, (ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &_columns[i-1].size, (ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				_columns[i-1].name[namelen] = 0;
				_columns[i-1].size += 1;

				DBGPRN("Column %d: name = %s, type = %u, size = %u", i, _columns[i-1].name , _columns[i-1].type, _columns[i-1].size);

				/*
					#define SQLT_CHR         1 (ORANET TYPE) character string
					#define SQLT_NUM         2   (ORANET TYPE) oracle numeric
					#define SQLT_LNG         8                           long
					#define SQLT_DAT        12          date in oracle format
					#define SQLT_AFC        96                Ansi fixed char
					#define SQLT_TIMESTAMP 187                      TIMESTAMP
				*/

				switch (_columns[i-1].type)
				{
					case SQLT_CHR:
					case SQLT_STR:
					case SQLT_VCS:
					case SQLT_AFC:
					case SQLT_AVC:
						_columns[i-1].memsize = (sizeof(char) * _columns[i-1].size * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement.get(), &dptr, _error.get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_STR, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
						break;

					case SQLT_NUM:
					case SQLT_LNG:
						_columns[i-1].memsize = (sizeof(OCINumber) * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement.get(), &dptr, _error.get(), i, _columns[i-1].data, sizeof(OCINumber), SQLT_VNU, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
						break;

					case SQLT_DAT:
						_columns[i-1].memsize = (sizeof(char) * _columns[i-1].size * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement.get(), &dptr, _error.get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_DAT, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
						break;

					case SQLT_TIMESTAMP:
					case SQLT_TIMESTAMP_TZ:
						_columns[i-1].memsize = (sizeof(char) * _columns[i-1].size * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement.get(), &dptr, _error.get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_DAT, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
						break;

					default:
						DBGPRN("Unknown type: %d", _columns[i-1].type);
				}
			}
		}

		int oracle::prepare(int count, va_list *list, axon::database::bind *first)
		{
			axon::database::bind *element = first;

			_bind.clear();

			for (int index = 0; index < count; index++)
			{
				if (element == nullptr)
					break;

				NOTPRN("* Index: %d of %d, Type Index: %s", index+1, count, element->type().name());

				_bind.push_back(*element);

				element = va_arg(*list, axon::database::bind*);
			}

			return count;
		}

		void oracle::vbind(statement *stmt, std::vector<axon::database::bind> &vars)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int rc, index = 1;

			for (auto &element : vars)
			{
				NOTPRN("+ Index: %d of %d, Type Index: %s", index, vars.size(), element.type().name());

				if (element.type() == typeid(std::vector<std::string>))
				{
					// std::vector<std::string> data = std::any_cast<std::vector<std::string>>(element);
				}
				else if (element.type() == typeid(std::vector<double>))
				{
					// std::vector<double> data = std::any_cast<std::vector<double>>(element);
				}
				else if (element.type() == typeid(std::vector<int>))
				{
					// std::vector<int> data = std::any_cast<std::vector<int>>(element);
				}
				else if (element.type() == typeid(char*))
				{
					char *data = std::any_cast<char *>(element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char **data = std::any_cast<unsigned char*>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (text*)(*data), strlen((char*)*data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(std::string))
				{
					std::string *data = std::any_cast<std::string>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (text*)data->c_str(), data->size()+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(float))
				{
					float *data = std::any_cast<float>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(float), SQLT_FLT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(double))
				{
					double *data = std::any_cast<double>(&element);
					OCIBind *bndp = (OCIBind *) 0;

					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(double), SQLT_BDOUBLE, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				}
				else if (element.type() == typeid(int8_t))
				{
					int8_t *data = std::any_cast<int8_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int8_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				}
				else if (element.type() == typeid(int16_t))
				{
					int16_t *data = std::any_cast<int16_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int16_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				}
				else if (element.type() == typeid(int32_t))
				{
					int32_t *data = std::any_cast<int32_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int32_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				}
				else if (element.type() == typeid(uint32_t))
				{
					uint32_t *data = std::any_cast<uint32_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(uint32_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				}
				else if (element.type() == typeid(int64_t))
				{
					int64_t *data = std::any_cast<int64_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((rc = OCIBindByPos(stmt->get(), &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int64_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				}
				else if (element.type() == typeid(uint64_t))
				{
					// uint64_t data = std::any_cast<uint64_t>(element);
				}
				else if (element.type() == typeid(bool))
				{
					// bool data = std::any_cast<bool>(element);
				}
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);

				index++;
			}
		}

		oracle::oracle():
		_context(&_environment),
		_error(&_environment),
		_server(&_environment),
		_session(&_environment),
		_statement(&_environment)
		{
			DBGPRN("axon::database - constructing");

			_port = 7776;
			_prefetch = 200;

			_connected = false;
			_running = false;
			_executed = false;

			_row_index = 1;
			_col_index = 0;
		}

		oracle::~oracle()
		{
			close();
			DBGPRN("axon::database - destroying");
		}

		bool oracle::connect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_hostname.size() <= 1 || _username.size() <= 1 || _password.size() <= 1)
				return false;

			if (!_connected)
			{
				sword rc;

				if ((rc = OCIServerAttach(_server.get(), _error.get(), (text *) _hostname.c_str(), (sb4) _hostname.size(), (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				/* set attribute server context in the service context */
				if ((rc = OCIAttrSet((dvoid *) _context.get(), (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _server.get(), (ub4) 0, (ub4) OCI_ATTR_SERVER, (OCIError *) _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				/* allocate a user context handle */
				if ((rc = OCIAttrSet((dvoid *) _session.get(), (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _username.c_str()), (ub4) _username.size(), OCI_ATTR_USERNAME, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				if ((rc = OCIAttrSet((dvoid *) _session.get(), (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _password.c_str()), (ub4) _password.size(), OCI_ATTR_PASSWORD, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				
				if ((rc = OCISessionBegin(_context.get(), _error.get(), _session.get(), OCI_CRED_RDBMS, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				/* Allocate a statement handle */
				if ((rc = OCIAttrSet((dvoid *) _context, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _session.get(), (ub4) 0, OCI_ATTR_SESSION, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				// OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 52, (dvoid **) &tmp);

				_connected = true;
			}

			return _connected;
		}

		bool oracle::connect(std::string hostname, std::string username, std::string password)
		{
			_hostname = hostname;
			_username = username;
			_password = password;

			return connect();
		}

		bool oracle::close()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			int rc;

			if (_connected)
			{
				if ((rc = OCISessionEnd(_context.get(), _error.get(), _session.get(), (ub4) 0)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				if ((rc = OCIServerDetach(_server.get(), _error.get(), (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
			}

			_connected = false;
			
			return _connected;
		}

		bool oracle::flush()
		{
			execute("COMMIT");
			return true;
		}

		bool oracle::ping()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			
			if (_connected)
			{
				int rc;

				if ((rc = OCIPing(_context.get(), _error.get(), (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
			}

			return true;
		}

		std::string oracle::version()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_connected)
			{
				sword rc;
				char sver[1024];

				if((rc = OCIServerVersion(_context.get(), _error.get(), (text*) sver, (ub4) sizeof(sver), (ub1) OCI_HTYPE_SVCCTX)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				_version = sver;
			}

			return _version;
		}

		bool oracle::transaction([[maybe_unused]] trans_t ttype)
		{
			return true;
		}

		bool oracle::execute(const std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			{
				std::lock_guard<std::mutex> lock(_lock);
				
				int rc;
				statement stmt(&_environment);

				_running = true;

				if ((rc = OCIStmtPrepare(stmt.get(), _error.get(), (text*) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				vbind(&stmt, _bind);

				if ((rc = OCIStmtExecute(_context.get(), stmt.get(), _error.get(), 1, 0, (OCISnapshot *) 0, (OCISnapshot *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				_running = false;
			}

			return true;
		}

		bool oracle::execute(const std::string sql, axon::database::bind first, ...)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			unsigned int count = _vcount(sql);

			std::va_list list;

			if (!_running)
			{
				_running = true;
				
				va_start(list, &first);
				prepare(count, &list, &first);
				va_end(list);

				_running = false;
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "already busy with one query. please release first");

			return execute(sql);
		}

		bool oracle::query(const std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int rc = 0;

			if (!_running)
			{
				_running = true;
				
				if ((rc = OCIAttrSet((dvoid *) _statement.get(), OCI_HTYPE_STMT, (void*) &_prefetch, sizeof(int), OCI_ATTR_PREFETCH_ROWS, _error.get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				if ((rc = OCIStmtPrepare2(_context.get(), _statement.ptr(), _error.get(), (const OraText *) sql.c_str(), sql.size(), 0, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
				// if ((rc = OCIStmtPrepare(_statement.get(), _error.get(), (const OraText *) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
				_executed = false;
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "already busy with one query. please release first");

			return true;
		}

		bool oracle::query(const std::string sql, axon::database::bind first, ...)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			unsigned int count = _vcount(sql);

			std::va_list list;

			if (!_running)
			{
				_running = true;
				
				va_start(list, &first);
				prepare(count, &list, &first);
				va_end(list);

				_running = false;
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "already busy with one query. please release first");

			return query(sql);
		}

		bool oracle::query(const std::string sql, std::vector<std::string> data)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			return true;
		}

		bool oracle::next()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int rc;

			if (!_running)
				return false;

			if (!_executed)
			{
				_col_index = 0;
				_row_index = 0;

				vbind(&_statement, _bind);

				if ((rc = OCIStmtExecute(_context.get(), _statement.get(), _error.get(), 0, 0, NULL, NULL, OCI_DEFAULT)) != OCI_SUCCESS) // prefetch is set to 0
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

				build();

				_executed = true;
			}

			DBGPRN("_fetched: %d, _row_index: %d", _fetched, _row_index);

			if (_row_index+1 < _fetched)
			{
				_row_index++;
			}
			else
			{
				_fetched = 0;
				_row_index = 0;

				rc = OCIStmtFetch2(_statement.get(), _error.get(), _prefetch, OCI_DEFAULT, 0, OCI_DEFAULT);
				OCIAttrGet(_statement.get(), OCI_HTYPE_STMT, (void*) &_fetched, NULL, OCI_ATTR_ROWS_FETCHED, _error.get());

				if (rc == OCI_SUCCESS && rc == OCI_NO_DATA && _fetched == 0)
					return false;

				if (_fetched == 0)
					return false;
			}

			_col_index = 0;
			return true;
		}

		void oracle::done()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int rc = 0;

			if (!_running)
				return;

			if (_dirty)
			{
				for (unsigned int x = 0; x < _col_count; x++)
				{
					if (_columns[x].indicator)
						free(_columns[x].indicator);

					if (_columns[x].data)
						free(_columns[x].data);
				}

				_dirty = false;
			}

			if ((rc = OCIStmtRelease(_statement.get(), _error.get(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			_running = false;
		}

		std::string oracle::get(unsigned int i)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			return "value";
		}

		std::string& oracle::operator[](char i)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			_throwawaystr.erase();
			return _throwawaystr;
		}

		int& oracle::operator[] (int i)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			_throwawayint = i;
			return _throwawayint;
		}

		oracle& oracle::operator<<(int value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			
			_bind.push_back(value);
			
			return *this;
		}

		oracle& oracle::operator<<(std::string& value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			
			_bind.push_back(value);
			
			return *this;
		}

		oracle& oracle::operator>>(int &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_col_index > _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column out of bounds");
			value = _get_int(_col_index++);

			return *this;
		}

		oracle& oracle::operator>>(double &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_col_index > _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column out of bounds");
			value = _get_double(_col_index++);

			return *this;
		}

		oracle& oracle::operator>>(std::string &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_col_index > _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column out of bounds");
			value = _get_string(_col_index++);

			return *this;
		}

		oracle& oracle::operator>>(long &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_col_index > _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column out of bounds");
			value = _get_long(_col_index++);

			return *this;
		}

		std::ostream& oracle::printer(std::ostream &stream)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			return stream;
		}
	}
}
