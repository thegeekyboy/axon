#include <iostream>

#include <axon.h>
#include <axon/util.h>
#include <axon/sqlite.h>

namespace axon
{
	namespace database
	{
		sqlite::statement::~statement() {
			reset();
		};

		void sqlite::statement::reset() {
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (_statement != NULL) {
				sqlite3_reset(_statement);
				sqlite3_finalize(_statement);
				_statement = NULL;
			}
		}

		void sqlite::statement::prepare(sqlite3 *session, std::string sql) {
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (_statement != NULL) {
				sqlite3_reset(_statement);
				sqlite3_finalize(_statement);
			}

			if (sqlite3_prepare_v2(session, sql.c_str(), sql.size(), &_statement, 0) != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error compiling sql statement, driver: %s", sqlite3_errmsg(session));

			_session = session;
		};

		void sqlite::statement::execute() {
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (_statement == NULL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No prepared statement to execute!");

			sqlite3_step(_statement);
			sqlite3_reset(_statement);
		}

		int sqlite::prepare(int count, va_list *list, axon::database::bind *first)
		{
			axon::database::bind *element = first;
			int index;

			_bind.clear();

			for (index = 0; index < count; index++)
			{
				if (element == nullptr)
					break;

				INFPRN("* Index: %d of %d, Type Index: %s\033[0m", index, count, element->type().name());

				_bind.push_back(*element);

				element = va_arg(*list, axon::database::bind*);
			}

			return 0;
		}

		int sqlite::bind(statement &stmt2)
		{
			int index = 1, count = _bind.size();

			sqlite3_stmt *stmt = stmt2.get();

			for (auto &element : _bind)
			{
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
					if (sqlite3_bind_text(stmt, index, data, strlen(data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(const char*))
				{
					const char *data = std::any_cast<const char *>(element);
					if (sqlite3_bind_text(stmt, index, (char*)data, strlen(data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(unsigned char*))
				{
					unsigned char *data = std::any_cast<unsigned char *>(element);
					if (sqlite3_bind_text(stmt, index, (char*) data, strlen((char*)data), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(std::string))
				{
					std::string data = std::any_cast<std::string>(element);
					if (sqlite3_bind_text(stmt, index, data.c_str(), data.size(), SQLITE_TRANSIENT) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(float))
				{
					float data = std::any_cast<float>(element);
					if (sqlite3_bind_double(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(double))
				{
					double data = std::any_cast<double>(element);
					if (sqlite3_bind_double(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(int8_t))
				{
					int8_t data = std::any_cast<int8_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(int16_t))
				{
					int16_t data = std::any_cast<int16_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(int32_t))
				{
					int32_t data = std::any_cast<int32_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(uint32_t))
				{
					uint32_t data = std::any_cast<uint32_t>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(int64_t))
				{
					int64_t data = std::any_cast<int64_t>(element);
					if (sqlite3_bind_int64(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(uint64_t))
				{
					uint64_t data = std::any_cast<uint64_t>(element);
					if (sqlite3_bind_int64(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else if (element.type() == typeid(bool))
				{
					bool data = std::any_cast<bool>(element);
					if (sqlite3_bind_int(stmt, index, data) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to bind at index: %d, driver: %s", index, sqlite3_errmsg(_dbp));
				}
				else
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unsupported data type %s at index: %d", element.type().name(), index);

				index++;
			}

			_bind.clear();

			return count;
		}

		int sqlite::_get_int(int position)
		{
			int value = 0;

			if (position >= sqlite3_column_count(_statement.get()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds (%d of %d)", position, sqlite3_column_count(_statement.get()));

			if (sqlite3_column_type(_statement.get(), position) != SQLITE_INTEGER)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_int(_statement.get(), position);

			return value;
		}

		long sqlite::_get_long(int position)
		{
			long value = 0;

			if (position >= sqlite3_column_count(_statement.get()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds (%d of %d)", position, sqlite3_column_count(_statement.get()));

			if (sqlite3_column_type(_statement.get(), position) != SQLITE_INTEGER)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_int64(_statement.get(), position);

			return value;
		}

		float sqlite::_get_float(int position)
		{
			float value = 0;

			if (position >= sqlite3_column_count(_statement.get()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds (%d of %d)", position, sqlite3_column_count(_statement.get()));

			if (sqlite3_column_type(_statement.get(), position) != SQLITE_FLOAT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_double(_statement.get(), position);

			return value;
		}

		double sqlite::_get_double(int position)
		{
			double value = 0;

			if (position >= sqlite3_column_count(_statement.get()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds (%d of %d)", position, sqlite3_column_count(_statement.get()));

			if (sqlite3_column_type(_statement.get(), position) != SQLITE_FLOAT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_double(_statement.get(), position);

			return value;
		}

		std::string sqlite::_get_string(int position)
		{
			std::string value;
			char *tmp;

			if (position >= sqlite3_column_count(_statement.get()))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds (%d of %d)", position, sqlite3_column_count(_statement.get()));

			if (sqlite3_column_type(_statement.get(), position) != SQLITE_TEXT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			if ((tmp = (char *) sqlite3_column_text(_statement.get(), position)) != NULL)
				value = tmp;

			return value;
		}

		sqlite::sqlite()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			_open = false;
			_query = false;
			_running = false;
			_prepared = false;
		}

		sqlite::sqlite(std::string filename)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			_open = false;
			_query = false;
			_running = false;
			_prepared = false;
			_path = filename;

			connect();
		}

		sqlite::sqlite(const sqlite &lhs)
		{
			_dbp = lhs._dbp;
		}

		sqlite::~sqlite()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (_open)
			{
				flush();

				try {
					close();
				} catch (axon::exception& e) {

					ERRPRN("%s", e.what());
				}
			}
		}

		bool sqlite::connect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			char *errmsg = 0;

			if (_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database already open");

			if (_path.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Path to file is empty");

			if (sqlite3_open(_path.c_str(), &_dbp) != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot open database (" + _path + ") - " + sqlite3_errmsg(_dbp));

			sqlite3_exec(_dbp, "PRAGMA synchronous = OFF", NULL, NULL, &errmsg);
			//sqlite3_exec(inst->db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			sqlite3_exec(_dbp, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			//sqlite3_exec(_dbp, "PRAGMA journal_mode = WAL", NULL, NULL, &errmsg);

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
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot close while query in progress");
_statement.reset();
			// TODO: Need to insert a procedure to check if there are any finalization pending on prepared statements

			if (sqlite3_close(_dbp) != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error closing the database, driver: %s", sqlite3_errmsg(_dbp));

			return true;
		}

		bool sqlite::flush()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			sqlite3_exec(_dbp, "COMMIT;", 0, 0, 0);

			return true;
		}

		bool sqlite::ping()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			return true;
		}

		std::string sqlite::version()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			return sqlite3_libversion();
		}

		bool sqlite::transaction(trans_t ttype)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (ttype == transaction::BEGIN)
				return execute("BEGIN TRANSACTION;");
			else if (ttype == transaction::END)
				return execute("END TRANSACTION;");

			return false;
		}

		bool sqlite::execute(const std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot execute while query in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);
				_statement.prepare(_dbp, sql);
				bind(_statement);
				_statement.execute();
			}

			return true;
		}

		bool sqlite::query(std::string sql)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Another query is in progress");

			if (!_running)
			{
				_running = true;

				{
					// Scope the lock guard locally just in case
					std::lock_guard<std::mutex> lock(_safety);

					_statement.prepare(_dbp, sql);

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

		bool sqlite::next()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (!_query && !_prepared)
				return false;
				// throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No fetch in progress");

			if (!_query && _prepared)
			{
				bind(_statement);
				_statement.execute();

				_query = true;
				_rowidx = 0;
				_colidx = 0;
			}

			retcode = sqlite3_step(_statement.get());

			if (retcode == SQLITE_ROW)
			{
				_colidx = 0;
				return true;
			}
			else if (retcode == SQLITE_DONE)
				return false;
			else
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error traversing result");

			return true;
		}

		void sqlite::done()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No statement compiled to close");

			_statement.reset();
			_bind.clear();

			_query = false;
			_prepared = false;

			_rowidx = 0;
			_colidx = 0;

		}

		std::string& sqlite::operator[](char i)
		{
			_throwawaystr.erase();

			if (i == AXON_DATABASE_FILEPATH)
				return _path;
			else if (i == AXON_DATABASE_USERNAME)
				return _username;
			else if (i == AXON_DATABASE_PASSWORD)
				return _password;

			return _throwawaystr;
		}

		int& sqlite::operator[] (int i)
		{
			_throwawayint = i;
			return _throwawayint;
		}

		sqlite& sqlite::operator<<(int value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(long long value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(float value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(double value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(std::string& value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator<<(axon::database::bind &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			_bind.push_back(value);
			_colidx++;

			return *this;
		}

		std::ostream& sqlite::printer(std::ostream &stream)
		{
			return stream;
		}

		sqlite& sqlite::operator>>(int &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_int(_colidx);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(long &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_long(_colidx);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(float &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_float(_colidx);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(double &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_double(_colidx);
			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(std::string &value)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			value = _get_string(_colidx);
			_colidx++;

			return *this;
		}
	}
}
