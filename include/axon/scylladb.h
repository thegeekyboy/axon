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
		struct SchemaMeta {
			SchemaMeta(CassSession *session) {
				axon::timer ctm(__PRETTY_FUNCTION__);
				_schema = cass_session_get_schema_meta(session);
			}
			~SchemaMeta() {
				if (_schema) cass_schema_meta_free(_schema);
			}
			const CassSchemaMeta *get() { return _schema; }
			private:
				const CassSchemaMeta* _schema;
		};

		struct tableinfo {

			tableinfo() = delete;
			tableinfo(CassSession *session, std::string keyspace, std::string table):
			sm(session)
			{
				if ((_keyspace = cass_schema_meta_keyspace_by_name(sm.get(), keyspace.c_str())) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot get keyspace meta");

				if ((_table = cass_keyspace_meta_table_by_name(_keyspace, table.c_str())) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot get table meta");
			}
			~tableinfo() {
				
			}

			size_t colcnt() { return cass_table_meta_column_count(_table); }
			size_t idxcnt() { return cass_table_meta_index_count(_table); }
			bool column_exists(std::string name) { 
				return (cass_table_meta_column_by_name(_table, name.c_str()) == NULL)?false:true;
			}
			void columns() {

				CassIterator *xd;
				
				xd = cass_iterator_fields_from_table_meta(_table);

				while (cass_iterator_next(xd))
				{
					const char *name;
					size_t sz;

					cass_iterator_get_meta_field_name(xd, &name, &sz);
					std::cout<<name<<std::endl;
				}

				cass_iterator_free(xd);
			}

			private:
				SchemaMeta sm;
				const CassKeyspaceMeta* _keyspace;
				const CassTableMeta *_table;
		};

		class scylladb: public interface {

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
					axon::timer ctm(__PRETTY_FUNCTION__);
					if (_result == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "result not extracted");
					
					if (!cass_iterator_next(_iterator))
						return false;
					return true;
				}

				const CassRow *get() {
					axon::timer ctm(__PRETTY_FUNCTION__);
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
					axon::timer ctm(__PRETTY_FUNCTION__);
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
					axon::timer ctm(__PRETTY_FUNCTION__);
					return std::make_unique<resultset>(_cf);
				}

				CassFuture *get() {
					if (_cf == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "future not ready");
					return _cf;
				}
				
				private:
					CassFuture *_cf;
					CassError _errcode;
					std::string _errmsg;
			};

			struct prepared {

				prepared() = delete;
				prepared(future &cf):
				p(NULL)
				{
					p = cass_future_get_prepared(cf.get());
					if (p == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create parpared");
				};

				~prepared() { 
					if (p) cass_prepared_free(p);
				}

				const CassPrepared *get() {
					if (p == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "parpared not ready");
					return p;
				}

				operator const CassPrepared*()
				{
					return get();
				}

				private:
					const CassPrepared *p;
			};
			
			struct statement {

				statement(): _statement(NULL) { };
				~statement() {
					if (_statement != NULL) cass_statement_free(_statement);
				};

				CassStatement *get() {
					if (_statement == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "statement not ready");
					return _statement;
				}

				operator CassStatement*() { return get(); }

				void prepare(CassSession *session, std::string sql) {

					future fq = cass_session_prepare(session, sql.c_str());
					fq.wait();
					if (!fq)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq.what());

					if (_statement != NULL) cass_statement_free(_statement);
					_statement = cass_prepared_bind(prepared(fq));				
					_session = session;
				};

				std::unique_ptr<future> execute() {
					if (_statement == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No prepared statement to execute!");
					return std::make_unique<future>(cass_session_execute(_session, _statement));
				}

				private:
					CassStatement *_statement;
					CassSession *_session;
			};

			bool _connected, _query, _prepared, _running;

			int _type, _rowidx, _colidx;
			uint8_t _port;
			std::string _keyspace, _hostname, _username, _password;
			
			CassCluster* _cluster = NULL;
			CassSession* _session = NULL;

			statement _statement;

			std::unique_ptr<resultset> _records;

			std::mutex _safety;

			int replace(std::string&);
			int prepare(int, va_list *, axon::database::bind*);
			int bind(statement&);

		protected:
			std::ostream& printer(std::ostream&) override;

		public:
			int _get_int(int) override;
			long _get_long(int) override;
			float _get_float(int) override;
			double _get_double(int) override;
			std::string _get_string(int) override;

			scylladb();
			scylladb(const scylladb&);

			~scylladb();

			bool connect() override;
			bool connect(std::string, std::string, std::string) override;

			bool close() override;
			bool flush() override;

			bool ping() override;
			std::string version() override;
			
			bool transaction(trans_t) override;

			bool execute(std::string) override;

			bool query(std::string) override;
			bool next() override;
			void done() override;

			std::string& operator[] (char);
			int& operator[] (int);

			scylladb& operator<<(int) override;
			scylladb& operator<<(long) override;
			scylladb& operator<<(long long) override;
			scylladb& operator<<(float) override;
			scylladb& operator<<(double) override;
			scylladb& operator<<(std::string&) override;
			scylladb& operator<<(axon::database::bind&) override;

			scylladb& operator>>(int&) override;
			scylladb& operator>>(long&) override;
			scylladb& operator>>(float&) override;
			scylladb& operator>>(double&) override;
			scylladb& operator>>(std::string&) override;

			std::shared_ptr<tableinfo> getinfo(std::string name) { return std::make_shared<tableinfo>(_session, _keyspace, name); }
		};
	}
}

#endif