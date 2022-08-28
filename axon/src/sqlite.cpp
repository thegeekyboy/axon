#include <axon.h>
#include <axon/sqlite.h>

namespace axon
{
	namespace database
	{
		sqlite::sqlite()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			_open = false;
			_query = false;
		}

		sqlite::sqlite(std::string filename)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			_open = false;
			_query = false;
			_path = filename;

			connect();
		}

		sqlite::sqlite(const sqlite &lhs)
		{
			_dbp = lhs._dbp;
		}

		sqlite::~sqlite()
		{
			if (_open)
			{
				flush();

				try {
					close();
				} catch (axon::exception& e) {

					std::cout<<e.what()<<std::endl;
				}
			}
		}

		bool sqlite::connect()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;
			char *errmsg = 0;

			if (_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database already open");

			retcode = sqlite3_open(_path.c_str(), &_dbp);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot open database (" + _path + ") - " + sqlite3_errmsg(_dbp));

			sqlite3_exec(_dbp, "PRAGMA synchronous = OFF", NULL, NULL, &errmsg);
			//sqlite3_exec(inst->db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			sqlite3_exec(_dbp, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			//sqlite3_exec(_dbp, "PRAGMA journal_mode = WAL", NULL, NULL, &errmsg);

			_open = true;
			return _open;
		}

		bool sqlite::connect(std::string filename, std::string username, std::string password)
		{
			_path = filename;
			connect();
			return true;
		}

		bool sqlite::close()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot close while query in progress");
			// Need to insert a procedure to check if there are any finalization pending on prepared statements

			retcode = sqlite3_close(_dbp);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error closing the database");

			return true;
		}

		bool sqlite::flush()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			sqlite3_exec(_dbp, "COMMIT;", 0, 0, 0);

			return true;
		}

		bool sqlite::ping()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			return true;
		}

		void sqlite::version()
		{
			axon::timer t1(__PRETTY_FUNCTION__);

		}

		bool sqlite::transaction(axon::trans_t ttype)
		{
			if (ttype == axon::transaction::BEGIN)
				return execute("BEGIN TRANSACTION;");
			else if (ttype == axon::transaction::END)
				return execute("END TRANSACTION;");

			return false;
		}

		bool sqlite::execute(const std::string &sqltext)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;
			char *errmsg;
			std::string errstr;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot execute while query in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);
				retcode = sqlite3_exec(_dbp, sqltext.c_str(), 0, 0, &errmsg);
			}

			if (retcode != SQLITE_OK)
			{
				errstr = errmsg;
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error executing sql statement - " + errstr + " for " + sqltext);
			}

			return true;
		}

		bool sqlite::execute(const std::string &sqltext, axon::database::bind *first, ...)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			return true;
		}

		bool sqlite::query(std::string sqltext)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Another query is in progress");

			retcode = sqlite3_prepare_v2(_dbp, sqltext.c_str(), sqltext.size(), &_stmt, 0);

			if (retcode != SQLITE_OK)
			{
				sqlite3_reset(_stmt);
				sqlite3_finalize(_stmt);

				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error compiling sql statement");
			}

			_query = true;
			_index = 1;

			return true;
		}

		bool sqlite::next()
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No fetch in progress");

			retcode = sqlite3_step(_stmt);

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
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Database not open");

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No statement compiled to close");

			retcode = sqlite3_finalize(_stmt);
			_query = false;

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Error freeing complied statement");
		}

		std::string sqlite::get(unsigned int position)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			char *tmp;
			std::string value;

			if ((int) position >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, position) != SQLITE_TEXT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			if ((tmp = (char *) sqlite3_column_text(_stmt, position)) != NULL)
				value = tmp;
			else
				value = "";

			return value;
		}

		sqlite& sqlite::operator<<(int value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			retcode = sqlite3_bind_int(_stmt, _index, value);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error binding values");

			_index++;

			return *this;
		}

		sqlite& sqlite::operator<<(std::string& value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			int retcode;

			retcode = sqlite3_bind_text(_stmt, _index, value.c_str(), -1, SQLITE_TRANSIENT);

			// Need to find an alternative to this?!
			// if (sqlite3_column_type(_stmt, _index) != SQLITE_TEXT)
				// throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Bind variable type is not compatable with row type");

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error binding values");

			_index++;

			return *this;
		}

		std::ostream& sqlite::printer(std::ostream &stream)
		{

			return stream;
		}

		sqlite& sqlite::operator>>(int &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			if (_colidx >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, _colidx) != SQLITE_INTEGER)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_int(_stmt, _colidx);

			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(double &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			if (_colidx >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, _colidx) != SQLITE_INTEGER)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			value = sqlite3_column_double(_stmt, _colidx);

			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(std::string &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);
			char *tmp;

			if (_colidx >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, _colidx) != SQLITE_TEXT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Variable type is not compatable with row type");

			if ((tmp = (char *) sqlite3_column_text(_stmt, _colidx)) != NULL)
				value = tmp;
			else
				value = "";

			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(std::time_t &value)
		{

			return *this;
		}
	}
}