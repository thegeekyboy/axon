#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>

namespace axon
{
	namespace database2r
	{
		sqlite::statement::~statement()
		{
			reset();
		};

		void sqlite::statement::reset()
		{
			BENCHMARK;

			if (_statement != nullptr) {
				sqlite3_finalize(_statement);
				_statement = nullptr;
			}
		}

		void sqlite::statement::prepare(sqlite3 *session, std::string sql)
		{
			BENCHMARK;

			reset();

			if (sqlite3_prepare_v2(session, sql.c_str(), sql.size(), &_statement, 0) != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error compiling sql statement, driver: %s", sqlite3_errmsg(session));

			_session = session;
		};

		void sqlite::statement::execute()
		{
			BENCHMARK;

			if (_statement == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No prepared statement to execute!");

			int rc = sqlite3_step(_statement);
			if (rc != SQLITE_DONE && rc != SQLITE_ROW)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "execute failed: %s", sqlite3_errmsg(_session));

			sqlite3_reset(_statement);
		}

		int sqlite::bind(statement &stmt2)
		{
			int index = 1, count = _bind.size();

			sqlite3_stmt *stmt = stmt2.get();

			for (auto &element : _bind)
			{
				INFPRN("+ Index: %d of %d, Type Index: %s", index, count, element.type().name());

				if (element.type() == typeid(char*))
				{
					char *data = std::any_cast<char *>(element);
					if (sqlite3_bind_text(stmt, index, data, strlen(data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					if (sqlite3_bind_text(stmt, index, (char*)data, strlen(data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char *data = std::any_cast<unsigned char *>(element);
					if (sqlite3_bind_text(stmt, index, (char*) data, strlen((char*)data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(std::string))
				{
					const std::string &data = std::any_cast<const std::string&>(element);
					if (sqlite3_bind_text(stmt, index, data.c_str(), data.size(), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(float))
				{
					float data = std::any_cast<float>(element);
					if (sqlite3_bind_double(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(double))
				{
					double data = std::any_cast<double>(element);
					if (sqlite3_bind_double(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(int8_t))
				{
					int8_t data = std::any_cast<int8_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(int16_t))
				{
					int16_t data = std::any_cast<int16_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(int32_t))
				{
					int32_t data = std::any_cast<int32_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(uint32_t))
				{
					uint32_t data = std::any_cast<uint32_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(int64_t))
				{
					int64_t data = std::any_cast<int64_t>(element);
					if (sqlite3_bind_int64(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(uint64_t))
				{
					uint64_t data = std::any_cast<uint64_t>(element);
					if (sqlite3_bind_int64(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(bool))
				{
					bool data = std::any_cast<bool>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(std::vector<uint8_t>))
				{
					std::vector<uint8_t> data = std::any_cast<std::vector<uint8_t>>(element);
					if (sqlite3_bind_blob(stmt, index, data.data(), data.size(), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp.get()));
				}
				else if (element.type() == typeid(std::vector<std::string>) || element.type() == typeid(std::vector<double>) || element.type() == typeid(std::vector<int>))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "array type data as bind variable not supported by sqlite");
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);

				index++;
			}

			_bind.clear();

			return count;
		}

		sqlite::sqlite(std::string filename)
		{
			BENCHMARK;

			_path = filename;

			connect();
		}

		sqlite::sqlite(const sqlite &lhs)
		{
			_path     = lhs._path;

			if (!_path.empty() && axon::util::exists(_path))
				connect();
		}

		sqlite::~sqlite()
		{
			BENCHMARK;

			if (_open) close();
		}

		bool sqlite::connect()
		{
			BENCHMARK;
			char *errmsg = 0;
			sqlite3 *ptr = { nullptr };

			if (_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database already open");

			if (_path.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path to file is empty");

			if (!axon::util::iswritable(_path))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "%s to file not writable", _path.c_str());

			if (sqlite3_open(_path.c_str(), &ptr) != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot open database2r (" + _path + ")");

			_dbp.reset(ptr, [](sqlite3* db) { sqlite3_close(db); });

			sqlite3_exec(_dbp.get(), "PRAGMA synchronous = OFF", NULL, NULL, &errmsg);
			sqlite3_exec(_dbp.get(), "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg); // journal_mode can be MEMORY or WAL

			_open = true;
			return _open;
		}

		bool sqlite::connect(std::string filename, std::string, std::string)
		{
			_path = filename;

			return connect();
		}

		bool sqlite::close()
		{
			BENCHMARK;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot close while query in progress");

			
			_statement.reset();
			
			_open = false;
			
			sqlite3_exec(_dbp.get(), "COMMIT;", nullptr, nullptr, nullptr);
			_dbp.reset();

			_query = false;
			_prepared = false;
			_schema_pushed = false;

			return true;
		}

		bool sqlite::flush()
		{
			BENCHMARK;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			sqlite3_exec(_dbp.get(), "COMMIT;", nullptr, nullptr, nullptr);

			return true;
		}

		bool sqlite::ping()
		{
			BENCHMARK;
			return true;
		}

		std::string sqlite::version()
		{
			BENCHMARK;
			return sqlite3_libversion();
		}

		bool sqlite::transaction(trans_t ttype)
		{
			BENCHMARK;
			if (ttype == transaction::BEGIN)
				return execute("BEGIN TRANSACTION;");
			else if (ttype == transaction::END)
				return execute("END TRANSACTION;");

			return false;
		}

		bool sqlite::execute(const std::string sql)
		{
			BENCHMARK;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot execute while query in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);
				_statement.prepare(_dbp.get(), sql);
				bind(_statement);
				_statement.execute();
			}

			return true;
		}

		bool sqlite::query(const std::string sql)
		{
			BENCHMARK;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Another query is in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);

				_statement.prepare(_dbp.get(), sql);

				_prepared = true;
			}

			return true;
		}

		void sqlite::fetch(axon::recordset2r &rs, int howmany)
		{
			BENCHMARK;
		
			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");
		
			if (!_query && !_prepared)
			{
				rs.set_eof();
				return;
			}
		
			// First fetch call: bind parameters and initialise traversal.
			if (!_query && _prepared)
			{
				bind(_statement);
				_query  = true;
			}
		
			int col_count = sqlite3_column_count(_statement.get());
			int count     = 0;
		
			while (count < howmany)
			{
				int rc = sqlite3_step(_statement.get());
		
				if (rc == SQLITE_DONE)
				{
					// Fully exhausted — clean up state and signal EOF to recordset.
					_query = false;
					_prepared = false;
					rs.set_eof();
					return;
				}
		
				if (rc != SQLITE_ROW)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "sqlite3_step error: %s", sqlite3_errmsg(_dbp.get()));
		
				// Build schema from first row — types are only reliable after step().
				if (!_schema_pushed)
				{
					for (int i = 0; i < col_count; i++)
					{
						const char *col_name  = sqlite3_column_name(_statement.get(), i);
						const char *decl_type = sqlite3_column_decltype(_statement.get(), i);

						axon::column_type ct = axon::column_type::string_t;   // safe default

						if (decl_type)
						{
							std::string dt(decl_type);
							// SQLite type affinity rules (https://sqlite.org/datatype3.html)
							
							if (dt.find("INT")  != std::string::npos) ct = axon::column_type::int64_t;
							else if (dt.find("REAL") != std::string::npos || dt.find("FLOA") != std::string::npos || dt.find("DOUB") != std::string::npos) ct = axon::column_type::double_t;
							else if (dt.find("BLOB") != std::string::npos) ct = axon::column_type::bytes_t;
							else if (dt.find("BOOL") != std::string::npos) ct = axon::column_type::bool_t;
							else ct = axon::column_type::string_t;
						}
						else
						{
							// Expression column — fall back to runtime type of this first row
							switch (sqlite3_column_type(_statement.get(), i))
							{
								case SQLITE_INTEGER: ct = axon::column_type::int64_t;  break;
								case SQLITE_FLOAT: ct = axon::column_type::double_t; break;
								case SQLITE_BLOB: ct = axon::column_type::bytes_t;  break;
								default: ct = axon::column_type::string_t; break;
							}
						}
						rs.add_column(col_name, ct);
					}
					_schema_pushed = true;
				}
		
				rs.begin_row();
		
				for (int i = 0; i < col_count; i++)
				{
					switch (sqlite3_column_type(_statement.get(), i))
					{
						case SQLITE_INTEGER: rs.push_int(sqlite3_column_int64(_statement.get(), i)); break;
						case SQLITE_FLOAT: rs.push_double(sqlite3_column_double(_statement.get(), i)); break;
						case SQLITE_TEXT: rs.push_string(reinterpret_cast<const char*>(sqlite3_column_text(_statement.get(), i))); break;
						case SQLITE_BLOB: rs.push_bytes(sqlite3_column_blob(_statement.get(), i), sqlite3_column_bytes(_statement.get(), i)); break;
						case SQLITE_NULL: rs.push_null(); break;
						default: rs.push_null(); break;
					}
				}
		
				rs.end_row();
				count++;
			}
			INFPRN("size: %d, count: %d", rs.rows(), count);
		}

		void sqlite::done()
		{
		    BENCHMARK;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			_statement.reset();
			_bind.clear();

			_query = false;
			_prepared = false;
			_schema_pushed = false;
		}

		std::string& sqlite::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE2R_FILEPATH)
				return _path;
			else if (i == AXON_DATABASE2R_USERNAME)
				return _username;
			else if (i == AXON_DATABASE2R_PASSWORD)
				return _password;

			return _throwawaystr;
		}

		int& sqlite::operator[] (int i)
		{
			_throwawayint = i;
			return _throwawayint;
		}
	}
}

