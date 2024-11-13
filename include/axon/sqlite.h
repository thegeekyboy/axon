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

			std::mutex _safety;

			int prepare(int, va_list*, axon::database::bind*);
			int bind(statement&);

		protected:
			std::ostream& printer(std::ostream&) override;

		public:
			int _get_int(int) override;
			long _get_long(int) override;
			float _get_float(int) override;
			double _get_double(int) override;
			std::string _get_string(int) override;

			sqlite();
			sqlite(std::string);
			sqlite(const sqlite&);

			~sqlite();

			bool connect() override;
			bool connect(std::string, std::string, std::string) override;

			bool close() override;
			bool flush() override;

			bool ping() override;
			std::string version() override;

			bool transaction(trans_t) override;

			bool execute(const std::string) override;
			bool query(const std::string) override;

			bool next();
			void done();

			std::string& operator[] (char);
			int& operator[] (int);

			sqlite& operator<<(int) override;
			sqlite& operator<<(long) override;
			sqlite& operator<<(long long) override;
			sqlite& operator<<(float) override;
			sqlite& operator<<(double) override;
			sqlite& operator<<(std::string&) override;
			sqlite& operator<<(axon::database::bind&) override;

			sqlite& operator>>(int&) override;
			sqlite& operator>>(long&) override;
			sqlite& operator>>(float&) override;
			sqlite& operator>>(double&) override;
			sqlite& operator>>(std::string&) override;
		};
	}
}

#endif
