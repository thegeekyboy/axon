#include <any>
#include <regex>
#include <cstdarg>

#include <axon.h>
#include <axon/util.h>
#include <axon/scylladb.h>

namespace axon
{
	namespace database
	{
		int scylladb::replace(std::string &sql)
		{
			boost::regex expression("'(?:[^']|'')*'|\\B(:[a-zA-Z0-9_]+)");
			sql = boost::regex_replace(sql, expression, [&](auto& match){
				if (match.str()[0] != '\'') return std::string("?");
				return match.str();
			});

			return 0;
		}

		int scylladb::prepare(int count, va_list *list, axon::database::bind *first)
		{
			axon::database::bind *element = first;
			int index;

			_bind.clear();

			for (index = 0; index < count; index++)
			{
				if (element == nullptr)
					break;

				INFPRN("* Index: %d of %d, Type Index: %s", index, count, element->type().name());

				_bind.push_back(*element);

				element = va_arg(*list, axon::database::bind*);
			}

			return 0;
		}

		int scylladb::bind(statement &stmt2)
		{
			int index = 0, count = _bind.size();

			CassStatement *stmt = stmt2.get();

			for (auto &element : _bind)
			{
				CassError ce = CASS_OK;;
				INFPRN("+ Index: %d of %d, Type Index: %s", index, count, element.type().name());

				if (element.type() == typeid(std::vector<std::string>))
				{
					std::vector<std::string> data = std::any_cast<std::vector<std::string>>(element);
				}
				else if (element.type() == typeid(std::vector<double>))
				{
					std::vector<double> data = std::any_cast<std::vector<double>>(element);
				}
				else if (element.type() == typeid(std::vector<int>))
				{
					std::vector<int> data = std::any_cast<std::vector<int>>(element);
				}
				else if (element.type() == typeid(char*))
				{
					char *data = std::any_cast<char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, (char*)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char *data = std::any_cast<unsigned char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, (char*)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(std::string))
				{
					std::string data = std::any_cast<std::string>(element);
					if ((ce = cass_statement_bind_string(stmt, index, data.c_str())) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(float))
				{
					float data = std::any_cast<float>(element);
					if ((ce = cass_statement_bind_float(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(double))
				{
					double data = std::any_cast<double>(element);
					if ((ce = cass_statement_bind_double(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(int8_t))
				{
					int8_t data = std::any_cast<int8_t>(element);
					if ((ce = cass_statement_bind_int8(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(int16_t))
				{
					int16_t data = std::any_cast<int16_t>(element);
					if ((ce = cass_statement_bind_int16(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(int32_t))
				{
					int32_t data = std::any_cast<int32_t>(element);
					if ((ce = cass_statement_bind_int32(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(uint32_t))
				{
					uint32_t data = std::any_cast<uint32_t>(element);
					if ((ce = cass_statement_bind_uint32(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(long long))
				{
					int64_t data = std::any_cast<long long>(element);
					if ((ce = cass_statement_bind_int64(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(uint64_t))
				{
					[[maybe_unused]]uint64_t data = std::any_cast<uint64_t>(element);
				}
				else if (element.type() == typeid(bool))
				{
					bool data = std::any_cast<bool>(element);
					if ((ce = cass_statement_bind_bool(stmt, index, (cass_bool_t)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else if (element.type() == typeid(CassUuid*))
				{
					CassUuid *data = std::any_cast<CassUuid*>(element);
					if ((ce = cass_statement_bind_uuid(stmt, index, *data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, cass_error_desc(ce));
				}
				else
				{
					std::cout<<element.type().name()<<std::endl;
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);
				}

				index++;
			}

			_bind.clear();

			return count;
		}

		scylladb::scylladb()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_connected = false;
			_query = false;
			_running = false;

			_cluster = cass_cluster_new();
			_session = cass_session_new();
		}

		scylladb::scylladb(const scylladb &)
		{
		}

		scylladb::~scylladb()
		{
			if (_connected)
			{
				flush();

				try {
					close();
				} catch (axon::exception& e) {

					std::cerr<<e.what()<<std::endl;
				}
			}
		}

		bool scylladb::connect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database already open");

			if (_hostname.empty() || _username.empty() || _password.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Connection credentials missing");

			cass_cluster_set_contact_points(_cluster, _hostname.c_str());
			cass_cluster_set_core_connections_per_host(_cluster, 2);
			cass_cluster_set_connection_heartbeat_interval(_cluster, 30);
			cass_cluster_set_connection_idle_timeout(_cluster, 45);
			cass_cluster_set_dse_plaintext_authenticator(_cluster, _username.c_str(), _password.c_str());

			future fq = cass_session_connect_keyspace(_session, _cluster, _keyspace.c_str());

			fq.wait();
			if (!fq)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot open database, driver: %s", fq.what());

			_connected = true;
			_rowidx = 0;
			_colidx = 0;

			return _connected;
		}

		bool scylladb::connect(std::string hostname, std::string username, std::string password)
		{
			_hostname = hostname;
			_username = username;
			_password = password;

			return connect();
		}

		bool scylladb::close()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (_query)
			{
				DBGPRN("%s called while query in progress", __PRETTY_FUNCTION__);
				done();
			}

			future fq = cass_session_close(_session);
			fq.wait();
			if (!fq)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error closing the database");

			cass_session_free(_session);
			cass_cluster_free(_cluster);

			return true;
		}

		bool scylladb::flush()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			return true;
		}

		bool scylladb::ping()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			return true;
		}

		std::string scylladb::version()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			const CassSchemaMeta* schema_meta;;

			if ((schema_meta = cass_session_get_schema_meta(_session)) == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot get database version");

			CassVersion cv = cass_schema_meta_version(schema_meta);
			std::stringstream ss;
			ss<<cv.major_version<<"."<<cv.minor_version<<"."<<cv.patch_version;
			cass_schema_meta_free(schema_meta);

			return ss.str();
		}

		bool scylladb::transaction(trans_t)
		{
			return false;
		}

		bool scylladb::execute(std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot execute while query in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);

				std::unique_ptr<future> fq;

				_statement.prepare(_session, sql);
				bind(_statement);
				fq = _statement.execute();

				fq->wait();

				if (!fq)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq->what());
			}

			return true;
		}

		bool scylladb::query(std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Another query is in progress");

			if (!_running)
			{
				_running = true;

				{
					// Scope the lock guard locally just in case
					std::lock_guard<std::mutex> lock(_safety);

					_statement.prepare(_session, sql);

					_prepared = true;
					_query = false;
				}

				_rowidx = 0;
				_colidx = 0;

				_running = false;
			}
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already busy with one query. please release first");

			return true;
		}

		bool scylladb::next()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (!_query && !_prepared)
				return false;
				// throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No fetch in progress");

			if (!_query && _prepared)
			{
				std::unique_ptr<future> fq;
				bind(_statement);
				fq = _statement.execute();

				fq->wait();

				if (!fq)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq->what());

				_records = fq->make_recordset();

				_query = true;
				_rowidx = 0;
				_colidx = 0;
			}

			_rowidx++;
			_colidx = 0;

			return _records->next();
		}

		void scylladb::done()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (!_query)
				return;
				// throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No statement compiled to close");

			_records.reset(nullptr);	// delete unique_ptr
			_bind.clear();
			_query = false;
			_prepared = false;

			_rowidx = 0;
			_colidx = 0;
		}

		std::string& scylladb::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE_HOSTNAME)
				return _hostname;
			else if (i == AXON_DATABASE_USERNAME)
				return _username;
			else if (i == AXON_DATABASE_PASSWORD)
				return _password;
			else if (i == AXON_DATABASE_KEYSPACE)
				return _keyspace;

			return _throwawaystr;
		}

		int& scylladb::operator[] (int i)
		{
			_throwawayint = i;
			return _throwawayint;
		}

		scylladb& scylladb::operator<<(int value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(long long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(float value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(double value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(std::string& value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator<<(axon::database::bind &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		// https://docs.datastax.com/en/developer/cpp-driver/2.2/api/cassandra.h/#enum-CassValueType

		scylladb& scylladb::operator>>(int &value)
		{
			value = _get_int(_colidx);

			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator>>(long &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_long(_colidx);
			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator>>(float &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_float(_colidx);

			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator>>(double &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_double(_colidx);

			_colidx++;

			return *this;
		}

		scylladb& scylladb::operator>>(std::string &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_string(_colidx);

			_colidx++;

			return *this;
		}

		int scylladb::_get_int(int position)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			int value = 0;

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No active query");

			if (position >= _records->colcount())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			const CassRow *row = _records->get();
			const CassValue *val = cass_row_get_column(row, position);
			const CassDataType *data_type = cass_value_data_type(val);
			CassError ce;

			if (data_type == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");

			if (cass_data_type_type(data_type) != CASS_VALUE_TYPE_INT && cass_data_type_type(data_type) != CASS_VALUE_TYPE_TINY_INT && cass_data_type_type(data_type) != CASS_VALUE_TYPE_SMALL_INT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type %d", cass_data_type_type(data_type));

			if (!cass_value_is_null(val))
			{
				cass_int8_t i8;
				cass_int16_t i16;
				cass_int32_t i32;

				switch (cass_data_type_type(data_type))
				{
					case CASS_VALUE_TYPE_INT:
						if ((ce = cass_value_get_int32(val, &i32)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", position, cass_error_desc(ce));
						value = i32;
						break;

					case CASS_VALUE_TYPE_SMALL_INT:
						if ((ce = cass_value_get_int16(val, &i16)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", position, cass_error_desc(ce));
						value = i16;
						break;

					case CASS_VALUE_TYPE_TINY_INT:
						if ((ce = cass_value_get_int8(val, &i8)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", position, cass_error_desc(ce));
						value = i8;
						break;

					default:
						value = 0;
						break;
				}
			}

			return value;
		};

		long scylladb::_get_long(int position)
		{
			long value = 0;

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No active query");

			if (position >= _records->colcount())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			const CassRow *row = _records->get();
			const CassValue *val = cass_row_get_column(row, position);
			const CassDataType *data_type = cass_value_data_type(val);
			CassError ce;

			if (data_type == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");

			if (cass_data_type_type(data_type) != CASS_VALUE_TYPE_BIGINT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type <long> is not compatable with row type %d", cass_data_type_type(data_type));

			if (!cass_value_is_null(val))
			{
				if ((ce = cass_value_get_int64(val, &value)) != CASS_OK)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", _colidx, cass_error_desc(ce));
			}

			return value;
		};

		float scylladb::_get_float(int position)
		{
			float value = 0;

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No active query");

			if (position >= _records->colcount())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			const CassRow *row = _records->get();
			const CassValue *val = cass_row_get_column(row, position);
			const CassDataType *data_type = cass_value_data_type(val);
			CassError ce;

			if (data_type == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");

			if (cass_data_type_type(data_type) != CASS_VALUE_TYPE_FLOAT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type %d", cass_data_type_type(data_type));

			if (!cass_value_is_null(val))
			{
				if ((ce = cass_value_get_float(val, &value)) != CASS_OK)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", _colidx, cass_error_desc(ce));
			}

			return value;
		}

		double scylladb::_get_double(int position)
		{
			double value = 0;

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No active query");

			if (position >= _records->colcount())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			const CassRow *row = _records->get();
			const CassValue *val = cass_row_get_column(row, position);
			const CassDataType *data_type = cass_value_data_type(val);
			CassError ce;

			if (data_type == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");

			if (cass_data_type_type(data_type) != CASS_VALUE_TYPE_DOUBLE)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type %d", cass_data_type_type(data_type));

			if (!cass_value_is_null(val))
			{
				if ((ce = cass_value_get_double(val, &value)) != CASS_OK)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", _colidx, cass_error_desc(ce));
			}

			return value;
		}

		std::string scylladb::_get_string(int position)
		{
			std::string value;

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No active query");

			if (position >= _records->colcount())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			const CassRow *row = _records->get();
			const CassValue *val = cass_row_get_column(row, position);
			const CassDataType *data_type = cass_value_data_type(val);

			if (data_type == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");

			if (cass_data_type_type(data_type) != CASS_VALUE_TYPE_TEXT && cass_data_type_type(data_type) != CASS_VALUE_TYPE_VARCHAR)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type %d", cass_data_type_type(data_type));

			if (!cass_value_is_null(val))
			{
				const char *tmp;
				size_t size;
				cass_value_get_string(val, &tmp, &size);
				if (size) value = tmp;
			}

			return value;
		};

		std::ostream& scylladb::printer(std::ostream &stream)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			int rc = _records->colcount();
			const CassRow *row = _records->get();

			for (int i = 0; i < rc; i++)
			{
				const CassValue *val = cass_row_get_column(row, i);
				const CassDataType *data_type = cass_value_data_type(val);
				CassError ce;

				if (cass_value_is_null(val))
				{
					stream<<"<"<<i<<":NULL:NULL>, ";
					continue;
					// throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "NULL value column");
				}

				switch(cass_data_type_type(data_type))
				{
					case CASS_VALUE_TYPE_TINY_INT:
						int8_t i8;
						if ((ce = cass_value_get_int8(val, &i8)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":i8:"<<i8<<">, ";
						break;

					case CASS_VALUE_TYPE_SMALL_INT:
						int16_t i16;
						if ((ce = cass_value_get_int16(val, &i16)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":i16:"<<i16<<">, ";
						break;

					case CASS_VALUE_TYPE_INT:
						int32_t i32;
						if ((ce = cass_value_get_int32(val, &i32)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":i32:"<<i32<<">, ";
						break;

					case CASS_VALUE_TYPE_BIGINT:
						long i64;
						if ((ce = cass_value_get_int64(val, &i64)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":i64:"<<i64<<">, ";
						break;

					case CASS_VALUE_TYPE_FLOAT:
						float flt;
						if ((ce = cass_value_get_float(val, &flt)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":f:"<<flt<<">, ";
						break;

					case CASS_VALUE_TYPE_DOUBLE:
						double dbl;
						if ((ce = cass_value_get_double(val, &dbl)) != CASS_OK)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to extract value at index: %d - %s", i, cass_error_desc(ce));
						stream<<"<"<<i<<":lf:"<<dbl<<">, ";
						break;

					case CASS_VALUE_TYPE_TEXT:
					case CASS_VALUE_TYPE_VARCHAR:
						const char *tmp;
						size_t size;
						cass_value_get_string(val, &tmp, &size);
						stream<<"<"<<i<<":S:"<<tmp<<">, ";
						break;

					default:
						break;
				}
			}

			stream<<std::endl;
			return stream;
		}
	}
}
