#ifndef AXON_SCYLLADB_H_
#define AXON_SCYLLADB_H_

#include <memory>
#include <mutex>
#include <vector>

#include <string.h>

#include <cassandra.h>
#include <dse.h>

#include <axon/database.h>

namespace axon
{
	namespace database
	{
		struct resultset {

				resultset() = delete;
				resultset(const resultset&) = delete;
				resultset(CassFuture *cf) {
					_result = cass_future_get_result(cf);
					_iterator = cass_iterator_from_result(_result);
					_row_count = cass_result_row_count(_result);
					_col_count = cass_result_column_count(_result);
				}

				~resultset() {
					if (_iterator != NULL) cass_iterator_free(_iterator);
					if (_result != NULL) cass_result_free(_result);
				}

				bool next() {
					axon::timer(__PRETTY_FUNCTION__);
					if (_result == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "result not extracted");
					
					if (!cass_iterator_next(_iterator))
						return false;
					return true;
				}

				const CassRow *get() {
					axon::timer(__PRETTY_FUNCTION__);
					if (_iterator == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "iterator empty");
					_row = cass_iterator_get_row(_iterator);
					return _row;
				}

				int rowcount() { return _row_count; }
				int colcount() { return _col_count; }

			private:
				const CassResult *_result;
				CassIterator *_iterator;
				const CassRow *_row;
				int _row_count, _col_count;
		};

		struct future {

			future(): _cf(NULL) { };
			future(CassFuture *cf): _cf(cf) { };
			~future() {
				if (_cf != NULL) cass_future_free(_cf);
			}

			void wait() {
				axon::timer(__PRETTY_FUNCTION__);
				if (_cf == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "future not set");
				cass_future_wait(_cf);
				_errcode = cass_future_error_code(_cf);

				const char *msg;
				size_t mlen;
				cass_future_error_message(_cf, &msg, &mlen);
				if (mlen)
					_errmsg = msg;
			}

			const char *what() {
				return _errmsg.c_str();
			}

			const CassPrepared *prepared() {
				return cass_future_get_prepared(_cf);
			}
			
			future& operator=(CassFuture *cf) {
				if (_cf != NULL)
					cass_future_free(_cf);
				_cf = cf;

				return *this;
			}
			
			bool operator!() {
				if (_errcode != CASS_OK)
					return true;
				return false;
			}

			std::unique_ptr<resultset> make_recordset() {
				axon::timer(__PRETTY_FUNCTION__);
				return std::make_unique<resultset>(_cf);
			}
			
			private:
				CassFuture *_cf;
				CassError _errcode;
				std::string _errmsg;
		};

		struct statement {

			statement(): _statement(NULL) { };
			~statement() {
				if (_statement != NULL) cass_statement_free(_statement);
			};

			CassStatement *get() { return _statement; }

			void prepare(CassSession *session, std::string sql) {

				future fq = cass_session_prepare(session, sql.c_str());
				fq.wait();
				if (!fq)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq.what());

				const CassPrepared *prepared = fq.prepared();
				_statement = cass_prepared_bind(prepared);
				_session = session;

				cass_prepared_free(prepared);
			};

			std::unique_ptr<future> execute() {
				return std::make_unique<future>(cass_session_execute(_session, _statement));
			}

			private:
				CassStatement *_statement;
				CassSession *_session;
		};

		class scylladb: public interface {

			bool _connected, _query, _prepared, _running;

			int _type, _rowidx, _colidx;
			uint8_t _port;
			std::string _keyspace, _hostname, _username, _password;
			
			CassCluster* _cluster = NULL;
			CassSession* _session = NULL;

			statement _statement;

			std::vector<axon::database::bind> _bind;
			std::unique_ptr<resultset> _records;

			std::mutex _safety;

			int replace(std::string&);
			int prepare(int, va_list *, axon::database::bind*);
			int bind(statement&);

			int _get_int(int);
			long _get_long(int);
			float _get_float(int);
			double _get_double(int);
			std::string _get_string(int);

		protected:
			std::ostream& printer(std::ostream&);

		public:
			scylladb();
			scylladb(const scylladb&);

			~scylladb();

			bool connect();
			bool connect(std::string, std::string, std::string);
			bool close();
			bool flush();

			bool ping();
			std::string version();
			
			bool transaction(trans_t);

			bool execute(std::string);
			bool execute(std::string, axon::database::bind&, ...);

			bool query(std::string);
			bool query(std::string, axon::database::bind&, ...);

			bool next();
			void done();

			std::string get(unsigned int); // <-- remove this
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

			std::string& operator[] (char i);
			int& operator[] (int i);

			scylladb& operator<<(int);
			scylladb& operator<<(std::string&);
			scylladb& operator<<(axon::database::bind&);

			scylladb& operator>>(int&);
			scylladb& operator>>(float&);
			scylladb& operator>>(double&);
			scylladb& operator>>(std::string&);
			scylladb& operator>>(long&);
		};
	}
}

#endif