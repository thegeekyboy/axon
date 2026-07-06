#include <any>
#include <regex>
#include <cstdarg>

#include <axon.h>
#include <axon/util.h>
#include <axon/scylla.h>

namespace axon
{
	namespace database2r
	{
		int scylla::replace(std::string &sql)
		{
			boost::regex expression("'(?:[^']|'')*'|\\B(:[a-zA-Z0-9_]+)");
			sql = boost::regex_replace(sql, expression, [&](auto& match){
				if (match.str()[0] != '\'') return std::string("?");
				return match.str();
			});

			return 0;
		}

		int scylla::bind(std::shared_ptr<axon::database2r::sci::statement> &stmt2)
		{
			int index = 0, count = _bind.size();

			CassStatement *stmt = stmt2->get();

			for (auto &element : _bind)
			{
				CassError ce = CASS_OK;

				INFPRN("+ Index: %d of %d, Type Index: %s", index, count, element.type().name());

				if (element.type() == typeid(std::vector<std::string>))
				{
					// std::vector<std::string> data = std::any_cast<std::vector<std::string>>(element);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "currently not supported data type %s at index: %d", element.type().name(), index);
				}
				else if (element.type() == typeid(std::vector<double>))
				{
					// std::vector<double> data = std::any_cast<std::vector<double>>(element);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "currently not supported data type %s at index: %d", element.type().name(), index);
				}
				else if (element.type() == typeid(std::vector<int>))
				{
					// std::vector<int> data = std::any_cast<std::vector<int>>(element);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "currently not supported data type %s at index: %d", element.type().name(), index);
				}
				else if (element.type() == typeid(char*))
				{
					char *data = std::any_cast<char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, (char*)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char *data = std::any_cast<unsigned char *>(element);
					if ((ce = cass_statement_bind_string(stmt, index, (char*)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(std::string))
				{
					const std::string &data = std::any_cast<const std::string&>(element);
					if ((ce = cass_statement_bind_string(stmt, index, data.c_str())) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(float))
				{
					const float &data = std::any_cast<const float&>(element);
					if ((ce = cass_statement_bind_float(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(double))
				{
					const double &data = std::any_cast<const double&>(element);
					if ((ce = cass_statement_bind_double(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(int8_t))
				{
					const int8_t &data = std::any_cast<const int8_t&>(element);
					if ((ce = cass_statement_bind_int8(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(int16_t))
				{
					const int16_t &data = std::any_cast<const int16_t&>(element);
					if ((ce = cass_statement_bind_int16(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(int32_t))
				{
					const int32_t &data = std::any_cast<const int32_t&>(element);
					if ((ce = cass_statement_bind_int32(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(uint32_t))
				{
					const uint32_t &data = std::any_cast<const uint32_t&>(element);
					if ((ce = cass_statement_bind_uint32(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(long long))
				{
					const long long &data = std::any_cast<const long long&>(element);
					if ((ce = cass_statement_bind_int64(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(int64_t))
				{
					const int64_t &data = std::any_cast<const int64_t&>(element);
					if ((ce = cass_statement_bind_int64(stmt, index, data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(uint64_t))
				{
					// [[maybe_unused]]uint64_t data = std::any_cast<uint64_t>(element);
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "uint64_t bind not supported at index %d — use int64_t", index);
				}
				else if (element.type() == typeid(bool))
				{
					bool data = std::any_cast<bool>(element);
					if ((ce = cass_statement_bind_bool(stmt, index, (cass_bool_t)data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else if (element.type() == typeid(CassUuid*))
				{
					CassUuid *data = std::any_cast<CassUuid*>(element);
					if ((ce = cass_statement_bind_uuid(stmt, index, *data)) != CASS_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, type: %s, driver: %s", index, element.type().name(), cass_error_desc(ce));
				}
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);

				index++;
			}

			_bind.clear();

			return count;
		}

		scylla::scylla()
		{
			BENCHMARK;

			_query = false;
			_prepared = false;

			_connection = std::make_shared<axon::database2r::sci::connection>();
			_statement = std::make_shared<axon::database2r::sci::statement>();
		}

		scylla::~scylla()
		{
			if (_connection->connected())
			{
				try { flush(); } catch (axon::exception &e) { ERRPRN("%s", e.what()); }
        		try { close(); } catch (axon::exception &e) { ERRPRN("%s", e.what()); }
			}
		}

		bool scylla::connect(std::string hostname, std::string username, std::string password)
		{
			_hostname = hostname;
			_username = username;
			_password = password;

			return connect();
		}

		bool scylla::connect()
		{
			BENCHMARK;

			if (!_keyspace.empty())
				_connection->connect(_hostname, _username, _password, _port, _keyspace);
			else
				_connection->connect(_hostname, _username, _password, _port);

			return _connection->connected();
		}

		bool scylla::close()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (_query)
			{
				DBGPRN("%s called while query in progress", __PRETTY_FUNCTION__);
				done();
			}

			_connection->disconnect();

			return true;
		}

		bool scylla::flush()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			return true;
		}

		bool scylla::ping()
		{
			BENCHMARK;

			if (!_connection->connected()) return false;

			try
			{
				_statement->prepare(_connection, "SELECT now() FROM system.local");
				auto fq = _statement->execute();
				return !fq->wait().failed();
			}
			catch (...) { return false; }
		}

		std::string scylla::version()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			axon::database2r::sci::metadata md(_connection->get());
			
			CassVersion cv = cass_schema_meta_version(md.get());
			char version[32];
			std::snprintf(version, sizeof(version), "%d.%d.%d", cv.major_version, cv.minor_version, cv.patch_version);

			return std::string(version);
		}

		bool scylla::transaction(trans_t)
		{
			return true;
		}

		bool scylla::execute(const std::string sql)
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot execute while query in progress");

			std::lock_guard<std::mutex> lock(_safety);

			std::unique_ptr<axon::database2r::sci::future> fq;

			_statement->prepare(_connection, sql);
			bind(_statement);
			fq = _statement->execute();

			if (fq->wait().failed())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq->what());

			_bind.clear();

			return true;
		}

		bool scylla::query(const std::string sql)
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Another query is in progress");

			std::lock_guard<std::mutex> lock(_safety);
			_statement->prepare(_connection, sql);
			_prepared = true;

			return true;
		}

		void scylla::fetch(axon::recordset2r &rs, int howmany)
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not connected");

			if (!_query && !_prepared)
			{
				rs.set_eof();
				return;
			}

			// First fetch call — execute and materialise the result
			if (!_query && _prepared)
			{
				bind(_statement);

				auto fq = _statement->execute();
				if (fq->wait().failed())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq->what());

				// Release any previous result
				if (_iterator) { cass_iterator_free(_iterator); _iterator = nullptr; }
				if (_result) { cass_result_free(_result); _result = nullptr; }

				_result = cass_future_get_result(fq->get());
				if (!_result) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "query returned no result — use execute() for DML statements");
				_iterator = cass_iterator_from_result(_result);

				_query   = true;
				_prepared = false;
			}

			// Build schema on first fetch using result metadata — no row needed
			if (!_schema_pushed)
			{
				size_t col_count = cass_result_column_count(_result);

				for (size_t i = 0; i < col_count; i++)
				{
					const char *col_name = nullptr;
					size_t      col_name_len = 0;
					cass_result_column_name(_result, i, &col_name, &col_name_len);

					CassValueType vtype = cass_result_column_type(_result, i);

					axon::column_type ct;
					switch (vtype)
					{
						case CASS_VALUE_TYPE_TINY_INT:
						case CASS_VALUE_TYPE_SMALL_INT:
						case CASS_VALUE_TYPE_INT:
						case CASS_VALUE_TYPE_BIGINT:
						case CASS_VALUE_TYPE_COUNTER:    ct = axon::column_type::int64_t;  break;
						case CASS_VALUE_TYPE_FLOAT:
						case CASS_VALUE_TYPE_DOUBLE:
						case CASS_VALUE_TYPE_DECIMAL:    ct = axon::column_type::double_t; break;
						case CASS_VALUE_TYPE_BOOLEAN:    ct = axon::column_type::bool_t;   break;
						case CASS_VALUE_TYPE_BLOB:       ct = axon::column_type::bytes_t;  break;
						default:                         ct = axon::column_type::string_t; break;
					}

					rs.add_column(std::string(col_name, col_name_len), ct);
				}
				_schema_pushed = true;
			}

			int col_count = (int) cass_result_column_count(_result);
			int count = 0;

			while (count < howmany)
			{
				if (!cass_iterator_next(_iterator))
				{
					_query = false;
					_schema_pushed = false;
					rs.set_eof();
					return;
				}

				const CassRow *row = cass_iterator_get_row(_iterator);
				rs.begin_row();

				for (int i = 0; i < col_count; i++)
				{
					const CassValue *val = cass_row_get_column(row, i);
					const CassDataType *dt = cass_value_data_type(val);
					CassError ce = CASS_OK;

					if (cass_value_is_null(val)) { rs.push_null(); continue; }

					switch (cass_data_type_type(dt))
					{
						case CASS_VALUE_TYPE_TINY_INT:
						{
							cass_int8_t v = 0;
							if ((ce = cass_value_get_int8(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get int8 at col %d: %s", i, cass_error_desc(ce));
							rs.push_int(v);
							break;
						}
						case CASS_VALUE_TYPE_SMALL_INT:
						{
							cass_int16_t v = 0;
							if ((ce = cass_value_get_int16(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get int16 at col %d: %s", i, cass_error_desc(ce));
							rs.push_int(v);
							break;
						}
						case CASS_VALUE_TYPE_INT:
						{
							cass_int32_t v = 0;
							if ((ce = cass_value_get_int32(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get int32 at col %d: %s", i, cass_error_desc(ce));
							rs.push_int(v);
							break;
						}
						case CASS_VALUE_TYPE_BIGINT:
						case CASS_VALUE_TYPE_COUNTER:
						{
							cass_int64_t v = 0;
							if ((ce = cass_value_get_int64(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get int64 at col %d: %s", i, cass_error_desc(ce));
							rs.push_int(v);
							break;
						}
						case CASS_VALUE_TYPE_FLOAT:
						{
							cass_float_t v = 0.0f;
							if ((ce = cass_value_get_float(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get float at col %d: %s", i, cass_error_desc(ce));
							rs.push_double(static_cast<double>(v));
							break;
						}
						case CASS_VALUE_TYPE_DOUBLE:
						case CASS_VALUE_TYPE_DECIMAL:
						{
							cass_double_t v = 0.0;
							if ((ce = cass_value_get_double(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get double at col %d: %s", i, cass_error_desc(ce));
							rs.push_double(v);
							break;
						}
						case CASS_VALUE_TYPE_BOOLEAN:
						{
							cass_bool_t v = cass_false;
							if ((ce = cass_value_get_bool(val, &v)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get bool at col %d: %s", i, cass_error_desc(ce));
							rs.push_bool(v == cass_true);
							break;
						}
						case CASS_VALUE_TYPE_BLOB:
						{
							const cass_byte_t *b = nullptr;
							size_t blen = 0;
							if ((ce = cass_value_get_bytes(val, &b, &blen)) != CASS_OK)
								throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to get blob at col %d: %s", i, cass_error_desc(ce));
							rs.push_bytes(b, blen);
							break;
						}
						default:
						{
							const char *tmp = nullptr;
							size_t sz = 0;
							cass_value_get_string(val, &tmp, &sz);
							rs.push_string(std::string(tmp, sz));
							break;
						}
					}
				}

				rs.end_row();
				count++;
			}
		}

		void scylla::done()
		{
			BENCHMARK;

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "database not connected");

			if (_iterator) { cass_iterator_free(_iterator); _iterator = nullptr; }
		    if (_result) { cass_result_free(_result); _result = nullptr; }

			_bind.clear();
			_query = false;
			_prepared = false;
			_schema_pushed = false;
		}

		std::string& scylla::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE2R_HOSTNAME)
				return _hostname;
			else if (i == AXON_DATABASE2R_USERNAME)
				return _username;
			else if (i == AXON_DATABASE2R_PASSWORD)
				return _password;
			else if (i == AXON_DATABASE2R_KEYSPACE)
				return _keyspace;

			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid index");
			return _throwawaystr;
		}

		int& scylla::operator[] (int i)
		{
			if (i == AXON_DATABASE2R_PORT)
				return _port;

			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid index");
			return _throwawayint;
		}
	}
}

