#ifndef AXON_DATABASE2R_H_
#define AXON_DATABASE2R_H_

#include <any>
#include <boost/regex.hpp>

#include <axon/recordset2r.h>

#define AXON_DATABASE2R_HOSTNAME 'a'
#define AXON_DATABASE2R_USERNAME 'b'
#define AXON_DATABASE2R_PASSWORD 'c'
#define AXON_DATABASE2R_KEYSPACE 'd'
#define AXON_DATABASE2R_FILEPATH 'e'

namespace axon {

	namespace database2r {

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

				std::vector<axon::database2r::bind> _bind;

				std::string _throwawaystr;
				int _throwawayint;

				std::string _version;

				bool _schema_pushed;

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

				virtual std::ostream& printer(std::ostream &) = 0;

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
					if constexpr (std::is_same<T, std::string>::value)
						_bind.push_back(t.c_str());
					else
						_bind.push_back(t);
					return execute(sql);
				}

				template<typename T, typename... Args>
				bool execute(std::string sql, T t, Args... args)
				{
					if constexpr (std::is_same<T, std::string>::value)
						_bind.push_back(t.c_str());
					else
						_bind.push_back(t);
					return execute(sql, args...) ;
				}

				virtual bool query(const std::string) = 0;

				template <typename T>
				bool query(const std::string sql, T t)
				{
					if constexpr (std::is_same<T, std::string>::value)
						_bind.push_back(t.c_str());
					else
						_bind.push_back(t);
					return query(sql);
				}

				template<typename T, typename... Args>
				bool query(std::string sql, T t, Args... args)
				{
					if constexpr (std::is_same<T, std::string>::value)
						_bind.push_back(t.c_str());
					else
						_bind.push_back(t);
					return query(sql, args...) ;
				}

				virtual connector& operator<<(int) = 0;
				virtual connector& operator<<(long) = 0;
				virtual connector& operator<<(long long) = 0;
				virtual connector& operator<<(float) = 0;
				virtual connector& operator<<(double) = 0;
				virtual connector& operator<<(std::string&) = 0;
				virtual connector& operator<<(axon::database2r::bind&) = 0;

				virtual void fetch(axon::recordset2r&, int) = 0;
				bool done() { return true; };
		};
	}
}

#endif

