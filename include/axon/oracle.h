#ifndef AXON_ORACLE_H_
#define AXON_ORACLE_H_

#include <mutex>
#include <algorithm>
#include <cstring>
#include <cstdarg>

#include <axon/util.h>
#include <axon/database.h>
#include <axon/oci.h>

#define AXON_DATABASE_ORACLE_PREFETCH 500

namespace axon {

	namespace database {

		class oracle: public connector {

			std::shared_ptr<axon::database::oci::connection> _connection;
			std::shared_ptr<axon::database::oci::statement> _statement;
			axon::database::oci::error _error;

			bool _running, _executed;

			std::string _hostname, _username, _password;

			std::vector<axon::database::oci::column> _columns;

			inline const axon::database::oci::column& _col(size_t position) const {
				if (position >= _columns.size())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");
				return _columns[position];
			}

			void _get_column_info();
			void _get_column_details(uint16_t);
			axon::column_type _attach_column_data(axon::database::oci::column&, uint16_t);

			int _get_int(size_t, int);
			bool _get_bool(size_t, int);
			long _get_long(size_t, int);
			float _get_float(size_t, int);
			double _get_double(size_t, int);
			std::string _get_string(size_t, int);

			public:

				oracle();
				oracle(const axon::database::oracle&) = delete;
				~oracle();

				oracle(std::shared_ptr<axon::database::oci::connection>);

				bool connect() override;
				bool connect(std::string, std::string, std::string) override;
				bool close() override;
				bool flush() override;

				bool ping() override;
				std::string version() override;

				bool transaction(axon::database::trans_t) override;

				bool execute(const std::string) override;
				bool query(const std::string) override;
				void fetch(axon::resultset&, int) override;
				void done() override;

				std::string& operator[] (char);
				int& operator[] (int);
		};
	}
}

#endif
