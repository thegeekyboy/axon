#ifndef AXON_DATABASE_H_
#define AXON_DATABASE_H_

#include <sqlite3.h>

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
		class sqlite {

			bool _open, _query;
			int _type, _index, _colidx;
			std::string _path, _name, _username, _password;
			
			sqlite3 *_dbp;
			sqlite3_stmt *_stmt;

			std::mutex _safety;

		public:
			sqlite();
			sqlite(std::string);
			~sqlite();

			bool open(std::string);
			bool close();
			bool flush();

			bool execute(std::string);

			bool query(std::string);
			bool next();
			bool done();

			sqlite& operator<<(int);
			sqlite& operator<<(std::string&);

			sqlite& operator>>(int&);
			sqlite& operator>>(std::string&);
		};
	}
}

#endif