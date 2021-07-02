#ifndef DATABASE_H_
#define DATABASE_H_

namespace tcn
{
	namespace database
	{
		class sqlite {

			int _type;
			std::string _filename, _username, _password;
			sqlite3 *_dbp;

			std::mutex mu;

		public:
			sqlite();
			sqlite(std::string, std::string, std::string);
			~sqlite();

			int reset();
			bool open();
			bool open(std::string);
			bool close();

			bool execute(std::string, std::string);
			sqlite3_stmt *query(std::string, std::string, int (*)(sqlite3_stmt*));

			std::string operator[] (char i);
			int operator[] (int i);
		};
	}
}

#endif