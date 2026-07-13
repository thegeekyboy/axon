#ifndef AXON_DATABASE_H_
#define AXON_DATABASE_H_

#include <any>
#include <boost/regex.hpp>

#include <axon/resultset.h>

#define AXON_DATABASE_HOSTNAME 'a'
#define AXON_DATABASE_USERNAME 'b'
#define AXON_DATABASE_PASSWORD 'c'
#define AXON_DATABASE_KEYSPACE 'd'
#define AXON_DATABASE_FILEPATH 'e'

#define AXON_DATABASE_PORT 1

namespace axon {

	namespace database {

		typedef int trans_t;
		using bind = std::any;

		enum operation {
			none = 0x0,
			startup = 0x1,
			shutdown = 0x2,
			shutany = 0x3,
			dropdb = 0x4,
			unregister = 0x5,
			object = 0x6,
			querychange = 0x7
		};

		enum change {
			allops = 0x0,
			allrows = 0x1,
			insert = 0x2,
			update = 0x4,
			remove = 0x8,
			alter = 0x10,
			drop = 0x20,
			unknown = 0x40
		};

		enum exec_type {
			select,
			other
		};

		struct transaction {
			static const trans_t END = 0;
			static const trans_t BEGIN = 1;
		};

		class connector
		{
			protected:

				std::vector<axon::database::bind> _bind;

				std::string _throwawaystr;
				int _throwawayint;

				std::string _version;

				bool _open { false }, _query { false }, _running { false }, _prepared { false }, _schema_pushed { false };

				int _vcount(std::string sql)
				{
					int count = 0;
					boost::regex expression("'(?:[^']|'')*'|\\B(:[a-zA-Z0-9_]+)");

					boost::sregex_iterator it(sql.begin(), sql.end(), expression);
					boost::sregex_iterator end;

					std::for_each(it, end, [&count](const boost::smatch& sm) {
						if (sm[1].length()>0)
							count++;
					});

					return count;
				}

			public:

				virtual bool connect() = 0;
				virtual bool connect(std::string, std::string, std::string) = 0;

				virtual bool close() = 0;
				virtual bool flush() = 0;

				virtual bool ping() = 0;
				virtual std::string version() = 0;

				virtual bool transaction(trans_t) = 0;

				virtual bool execute(const std::string) = 0;

				template <typename T>
				bool execute(const std::string sql, T t)
				{
					_bind.push_back(t);
					return execute(sql);
				}

				template<typename T, typename... Args>
				bool execute(std::string sql, T t, Args... args)
				{
					_bind.push_back(t);
					return execute(sql, args...);
				}

				virtual bool query(const std::string) = 0;

				template <typename T>
				bool query(const std::string sql, T t)
				{
					_bind.push_back(t);
					return query(sql);
				}

				template<typename T, typename... Args>
				bool query(std::string sql, T t, Args... args)
				{
					_bind.push_back(t);
					return query(sql, args...);
				}

				connector& operator<<(int value) { _bind.push_back(value); return *this; }
				connector& operator<<(long value) { _bind.push_back(value); return *this; }
				connector& operator<<(long long value) { _bind.push_back(value); return *this; }
				connector& operator<<(float value) { _bind.push_back(value); return *this; }
				connector& operator<<(double value) { _bind.push_back(value); return *this; }
				connector& operator<<(std::string &value) { _bind.push_back(value); return *this; }
				connector& operator<<(axon::database::bind &value) { _bind.push_back(value); return *this; }

				virtual void fetch(axon::resultset&, int) = 0;
				virtual void done() = 0;
		};
	}
}

#endif