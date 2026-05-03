#ifndef AXON_RECORDSET_H_
#define AXON_RECORDSET_H_

#include <boost/json.hpp>

namespace axon {

	class recordset final {

		bool _empty;
		std::string _id, _name, _sdata;

		boost::json::value _data;
		
		public:

			bool get_bool(std::string);
			int get_int(std::string);
			long get_long(std::string);
			float get_float(std::string);
			double get_double(std::string);
			std::string get_string(std::string);

			template <typename T>
			bool get(std::string name, T& p) {

				if constexpr(std::is_same<T, bool>::value)
					p = static_cast<T>(get_bool(name));
				else if constexpr(std::is_same<T, int>::value)
					p = static_cast<T>(get_int(name));
				else if constexpr(std::is_same<T, long>::value)
					p = static_cast<T>(get_long(name));
				else if constexpr(std::is_same<T, long long>::value)
					p = static_cast<T>(get_long(name));
				else if constexpr(std::is_same<T, float>::value)
					p = static_cast<T>(get_float(name));
				else if constexpr(std::is_same<T, double>::value)
					p = static_cast<T>(get_double(name));
				else if constexpr(std::is_same<T, std::string>::value)
					p = static_cast<T>(get_string(name));
				else return false;

				return true;
			}

			recordset() = delete;
			recordset(const recordset&) = delete;
			recordset& operator=(const recordset&) = delete;
			
			~recordset();
			
			recordset(const unsigned char*);
			recordset(std::string);

			size_t get(std::string, void *, size_t);
			bool is_empty() { return _empty; };
			std::string name() { return _name; };
			void name(std::string val) { _name = val; };
			
			void print();
			std::ostream& print(std::ostream&);
			
			friend std::ostream& operator<<(std::ostream& os, axon::recordset& rc) { return rc.print(os); };
	};
};

#endif
