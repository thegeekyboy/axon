#ifndef AXON_SQLITE_H_
#define AXON_SQLITE_H_

#include <mutex>
#include <memory>

#include <string.h>

#include <sqlite3.h>

#include <axon/database.h>

namespace axon
{
	namespace database
	{
		class sqlite: public interface {

			struct statement {

				statement(): _statement(NULL) { };
				~statement() {
					reset();
				};

				void reset() {
					axon::timer ctm(__PRETTY_FUNCTION__);
					if (_statement != NULL) {
						sqlite3_reset(_statement);
						sqlite3_finalize(_statement);
						_statement = NULL;
					}
				}

				sqlite3_stmt *get() { return _statement; }

				void prepare(sqlite3 *session, std::string sql) {
					axon::timer ctm(__PRETTY_FUNCTION__);
					if (_statement != NULL) {
						sqlite3_reset(_statement);
						sqlite3_finalize(_statement);
					}

					if (sqlite3_prepare_v2(session, sql.c_str(), sql.size(), &_statement, 0) != SQLITE_OK)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error compiling sql statement, driver: %s", sqlite3_errmsg(session));

					_session = session;
				};

				void execute() {
					axon::timer ctm(__PRETTY_FUNCTION__);
					if (_statement == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No prepared statement to execute!");

					sqlite3_step(_statement);
					sqlite3_reset(_statement);
				}

				private:
					sqlite3_stmt* _statement;
					sqlite3 *_session;
			};

			bool _open, _query, _running, _prepared;
			int _type, _index, _rowidx, _colidx;
			std::string _path, _name, _username, _password;
			
			sqlite3 *_dbp;
			sqlite3_stmt *_stmt;

			statement _statement;

			std::vector<axon::database::bind> _bind;

			std::mutex _safety;

			int prepare(int, va_list*, axon::database::bind*);
			int bind(statement&);

		protected:
			std::ostream& printer(std::ostream&);

		public:
			int _get_int(int);
			long _get_long(int);
			float _get_float(int);
			double _get_double(int);
			std::string _get_string(int);

			sqlite();
			sqlite(std::string);
			sqlite(const sqlite&);

			~sqlite();

			bool connect();
			bool connect(std::string, std::string, std::string);
			bool close();
			bool flush();

			bool ping();
			std::string version();
			
			bool transaction(trans_t);

			bool execute(const std::string);
			bool execute(const std::string, axon::database::bind, ...);

			bool query(const std::string, axon::database::bind, ...);
			bool query(const std::string);

			bool next();
			void done();

			std::string& operator[] (char);
			int& operator[] (int);

			sqlite& operator<<(int);
			sqlite& operator<<(long);
			sqlite& operator<<(float);
			sqlite& operator<<(double);
			sqlite& operator<<(std::string&);
			sqlite& operator<<(axon::database::bind&);

			sqlite& operator>>(int&);
			sqlite& operator>>(float&);
			sqlite& operator>>(double&);
			sqlite& operator>>(std::string&);
			sqlite& operator>>(long&);
		};
	}
}

#endif