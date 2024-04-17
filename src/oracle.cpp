#include <regex>

#include <axon.h>
#include <axon/oracle.h>

namespace axon {

	namespace database {

		resultset::resultset(statement *stmt, error *err)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_uuid = axon::util::uuid();
			int rc = 0;
			
			_prefetch = 500;
			_fetched = 0;
			_statement = stmt;
			_error = err;

			_row_index = 0;
			_row_count = 0;
			_col_index = 0;
			_col_count = 0;

			ub4 paramcnt;
			OCIParam *_param = (OCIParam *) 0;

			DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);

			if ((rc = OCIAttrGet((CONST dvoid *) stmt->get(), (ub4) OCI_HTYPE_STMT, (void *) &paramcnt, (ub4 *) 0, (ub4) OCI_ATTR_PARAM_COUNT, err->get())) < OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));

			DBGPRN("Number of columns in query is %d", paramcnt);
			_col_count = paramcnt;
			_dirty = true;

			for (ub4 i = 1; i <= paramcnt; i++)
			{
				OCIDefine *dptr = (OCIDefine *) 0;
				ub4 namelen = 128;

				_columns.push_back({NULL, i, 0, 0, 0, NULL, NULL});

				if ((rc = OCIParamGet((dvoid *) stmt->get(), (ub4) OCI_HTYPE_STMT, (OCIError *) err->get(), (dvoid **) &_param, (ub4) i)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid **) &_columns[i-1].name, (ub4 *) &namelen, (ub4) OCI_ATTR_NAME, err->get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, &_columns[i-1].type, (ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE, err->get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &_columns[i-1].size, (ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE, err->get())) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));

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

						if ((rc = OCIDefineByPos(stmt->get(), &dptr, err->get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_STR, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
						break;

					case SQLT_NUM:
					case SQLT_LNG:
						_columns[i-1].memsize = (sizeof(OCINumber) * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(stmt->get(), &dptr, err->get(), i, _columns[i-1].data, sizeof(OCINumber), SQLT_VNU, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
						break;

					case SQLT_DAT:
						_columns[i-1].memsize = (sizeof(char) * _columns[i-1].size * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(stmt->get(), &dptr, err->get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_DAT, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
						break;

					case SQLT_TIMESTAMP:
					case SQLT_TIMESTAMP_TZ:
						_columns[i-1].memsize = (sizeof(char) * _columns[i-1].size * _prefetch);
						_columns[i-1].data = malloc(_columns[i-1].memsize);
						std::memset(_columns[i-1].data, 0, _columns[i-1].memsize);

						_columns[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(stmt->get(), &dptr, err->get(), i, _columns[i-1].data, _columns[i-1].size, SQLT_DAT, _columns[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err->what(rc));
						break;

					default:
						DBGPRN("Unknown type: %d", _columns[i-1].type);
				}
			}
		}

		resultset::~resultset()
		{
			WRNPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			done();
		}

		bool resultset::next()
		{
			int rc;

			DBGPRN("_fetched: %d, _row_index: %d", _fetched, _row_index);

			if (_row_index+1 < _fetched)
			{
				_row_index++;
			}
			else
			{
				_fetched = 0;
				_row_index = 0;

				axon::timer ctm(__PRETTY_FUNCTION__);

				rc = OCIStmtFetch2(_statement->get(), _error->get(), _prefetch, OCI_DEFAULT, 0, OCI_DEFAULT);
				OCIAttrGet(_statement->get(), OCI_HTYPE_STMT, (void*) &_fetched, NULL, OCI_ATTR_ROWS_FETCHED, _error->get());

				if (rc == OCI_SUCCESS && rc == OCI_NO_DATA && _fetched == 0)
					return false;

				if (_fetched == 0)
					return false;
			}

			_col_index = 0;

			return true;
		}

		void resultset::done()
		{
			if (_dirty)
			{
				for (int x = 0; x < _col_count; x++)
				{
					if (_columns[x].indicator)
						free(_columns[x].indicator);

					if (_columns[x].data)
						free(_columns[x].data);
				}

				_dirty = false;
			}
		}

		std::string resultset::get(int i)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			std::string value;

			DBGPRN("_index: %d, _col_count: %d, i: %d, col.type: %d", 0, _col_count, i, _columns[_col_index].type);

			if (i >= _col_count)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Column out of bounds");

			switch (_columns[i].type)
			{
				case SQLT_AFC:
				case SQLT_CHR:
					value = reinterpret_cast<char const*>((((text *) _columns[i].data)+(_row_index*_columns[i].size)));
					break;

				case SQLT_DAT:
				case SQLT_TIMESTAMP:
					{
						// detail why I did the following can be found at https://www.sqlines.com/oracle/datatypes/date
						char temp[64];
						char *raw;

						raw = ((char*) _columns[i].data) + (_row_index*_columns[i].size);

						sprintf(temp, "%02d-%02d-%02d%02d %02d:%02d:%02d", 
									((int)raw[3]), ((int)raw[2]), ((int)raw[0])-100, ((int)raw[1])-100, 
									((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
						value = temp;
					}
					break;

				case SQLT_NUM:
					{
						double temp;
						//OCINumberToInt(_error, (((OCINumber *) _columns[_colidx].data)+_row_index), sizeof(int), OCI_NUMBER_SIGNED, &temp);
						OCINumberToReal(_error->get(), (((OCINumber *) _columns[i].data)+_row_index), sizeof(double), &temp);
						value = std::to_string(temp);
					}
					break;

				default:
					throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Variable type is not compatable with row type");
			}

			return value;
		}

		int resultset::get_int(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int value = 0;

			DBGPRN("_index: %d, _col_count: %d, i: %d, col.type: %d<>%d", 0, _col_count, position, _columns[position].type, SQLT_NUM);

			if (position >= _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (_columns[position].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			OCINumberToInt(_error->get(), (((OCINumber *) _columns[position].data)+_row_index), sizeof(int), OCI_NUMBER_SIGNED, &value);

			return value;
		}

		long resultset::get_long(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			long value = 0;

			DBGPRN("_index: %d, _col_count: %d, i: %d, col.type: %d", 0, _col_count, position, _columns[_col_index].type);

			if (position >= _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (_columns[position].type == SQLT_DAT || _columns[position].type == SQLT_TIMESTAMP || _columns[position].type == SQLT_TIMESTAMP_TZ)
			{
				char *raw;

				raw = ((char*) _columns[position].data) + (_row_index*_columns[position].size);

				if (raw[0] == 0)
					value = 0;
				else
				{
					const char *format = "%Y %m %d %H %M %S";
					char dv[128];
					std::tm t;

					sprintf(dv, "%d %d %d %d %d %d",
					(int((((unsigned char) raw[0]) - 100) * 100)+(((unsigned char) raw[1]) - 100)),
					((int) raw[2]),
					((int) raw[3]),
					((int) raw[4])-1,
					((int) raw[5])-1,
					((int) raw[6])-1);

					strptime(dv, format, &t);
					value = mktime(&t);
				}
			}
			else if (_columns[position].type == SQLT_NUM)
				OCINumberToInt(_error->get(), (((OCINumber *) _columns[position].data)+_row_index), sizeof(long), OCI_NUMBER_SIGNED, &value);
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			return value;
		}

		float resultset::get_float(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			float value = 0;

			DBGPRN("_index: %d, _col_count: %d, i: %d, col.type: %d", 0, _col_count, position, _columns[position].type);

			if (position >= _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (_columns[position].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			OCINumberToReal(_error->get(), (((OCINumber *) _columns[position].data)+_row_index), sizeof(float), &value);

			return value;
		}

		double resultset::get_double(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			double value = 0;

			DBGPRN("_index: %d, _col_count: %d, i: %d, col.type: %d", 0, _col_count, position, _columns[position].type);

			if (position >= _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (_columns[position].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			OCINumberToReal(_error->get(), (((OCINumber *) _columns[position].data)+_row_index), sizeof(double), &value);

			return value;
		}

		std::string resultset::get_string(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			std::string value;

			DBGPRN("_index: %d, column count: %d, position: %d, type: %d, size: %d", 0, _col_count, position, _columns[position].type, _columns[position].size);

			if (position >= _col_count)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (_columns[position].type == SQLT_DAT || _columns[position].type == SQLT_TIMESTAMP || _columns[position].type == SQLT_TIMESTAMP_TZ)
			{
				char temp[64];
				char *raw;

				raw = ((char*) _columns[position].data) + (_row_index*_columns[position].size);

				if (raw[0] == 0)
					sprintf(temp, "0000-00-00 00:00:00");
				else
					sprintf(temp, "%02d%02d-%02d-%02d %02d:%02d:%02d", 
						((unsigned char)raw[0])-100, ((unsigned char)raw[1])-100, 
						((int)raw[2]), ((int)raw[3]),
							((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
				value = temp;
			}
			else if (_columns[position].type == SQLT_AFC || _columns[position].type != SQLT_CHR || _columns[position].type != SQLT_STR)
			{
				value = reinterpret_cast<char const*>((((text *) _columns[position].data)+(_row_index*_columns[position].size)));
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");
			
			return value;
		}

		statement::statement(environment &env, context &ctx):
		_pointer((OCIStmt*) 0), _error(env)
		{
			_uuid = axon::util::uuid();

			DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			_context = &ctx;
			if ((_error = OCIHandleAlloc((dvoid *) env.get(), (dvoid **) &_pointer, (ub4) OCI_HTYPE_STMT, 0, NULL)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
			
			/* Allocate a statement handle */
			// if ((rc = OCIAttrSet((dvoid *) ctx.get(), (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _pointer, (ub4) 0, OCI_ATTR_SESSION, _error.get())) != OCI_SUCCESS)
			// 	throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
		}

		statement::~statement()
		{
			DBGPRN("[%s] %s", _uuid.c_str(), __PRETTY_FUNCTION__);
			if (_pointer != (OCIStmt*) 0)
				OCIHandleFree(_pointer, OCI_HTYPE_STMT);
		}

		OCIStmt *statement::get()
		{
			if (_pointer == (OCIStmt*) 0)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "statement not allocated");
			return _pointer;
		}

		void statement::prepare(std::string sql)
		{
			// if ((rc = OCIStmtPrepare(_pointer, _error.get(), (text*) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
			if ((_error = OCIStmtPrepare2(_context->get(), &_pointer, _error.get(), (text*) sql.c_str(), sql.size(), (OraText*) 0, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
		}

		int statement::bind(std::vector<axon::database::bind> &vars)
		{
			// READ: https://stackoverflow.com/questions/16883694/ocibindbypos-on-array-of-strings

			axon::timer ctm(__PRETTY_FUNCTION__);
			int index = 1, count = vars.size();

			for (auto &element : vars)
			{
				NOTPRN("+ Index: %d of %d, Type Index: %s", index, count, element.type().name());

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
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data, strlen(data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char **data = std::any_cast<unsigned char*>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)(*data), strlen((char*)*data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(std::string))
				{
					std::string *data = std::any_cast<std::string>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (text*)data->c_str(), data->size(), SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(float))
				{
					float *data = std::any_cast<float>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(float), SQLT_FLT, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(double))
				{
					double *data = std::any_cast<double>(&element);
					OCIBind *bndp = (OCIBind *) 0;

					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(double), SQLT_BDOUBLE, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				}
				else if (element.type() == typeid(int8_t))
				{
					int8_t *data = std::any_cast<int8_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int8_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

				}
				else if (element.type() == typeid(int16_t))
				{
					int16_t *data = std::any_cast<int16_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int16_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

				}
				else if (element.type() == typeid(int32_t))
				{
					int32_t *data = std::any_cast<int32_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int32_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT)).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

				}
				else if (element.type() == typeid(uint32_t))
				{
					uint32_t *data = std::any_cast<uint32_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(uint32_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

				}
				else if (element.type() == typeid(int64_t))
				{
					int64_t *data = std::any_cast<int64_t>(&element);
					OCIBind *bndp = (OCIBind *) 0;
					
					if ((_error = OCIBindByPos(_pointer, &bndp, _error.get(), index, (dvoid *) data, (sword) sizeof(int64_t), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )).failed())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

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

			vars.clear();

			return count;
		}

		void statement::execute(exec_type et)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if ((_error = OCIStmtExecute(_context->get(), _pointer, _error.get(), (et == exec_type::select)?0:1, 0, (OCISnapshot *) 0, (OCISnapshot *) 0, OCI_DEFAULT)).failed())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

		}

		int oracle::_get_int(int position)
		{
			return _resultset->get_int(position);
		}
		long oracle::_get_long(int position)
		{
			return _resultset->get_long(position);
		}
		float oracle::_get_float(int position)
		{
			return _resultset->get_float(position);
		}
		double oracle::_get_double(int position)
		{
			return _resultset->get_double(position);
		}
		std::string oracle::_get_string(int position)
		{
			return _resultset->get_string(position);
		}

		oracle::oracle():
			_context(_environment),
			_error(_environment),
			_server(_environment),
			_session(_environment, _context),
			_statement(_environment, _context)
		{
			_connected = false;
			_running = false;
			_executed = false;

			_port = 7776;

			_rowidx = 0;
			_colidx = 0;
		}

		oracle::~oracle()
		{
			close();
		}

		bool oracle::connect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_hostname.size() <= 1 || _username.size() <= 1 || _password.size() <= 1)
				return false;

			// this probably is only needed for CQN
			// if ((rc = OCIAttrSet((void *) _environment.get(), (ub4) OCI_HTYPE_ENV, (void *) &_port, (ub4) 0, (ub4) OCI_ATTR_SUBSCR_PORTNO, _error.get())) != OCI_SUCCESS)
			// 	throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			// attach to server
			if ((_error = OCIServerAttach(_server.get(), _error, (text *) _hostname.c_str(), (sb4) _hostname.size(), (ub4) OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			// set server attribute to service context
			if ((_error = OCIAttrSet((dvoid *) _context.get(), (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _server.get(), (ub4) 0, (ub4) OCI_ATTR_SERVER, (OCIError *) _error.get())).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			_session.connect(_username, _password);
			_connected = true;

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

			if (_connected)
			{
				_session.disconnect();
				if ((_error = OCIServerDetach(_server.get(), _error.get(), (ub4) OCI_DEFAULT)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
				_connected = false;
			}
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

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if ((_error = OCIPing(_context.get(), _error.get(), (ub4) OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			return true;
		}

		std::string oracle::version()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			char sver[1024];

			if((_error = OCIServerVersion(_context.get(), _error.get(), (text*) sver, (ub4) sizeof(sver), (ub1) OCI_HTYPE_SVCCTX)).failed())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			_version = sver;

			return _version;
		}

		bool oracle::transaction([[maybe_unused]] trans_t ttype)
		{
			return true;
		}

		bool oracle::execute(const std::string sql)
		{
			// TODO: DONE! How do we select all columns from a select statement? This we need to iterate between the field and see
			// how to pass the result accordingly. For now we are going to only return first field as this is the immediate requirement
			// For further reading, following link is helpful
			// https://docs.oracle.com/cd/B19306_01/appdev.102/b14250/oci04sql.htm

			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_running)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "query in progress, try later.");

			_statement.prepare(sql);
			_statement.bind(_bind);
			_statement.execute(exec_type::other);

			return true;
		}

		bool oracle::query(const std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			// This is the bind one list overload of the hard query
			//
			// How to use list bind variable with IN statement was borrowed with thanks from
			// https://stackoverflow.com/questions/18603281/oracle-oci-bind-variables-and-queries-like-id-in-1-2-3

			
			if (_running)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "query in progress, try later.");

			_running = true;

			OCIStmt *stmt = _statement.get();

			if ((_error = OCIHandleAlloc((dvoid *) _environment.get(), (dvoid **) &stmt, (ub4) OCI_HTYPE_STMT, 0, NULL)).failed())
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIHandleAlloc", _error.what());
			// if ((rc = OCIAttrSet((dvoid *) _statement, OCI_HTYPE_STMT, (void*) &_prefetch, sizeof(int), OCI_ATTR_PREFETCH_ROWS, _error)) != OCI_SUCCESS)
			// 	throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIAttrSet", axon::database::oracle::checker(_error, rc));
			_statement.prepare(sql);

			return true;
		}

		bool oracle::next()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_running && !_executed)
			{
				_statement.bind(_bind);
				_statement.execute(exec_type::select);

				_resultset.reset(nullptr);
				_resultset = std::make_unique<resultset>(&_statement, &_error);
				_executed = true;
			}

			if (!_resultset)
				return false;

			_colidx = 0;

			return _resultset->next();
		}

		void oracle::done()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_executed = false;
			_running = false;
			_resultset.reset(nullptr);
		}

		std::unique_ptr<resultset> oracle::make_resultset()
		{
			return std::make_unique<resultset>(&_statement, &_error);
		}

		std::string& oracle::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE_HOSTNAME)
				return _hostname;
			else if (i == AXON_DATABASE_USERNAME)
				return _username;
			else if (i == AXON_DATABASE_PASSWORD)
				return _password;

			return _throwawaystr;
		}

		int& oracle::operator[] (int i)
		{
			_throwawayint = i;
			return _throwawayint;
		}

		oracle& oracle::operator<<(int value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(long long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(float value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(double value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(std::string& value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value.c_str());
			_colidx++;

			return *this;
		}

		oracle& oracle::operator<<(axon::database::bind &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		oracle& oracle::operator>>(int &value)
		{
			// it was difficult to figure how to extract SQLT_NUM from Returned object
			// Reference on how to extract long long helped. check the following link
			// https://stackoverflow.com/questions/25808798/ocidefinebypos-extract-number-value-via-cursor-in-c
			//
			// API details on how to extract different type of number via OCI Number functions
			// https://docs.oracle.com/cd/B28359_01/appdev.111/b28395/oci19map003.htm

			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _resultset->get_int(_colidx);
			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(long &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _resultset->get_long(_colidx);
			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(float &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _resultset->get_float(_colidx);
			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(double &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _resultset->get_double(_colidx);
			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(std::string &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _resultset->get_string(_colidx);
			_colidx++;
			return *this;
		}

		std::ostream& oracle::printer(std::ostream &stream)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			std::string strval;

			for (unsigned int i = 0; i <= _resultset->column_count(); i++)
			{
				switch(_resultset->column_type(i))
				{
					case SQLT_CHR:
					case SQLT_STR:
					case SQLT_VCS:
					case SQLT_AFC:
					case SQLT_AVC:
						strval = _resultset->get_string(i);
						stream<<"<"<<i<<":S:"<<_resultset->column_type(i)<<":"<<strval<<">, ";
						break;
					
					case SQLT_NUM:
						stream<<"<"<<i<<":D:"<<_resultset->column_type(i)<<":"<<_resultset->get_int(i)<<">, ";
						break;
					
					case SQLT_DAT:
					case SQLT_TIMESTAMP:
						stream<<"<"<<i<<":T:"<<_resultset->column_type(i)<<":"<<_resultset->get_long(i)<<">, ";
						break;
				}
			}
			return stream<<std::endl;
		}
	}
}