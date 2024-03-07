#ifndef AXON_DATABASE_H_
#define AXON_DATABASE_H_

#include <any>
#include <variant>
#include <boost/regex.hpp>

#define AXON_DATABASE_HOSTNAME 'a'
#define AXON_DATABASE_USERNAME 'b'
#define AXON_DATABASE_PASSWORD 'c'
#define AXON_DATABASE_KEYSPACE 'd'
#define AXON_DATABASE_FILEPATH 'e'

namespace axon {

	namespace database {

		typedef int trans_t;
		// typedef std::variant<std::vector<std::string>, std::vector<double>, std::vector<int>, void *, char *, unsigned char*, float, double, int8_t, int16_t, int32_t, uint32_t, int64_t, uint64_t, bool> bind;
		using bind = std::any;

		struct transaction {

			static const trans_t END = 0;
			static const trans_t BEGIN = 1;
		};

		class interface
		{
			protected:
				std::string _throwawaystr;
				int _throwawayint;

				std::string _version;

				int _vcount(std::string sql)
				{
					axon::timer(__PRETTY_FUNCTION__);

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

				virtual int _get_int(int) = 0;
				virtual long _get_long(int) = 0;
				virtual float _get_float(int) = 0;
				virtual double _get_double(int) = 0;
				virtual std::string _get_string(int) = 0;

				virtual bool connect() = 0;
				virtual bool connect(std::string, std::string, std::string) = 0;

				virtual bool close() = 0;
				virtual bool flush() = 0;

				virtual bool ping() = 0;
				virtual std::string version() = 0;

				virtual bool transaction(trans_t) = 0;

				virtual bool execute(const std::string) = 0;
				virtual bool execute(const std::string, axon::database::bind, ...) = 0;

				virtual bool query(const std::string) = 0;
				virtual bool query(const std::string, axon::database::bind, ...) = 0;


				virtual bool next() = 0;
				virtual void done() = 0;

				template <typename T>
				bool get(int position, T& p) {

					if constexpr(std::is_same<T, int>::value)
						p = static_cast<T>(_get_int(position));
					else if constexpr(std::is_same<T, long>::value)
						p = static_cast<T>(_get_long(position));
					else if constexpr(std::is_same<T, float>::value)
						p = static_cast<T>(_get_float(position));
					else if constexpr(std::is_same<T, double>::value)
						p = static_cast<T>(_get_double(position));
					else if constexpr(std::is_same<T, std::string>::value)
						p = static_cast<T>(_get_string(position));
					else return false;

					return true;
				}

				virtual interface& operator<<(int) = 0;
				virtual interface& operator<<(std::string&) = 0;
				
				virtual interface& operator>>(int&) = 0;
				virtual interface& operator>>(double&) = 0;
				virtual interface& operator>>(std::string&) = 0;
				virtual interface& operator>>(long&) = 0;

				friend std::ostream& operator<<(std::ostream&, interface&);
		};

		inline std::ostream& operator<< (std::ostream& o, interface& i) {
			i.printer(o);
			return o;
		}
	}
}

#endif