#ifndef AXON_DATABASE_H_
#define AXON_DATABASE_H_

#include <variant>

namespace axon {

    namespace database {

        typedef std::variant<std::vector<std::string>, std::vector<double>, std::vector<int>, unsigned char*, double, int> bind;

        class interface
        {
            public:

                virtual bool connect() = 0;
				virtual bool connect(std::string, std::string, std::string) = 0;

				virtual bool close() = 0;
                virtual bool flush() = 0;

                virtual bool ping() = 0;
				virtual void version() = 0;

                virtual bool execute(const std::string&) = 0;
			    virtual bool execute(const std::string&, axon::database::bind*, ...) = 0;
        };
    }
}

#endif