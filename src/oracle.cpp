#include <regex>

#include <axon.h>
#include <axon/oracle.h>

namespace axon {

	namespace database2r {

		void oracle::_get_column_info()
		{
			BENCHMARK;
			ub4 column_count = 0;

			if ((_error = OCIAttrGet((CONST dvoid *) _statement->get(), (ub4) OCI_HTYPE_STMT, (void *) &column_count, (ub4 *) 0, (ub4) OCI_ATTR_PARAM_COUNT, _error.get())).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			_columns.clear();
			_columns.reserve(column_count);

			for (uint16_t count = 1; count <= column_count; count++)
			{
				_get_column_details(count);
			}
		}

		void oracle::_get_column_details(uint16_t position)
		{
			OCIParam *param  = nullptr;
			text     *name   = nullptr;
			ub4       namelen = 0;
			ub4       type   = 0;
			ub4       size   = 0;
			sb1       scale  = 0;
			sword     rc;

			struct ParamGuard {
				OCIParam *p;
				~ParamGuard() { if (p) OCIDescriptorFree(p, OCI_DTYPE_PARAM); }
			} guard { param };

			if ((rc = OCIParamGet(_statement->get(), OCI_HTYPE_STMT, _error.get(), (dvoid**) &param, position)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			guard.p = param;

			if ((rc = OCIAttrGet(param, OCI_DTYPE_PARAM, (dvoid**) &name, &namelen, OCI_ATTR_NAME, _error.get())) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			if ((rc = OCIAttrGet(param, OCI_DTYPE_PARAM, &type, nullptr, OCI_ATTR_DATA_TYPE, _error.get())) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			if ((rc = OCIAttrGet(param, OCI_DTYPE_PARAM, &size, nullptr, OCI_ATTR_DATA_SIZE, _error.get())) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			if ((type == SQLT_NUM || type == SQLT_VNU) &&
				(rc = OCIAttrGet(param, OCI_DTYPE_PARAM, &scale, nullptr, OCI_ATTR_SCALE, _error.get())) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));

			_columns.emplace_back(name, namelen, position, type, scale, size);
		}

		axon::column_type oracle::_attach_column_data(axon::database2r::oci::column &clmn, uint16_t count)
		{
			OCIDefine *def = { nullptr };
			sword rc;
			axon::column_type ct = axon::column_type::string_t;

			switch (clmn.type)
			{
				case SQLT_CHR:
				case SQLT_STR:
				case SQLT_VCS:
				case SQLT_AFC:
				case SQLT_AVC:
				case SQLT_LNG:
					clmn.allocate(count);
					ct = axon::column_type::string_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_STR, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_NUM:
					clmn.allocate(count);
					ct = (clmn.scale > 0) ? axon::column_type::double_t : axon::column_type::int64_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_VNU, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_DAT:
					clmn.allocate(count);
					ct = axon::column_type::string_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_DAT, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_TIMESTAMP:
				case SQLT_TIMESTAMP_TZ:
					clmn.allocate(count);
					ct = axon::column_type::string_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_DAT, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_BOL:
					clmn.size = sizeof(unsigned char);
					clmn.allocate(count);
					ct = axon::column_type::bool_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), (sb4) sizeof(unsigned char), SQLT_BOL, clmn.indicator.data(), nullptr, nullptr, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_BFLOAT:    // 100 — BINARY_FLOAT
				case SQLT_BDOUBLE:   // 101 — BINARY_DOUBLE
					clmn.size = sizeof(double);
					clmn.data.resize(sizeof(double) * count, 0);
					clmn.indicator.resize(count, 0);
					clmn.rlen.resize(count, 0);
					ct = axon::column_type::double_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), sizeof(double), SQLT_BDOUBLE, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_TIMESTAMP_LTZ:  // 232
					clmn.allocate(count);
					ct = axon::column_type::string_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_DAT, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_INTERVAL_YM:  // 189
				case SQLT_INTERVAL_DS:  // 190
					clmn.size = 64;     // enough for string representation
					clmn.allocate(count);
					ct = axon::column_type::string_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_STR, clmn.indicator.data(), 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_BIN:       // 23 RAW
					clmn.allocate(count);
					ct = axon::column_type::bytes_t;
					if ((rc = OCIDefineByPos(_statement->get(), &def, _error.get(), clmn.position, clmn.data.data(), clmn.size+1, SQLT_BIN, clmn.indicator.data(), clmn.rlen.data(), nullptr, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what(rc));
					break;

				case SQLT_BLOB:      // 113
				case SQLT_CLOB:      // 112
					// LOB locators require special handling — skip for now, treat as null
					clmn.size = 1;
					clmn.allocate(count);
					ct = axon::column_type::bytes_t;
					// deliberately not calling OCIDefineByPos for LOBs
					break;

				case SQLT_INT:          ct = axon::column_type::int64_t;  break;   // INTEGER
				case SQLT_FLT:          ct = axon::column_type::double_t; break;   // FLOAT — was string_t, wrong
				case SQLT_VNU:          ct = (clmn.scale > 0) ? axon::column_type::double_t : axon::column_type::int64_t; break;  // VARNUM
				case SQLT_PDN:          ct = axon::column_type::string_t; break;   // packed decimal (rare)
				case SQLT_NON:          ct = axon::column_type::string_t; break;
				case SQLT_RID:          ct = axon::column_type::string_t; break;   // ROWID
				case SQLT_VBI:          ct = axon::column_type::bytes_t;  break;   // variable binary — was string_t
				case SQLT_LBI:          ct = axon::column_type::bytes_t;  break;   // LONG RAW — was string_t
				case SQLT_UIN:          ct = axon::column_type::int64_t;  break;   // UNSIGNED INT — was string_t
				case SQLT_SLS:          ct = axon::column_type::string_t; break;
				case SQLT_LVC:          ct = axon::column_type::string_t; break;   // LONG VARCHAR
				case SQLT_LVB:          ct = axon::column_type::bytes_t;  break;   // LONG VARRAW — was string_t
				case SQLT_IBFLOAT:      ct = axon::column_type::double_t; break;   // BINARY_FLOAT (internal)
				case SQLT_IBDOUBLE:     ct = axon::column_type::double_t; break;   // BINARY_DOUBLE (internal)
				case SQLT_CUR:          ct = axon::column_type::string_t; break;   // cursor
				case SQLT_RDD:          ct = axon::column_type::string_t; break;   // ROWID descriptor
				case SQLT_LAB:          ct = axon::column_type::string_t; break;
				case SQLT_OSL:          ct = axon::column_type::string_t; break;
				case SQLT_NTY:          ct = axon::column_type::string_t; break;   // named type
				case SQLT_REF:          ct = axon::column_type::string_t; break;   // REF
				case SQLT_BFILEE:       ct = axon::column_type::bytes_t;  break;   // BFILE
				case SQLT_CFILEE:       ct = axon::column_type::string_t; break;   // CFILE
				case SQLT_RSET:         ct = axon::column_type::string_t; break;   // result set
				case SQLT_JSON:         ct = axon::column_type::string_t; break;   // JSON
				case SQLT_NCO:          ct = axon::column_type::string_t; break;   // NCHAR/NVARCHAR2
				case SQLT_VEC:          ct = axon::column_type::bytes_t;  break;   // VECTOR (23c) — binary
				case SQLT_VST:          ct = axon::column_type::string_t; break;
				case SQLT_ODT:          ct = axon::column_type::string_t; break;   // OCI date
				case SQLT_DATE:         ct = axon::column_type::string_t; break;   // ANSI DATE
				case SQLT_TIME:         ct = axon::column_type::string_t; break;
				case SQLT_TIME_TZ:      ct = axon::column_type::string_t; break;
				case SQLT_REC:          ct = axon::column_type::string_t; break;
				case SQLT_TAB:          ct = axon::column_type::string_t; break;

				default:
					DBGPRN("Unknown type: %d", clmn.type);
			}

			return ct;
		}

		int oracle::_get_int(size_t position, int row_index)
		{
			BENCHMARK;
			int value = 0;

			if (position >= _columns.size())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			DBGPRN("_columns.size(): %d, position: %d, col.type: %d", _columns.size(), position, _columns[position].type);

			if (_columns[position].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			OCINumber *num = reinterpret_cast<OCINumber*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
			OCINumberToInt(_error.get(), num, sizeof(int), OCI_NUMBER_SIGNED, &value);

			return value;
		}

		bool oracle::_get_bool(size_t position, int row_index)
		{
			BENCHMARK;

			if (position >= _columns.size())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");

			if (_columns[position].type != SQLT_BOL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column is not a boolean type");

			const sb2 ind = _columns[position].indicator[row_index];
			if (ind < 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column value is NULL");

			const unsigned char *ptr = _columns[position].data.data() + (row_index * sizeof(unsigned char));

			return (*ptr != 0);
		}

		long oracle::_get_long(size_t position, int row_index)
		{
			BENCHMARK;
			long value = 0;

			if (position >= _columns.size())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			DBGPRN("_columns.size(): %d, position: %d, col.type: %d", _columns.size(), position, _columns[position].type);

			if (_columns[position].type == SQLT_DAT || _columns[position].type == SQLT_TIMESTAMP || _columns[position].type == SQLT_TIMESTAMP_TZ)
			{
				char *raw;

				raw = ((char*) _columns[position].data.data()) + (row_index*(_columns[position].size+1));

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
			{
				OCINumber *num = reinterpret_cast<OCINumber*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
				OCINumberToInt(_error.get(), num, sizeof(long), OCI_NUMBER_SIGNED, &value);
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			return value;
		}

		float oracle::_get_float(size_t position, int row_index)
		{
			BENCHMARK;
			float value = 0;

			if (position >= _columns.size())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			DBGPRN("_columns.size(): %d, position: %d, col.type: %d", _columns.size(), position, _columns[position].type);

			if (_columns[position].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			OCINumber *num = reinterpret_cast<OCINumber*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
			OCINumberToReal(_error.get(), num, sizeof(float), &value);

			return value;
		}

		double oracle::_get_double(size_t position, int row_index)
		{
			BENCHMARK;
			double value = 0;

			if (position >= _columns.size())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			DBGPRN("_columns.size(): %d, position: %d, col.type: %d", _columns.size(), position, _columns[position].type);

			if (_columns[position].type == SQLT_NUM)
			{
				OCINumber *num = reinterpret_cast<OCINumber*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
				OCINumberToReal(_error.get(), num, sizeof(double), &value);
			}
			else if (_columns[position].type == SQLT_BFLOAT || _columns[position].type == SQLT_BDOUBLE)
			{
				const double *ptr = reinterpret_cast<const double*>(_columns[position].data.data() + (row_index * sizeof(double)));
				return *ptr;
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type");

			return value;
		}

		std::string oracle::_get_string(size_t position, int row_index)
		{
			BENCHMARK;
			std::string value;

			if (position >= _columns.size())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			DBGPRN("_columns.size(): %d, position: %d, type: %d, size: %d", _columns.size(), position, _columns[position].type, _columns[position].size);

			if (_columns[position].type == SQLT_DAT || _columns[position].type == SQLT_TIMESTAMP || _columns[position].type == SQLT_TIMESTAMP_TZ || _columns[position].type == SQLT_TIMESTAMP_LTZ)
			{
				char temp[64], *raw;

				raw = ((char*) _columns[position].data.data()) + (row_index*(_columns[position].size+1));

				if (raw[0] == 0)
					sprintf(temp, "0000-00-00 00:00:00");
				else
					sprintf(temp, "%02d%02d-%02d-%02d %02d:%02d:%02d", 
						((unsigned char)raw[0])-100, ((unsigned char)raw[1])-100, 
						((int)raw[2]), ((int)raw[3]),
						((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
				value = temp;
			}
			else if (_columns[position].type == SQLT_AFC || _columns[position].type == SQLT_CHR || _columns[position].type == SQLT_STR)
			{
				value = reinterpret_cast<char const*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
				auto pos = value.find_last_not_of(' ');
				if (pos != std::string::npos)
					value.erase(pos + 1);
				else
					value.clear();
			}
			else if (_columns[position].type == SQLT_INTERVAL_YM || _columns[position].type == SQLT_INTERVAL_DS)
			{
				value = reinterpret_cast<const char*>(_columns[position].data.data() + (row_index * (_columns[position].size + 1)));
			}
			else
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "variable type is not compatable with row type %u", _columns[position].type);

			return value;
		}

		oracle::oracle(): _connection(std::make_shared<axon::database2r::oci::connection>()), _statement(std::make_shared<axon::database2r::oci::statement>(_connection->context()))
		{
			_running = false;
		}

		oracle::oracle(std::shared_ptr<axon::database2r::oci::connection> connection): _connection(connection), _statement(std::make_shared<axon::database2r::oci::statement>(_connection->context()))
		{
			_running = false;
		}

		oracle::~oracle()
		{
			close();
		}

		bool oracle::connect()
		{
			BENCHMARK;

			if (_hostname.empty() || _username.empty() || _password.empty())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "hostname, username or password not set");

			_connection->connect(_hostname, _username, _password);

			return _connection->connected();
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
			BENCHMARK;

			if (_connection->connected())
				_connection->disconnect();

			return _connection->connected();
		}

		bool oracle::flush()
		{
			execute("COMMIT");
			return true;
		}

		bool oracle::ping()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if ((_error = OCIPing(_connection->ctx(), _error.get(), (ub4) OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			return true;
		}

		std::string oracle::version()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			char sver[1024];

			if((_error = OCIServerVersion(_connection->ctx(), _error.get(), (text*) sver, (ub4) sizeof(sver), (ub1) OCI_HTYPE_SVCCTX)).failed())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			_version = sver;

			return _version;
		}

		bool oracle::transaction([[maybe_unused]] axon::database2r::trans_t ttype)
		{
			if (ttype == axon::database2r::transaction::END)
				return execute("COMMIT");
			return true; 
		}

		bool oracle::execute(const std::string sql)
		{
			// How do we select all columns from a select statement?
			// For further reading, following link is helpful
			// https://docs.oracle.com/cd/B19306_01/appdev.102/b14250/oci04sql.htm

			BENCHMARK;

			if (_running)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot execute while query in progress");

			_statement->prepare(sql);
			_statement->bind(_bind);
			_statement->execute(axon::database2r::exec_type::other);

			return true;
		}

		bool oracle::query(const std::string sql)
		{
			BENCHMARK;
			// This is the bind one list overload of the hard query
			//
			// How to use list bind variable with IN statement was borrowed with thanks from
			// https://stackoverflow.com/questions/18603281/oracle-oci-bind-variables-and-queries-like-id-in-1-2-3


			if (_running)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "query in progress, try later.");

			_statement->prepare(sql);
			_statement->bind(_bind);
			_statement->execute(exec_type::select);

			_get_column_info();

			_running = true;

			return true;
		}

		void oracle::fetch(axon::recordset2r &rs, int howmany)
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (_running)
			{
				sword rc;

				if (!_schema_pushed)
				{
					for (auto &column : _columns)
					{
						axon::column_type ct = _attach_column_data(column, howmany);
						rs.add_column(column.name, ct);
					}

					_schema_pushed = true;
				}

				rc = OCIStmtFetch2(_statement->get(), _error.get(), howmany, OCI_DEFAULT, 0, OCI_DEFAULT);

				if (rc == OCI_SUCCESS || rc == OCI_NO_DATA || rc == OCI_SUCCESS_WITH_INFO)
				{
					ub4 fetched = 0;

					OCIAttrGet(_statement->get(), OCI_HTYPE_STMT, (void*) &fetched, NULL, OCI_ATTR_ROWS_FETCHED, _error.get());

					if (rc == OCI_SUCCESS_WITH_INFO)
    					WRNPRN("OCIStmtFetch2() returned OCI_SUCCESS_WITH_INFO — possible truncation");

					for (ub4 row = 0; row < fetched; row++)
					{
						rs.begin_row();

						for (size_t col = 0; col < _columns.size(); col++)
						{
							const axon::database2r::oci::column &clmn = _columns[col];

							if (clmn.indicator.empty() || clmn.data.empty())
							{
								rs.push_null();
								continue;
							}

							if (clmn.indicator[row] < 0) { rs.push_null(); continue; }

							switch (clmn.type)
							{
								case SQLT_CHR:
								case SQLT_STR:
								case SQLT_VCS:
								case SQLT_AFC:
								case SQLT_AVC:
								case SQLT_LNG:
									rs.push_string(_get_string(col, row));
									break;

								case SQLT_DAT:
								case SQLT_TIMESTAMP:
								case SQLT_TIMESTAMP_TZ:
									rs.push_string(_get_string(col, row));
									break;

								case SQLT_NUM:
								case SQLT_VNU:
									if (clmn.scale != 0)
										rs.push_double(_get_double(col, row));
									else
										rs.push_int(_get_long(col, row));
									break;

								case SQLT_INT:
								case SQLT_UIN:
									rs.push_int(_get_long(col, row));
									break;

								case SQLT_FLT:
								case SQLT_BFLOAT:
								case SQLT_BDOUBLE:
								case SQLT_IBFLOAT:
								case SQLT_IBDOUBLE:
									rs.push_double(_get_double(col, row));
									break;

								case SQLT_BOL:
									rs.push_bool(_get_bool(col, row));
									break;

								case SQLT_BIN:
								case SQLT_LBI:
								case SQLT_VBI:
								case SQLT_LVB:
								case SQLT_BLOB:
								case SQLT_BFILEE:
									rs.push_bytes(clmn.data.data() + (row * (clmn.size+1)), clmn.rlen[row]);
									break;

								default:
									rs.push_null();
									break;
							}
						}

						rs.end_row();
					}

					if (rc == OCI_NO_DATA)
						rs.set_eof();
				}
			}
		}

		void oracle::done()
		{
			BENCHMARK;

			_statement->reset();
			_bind.clear();
			_columns.clear();

			_running = false;
			_schema_pushed = false;
		}

		std::string& oracle::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE2R_HOSTNAME)
				return _hostname;
			else if (i == AXON_DATABASE2R_USERNAME)
				return _username;
			else if (i == AXON_DATABASE2R_PASSWORD)
				return _password;

			return _throwawaystr;
		}

		int& oracle::operator[] (int i)
		{
			_throwawayint = i;
			return _throwawayint;
		}
	}
}
