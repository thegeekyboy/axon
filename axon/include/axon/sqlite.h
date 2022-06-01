#ifndef AXON_SQLITE_H_
#define AXON_SQLITE_H_

#include <mutex>

#include <string.h>

#include <sqlite3.h>

#include <axon/database.h>

#define CFG_DB_PATH 'a'
#define CFG_DB_NAME 'b'
#define CFG_DB_USERNAME 'c'
#define CFG_DB_PASSWORD 'd'
#define CFG_DB_GTT 'e'
#define CFG_DB_FILELIST 'f'
#define CFG_DB_ERROR 'g'
#define CFG_DB_TYPE 1

namespace axon
{
	namespace database
	{
		class sqlite: public interface {

			bool _open, _query;
			int _type, _index, _colidx;
			std::string _path, _name, _username, _password;
			
			sqlite3 *_dbp;
			sqlite3_stmt *_stmt;

			std::mutex _safety;

		protected:
			std::ostream& printer(std::ostream&);

		public:
			sqlite();
			sqlite(std::string);
			sqlite(const sqlite&);

			~sqlite();

			bool connect();
			bool connect(std::string, std::string, std::string);
			bool close();
			bool flush();

			bool ping();
			void version();

			bool execute(const std::string&);
			bool execute(const std::string&, axon::database::bind*, ...);

			bool query(std::string);
			bool next();
			void done();

			std::string get(unsigned int);

			sqlite& operator<<(int);
			sqlite& operator<<(std::string&);

			sqlite& operator>>(int&);
			sqlite& operator>>(double&);
			sqlite& operator>>(std::string&);
			sqlite& operator>>(std::time_t&);
		};
	}
}

#endif