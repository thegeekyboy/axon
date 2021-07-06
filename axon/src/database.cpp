#include <axon.h>
#include <database.h>

namespace axon
{
	namespace database
	{
		sqlite::sqlite()
		{
			// Nothing to construct?
			_open = false;
			_query = false;
		}

		sqlite::sqlite(std::string filename)
		{
			// Nothing to construct?
			_open = false;
			_query = false;

			open(filename);
		}

		sqlite::~sqlite()
		{
			// Destruct something
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

		bool sqlite::open(std::string filename)
		{
			int retcode;
			char *errmsg = 0;

			if (_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database already open");

			retcode = sqlite3_open(filename.c_str(), &_dbp);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Cannot open database (" + filename + ") - " + sqlite3_errmsg(_dbp));

			sqlite3_exec(_dbp, "PRAGMA synchronous = OFF", NULL, NULL, &errmsg);
			//sqlite3_exec(inst->db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			sqlite3_exec(_dbp, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			//sqlite3_exec(_dbp, "PRAGMA journal_mode = WAL", NULL, NULL, &errmsg);

			_open = true;
			return true;
		}

		bool sqlite::close()
		{
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Cannot close while query in progress");
			// Need to insert a procedure to check if there are any finalization pending on prepared statments

			retcode = sqlite3_close(_dbp);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "There was an error closing the database");

			return true;
		}

		bool sqlite::flush()
		{
			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			sqlite3_exec(_dbp, "COMMIT;", 0, 0, 0);

			return true;
		}

		bool sqlite::execute(std::string sqltext)
		{
			int retcode;
			char *errmsg;
			std::string errstr;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Cannot execute while query in progress");

			{
				// Scope the lock guard locally just in case
				std::lock_guard<std::mutex> lock(_safety);
				retcode = sqlite3_exec(_dbp, sqltext.c_str(), 0, 0, &errmsg);
			}

			if (retcode != SQLITE_OK)
			{
				errstr = errmsg;
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Error executing sql statement - " + errstr);
			}

			return true;
		}

		bool sqlite::query(std::string sqltext)
		{
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			if (_query)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Another query is in progress");

			retcode = sqlite3_prepare_v2(_dbp, sqltext.c_str(), sqltext.size(), &_stmt, 0);

			if (retcode != SQLITE_OK)
			{
				sqlite3_reset(_stmt);
				sqlite3_finalize(_stmt);

				throw axon::exception(__FILENAME__, __LINE__, __func__, "There was an error compiling sql statement");
			}

			_query = true;
			_index = 1;

			return true;
		}

		bool sqlite::next()
		{
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "No fetch in progress");

			retcode = sqlite3_step(_stmt);

			if (retcode == SQLITE_ROW)
			{
				_colidx = 0;
				return true;
			}
			else if (retcode == SQLITE_DONE)
				return false;
			else
				throw axon::exception(__FILENAME__, __LINE__, __func__, "There was an error traversing result");

			return true;
		}

		bool sqlite::done()
		{
			int retcode;

			if (!_open)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Database not open");

			if (!_query)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "No statement compiled to close");

			retcode = sqlite3_finalize(_stmt);
			_query = false;

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Error freeing complied statement");

			return true;
		}

		sqlite& sqlite::operator<<(int value)
		{
			int retcode;

			retcode = sqlite3_bind_int(_stmt, _index, value);

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "There was an error binding values");

			_index++;

			return *this;
		}

		sqlite& sqlite::operator<<(std::string& value)
		{
			int retcode;

			retcode = sqlite3_bind_text(_stmt, _index, value.c_str(), -1, SQLITE_TRANSIENT);

			// Need to find an alternative to this?!
			// if (sqlite3_column_type(_stmt, _index) != SQLITE_TEXT)
				// throw axon::exception(__FILENAME__, __LINE__, __func__, "Bind variable type is not compatable with row type");

			if (retcode != SQLITE_OK)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "There was an error binding values");

			_index++;

			return *this;
		}

		sqlite& sqlite::operator>>(int &value)
		{
			if (_colidx >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, _colidx) != SQLITE_INTEGER)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Variable type is not compatable with row type");

			value = sqlite3_column_int(_stmt, _colidx);

			_colidx++;

			return *this;
		}

		sqlite& sqlite::operator>>(std::string &value)
		{
			char *tmp;

			if (_colidx >= sqlite3_column_count(_stmt))
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Column out of bounds");

			if (sqlite3_column_type(_stmt, _colidx) != SQLITE_TEXT)
				throw axon::exception(__FILENAME__, __LINE__, __func__, "Variable type is not compatable with row type");

			if ((tmp = (char *) sqlite3_column_text(_stmt, _colidx)) != NULL)
				value = tmp;
			else
				value = "";

			_colidx++;

			return *this;
		}
	}
}
