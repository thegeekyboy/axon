#ifndef AXON_DATABASE_H_
#define AXON_DATABASE_H_

#include <variant>

namespace axon {

	namespace database {

		typedef std::variant<std::vector<std::string>, std::vector<double>, std::vector<int>, unsigned char*, double, int> bind;

		class interface
		{
			protected:
				virtual std::ostream& printer(std::ostream &) = 0;

			public:

				virtual bool connect() = 0;
				virtual bool connect(std::string, std::string, std::string) = 0;

				virtual bool close() = 0;
				virtual bool flush() = 0;

				virtual bool ping() = 0;
				virtual void version() = 0;

				virtual bool execute(const std::string&) = 0;
				virtual bool execute(const std::string&, axon::database::bind*, ...) = 0;

				virtual bool query(std::string) = 0;

				virtual bool next() = 0;
				virtual void done() = 0;

				virtual std::string get(unsigned int) = 0;

				virtual interface& operator<<(int) = 0;
				virtual interface& operator<<(std::string&) = 0;
				
				virtual interface& operator>>(int&) = 0;
				virtual interface& operator>>(double&) = 0;
				virtual interface& operator>>(std::string&) = 0;
				virtual interface& operator>>(std::time_t&) = 0;

				friend std::ostream& operator<<(std::ostream&, interface&);
		};

		inline std::ostream& operator<< (std::ostream& o, interface& i){
			i.printer(o);
			return o;
		}
	}
}

#endif