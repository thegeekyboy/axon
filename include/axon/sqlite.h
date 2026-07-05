#ifndef AXON_SQLITE_H_
#define AXON_SQLITE_H_

#include <mutex>
#include <memory>

#include <string.h>

#include <sqlite3.h>

#include <axon/database2r.h>

namespace axon
{
	namespace database2r
	{
		class sqlite: public connector {

			struct statement {

				statement(): _statement(nullptr), _session(nullptr) { };
				~statement();

				void reset();
				sqlite3_stmt *get() { return _statement; }
				void prepare(sqlite3 *, std::string);
				void execute();

				private:
					sqlite3_stmt* _statement;
					sqlite3 *_session;
			};

			std::string _path, _name, _username, _password;
			std::shared_ptr<sqlite3> _dbp;
			statement _statement;

			std::mutex _safety;

			int bind(statement&);

		public:

			using connector::execute;
			using connector::query;

			sqlite() = default;
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

			void fetch(axon::recordset2r &, int) override;
			void done() override;

			std::string& operator[] (char);
			int& operator[] (int);
		};
	}
}

#endif

