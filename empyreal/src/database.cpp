#include <main.h>

namespace tcn
{
	namespace database
	{
		sqlite::sqlite()
		{
			_filename = nullptr;
			_username = nullptr;
			_password = nullptr;
		}

		sqlite::sqlite(std::string filename, std::string username, std::string password)
		{
			_filename = filename;
			_username = username;
			_password = password;
		}

		sqlite::~sqlite()
		{
		}

		int sqlite::reset()
		{
			// _filename = "";
			// _username = "";
			// _password = "";

			// reset db connection and reopen

			return true;
		}

		std::string sqlite::operator[](char i)
		{
			if (i == CFG_DB_FILENAME)
				return _filename;
			else if (i == CFG_DB_USERNAME)
				return _username;
			else if (i == CFG_DB_PASSWORD)
				return _password;

			return 0;
		}

		int sqlite::operator[] (int i)
		{
			if (i == CFG_DB_TYPE)
				return _type;

			return 0;
		}

		bool sqlite::open()
		{
			char *errmsg = 0;
			int rc;
			
			rc = sqlite3_open(_filename.c_str(), &_dbp);

			if (rc != SQLITE_OK)
			{
				std::cout<<"There was an error opening "<<_filename<<" for reading!"<<std::endl;
				return false;
			}
			else
			{
				sqlite3_exec(_dbp, "PRAGMA synchronous = OFF", NULL, NULL, &errmsg);
				sqlite3_exec(_dbp, "PRAGMA journal_mode = MEMORY", NULL, NULL, &errmsg);
			}

			return true;
		}

		bool sqlite::open(std::string filename)
		{
			_filename = filename;
			open();
		}

		bool sqlite::close()
		{
			sqlite3_close(_dbp);
		}

		bool sqlite::execute(std::string nodename, std::string query)
		{
			char *errmsg = 0;
			int rc;

			if (!_dbp)
				return true;

			std::lock_guard<std::mutex> lock(mu);
			rc = sqlite3_exec(_dbp, query.c_str(), 0, 0, &errmsg);
			
			if (rc != SQLITE_OK )
			{
				if (!strstr(errmsg, "already exists"))
				{
					std::cout<<"Error running query"<<std::endl;
					sqlite3_free(errmsg);

					return false;
				}

				sqlite3_free(errmsg);
			}
		
			return true;
		}

		sqlite3_stmt *sqlite::query(std::string nodename, std::string query, int (*callback)(sqlite3_stmt*))
		{
			sqlite3_stmt *stmt;
			char *errmsg = 0;
			int rc;

			if (!_dbp)
				return false;

			rc = sqlite3_prepare_v2(_dbp, query.c_str(), -1, &stmt, 0);

			if (rc != SQLITE_OK )
			{
				//handle some error here
				sqlite3_free(errmsg);
			} else {

				while (true)
				{
					int s = sqlite3_step(stmt);

					if (s == SQLITE_ROW)
					{
						callback(stmt);
					} else if (s == SQLITE_DONE) {

						break;
					} else {

					}
				}
			}

			sqlite3_finalize(stmt);

			return stmt;
		}
	}
}
