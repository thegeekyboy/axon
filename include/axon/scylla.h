#ifndef AXON_SCYLLA_H_
#define AXON_SCYLLA_H_

#include <memory>
#include <mutex>
#include <vector>

#include <string.h>

#include <cassandra.h>
#include <dse.h>

#include <axon/database.h>

#define AXON_DATABASE_SCYLLA_CONN_PER_HOST 2
#define AXON_DATABASE_SCYLLA_TIMEOUT 10
#define AXON_DATABASE_SCYLLA_HEARTBEAT 30
#define AXON_DATABASE_SCYLLA_IDLE 45

namespace axon
{
	namespace database
	{
		namespace sci {

			struct metadata {
				metadata(CassSession *session) {
					BENCHMARK;
					_schema = cass_session_get_schema_meta(session);
					if (!_schema)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot get schema meta");
				}
				~metadata() {
					if (_schema) cass_schema_meta_free(_schema);
				}
				const CassSchemaMeta *get() { return _schema; }
				private:
					const CassSchemaMeta* _schema;
			};

			struct description {

				description() = delete;
				description(CassSession *session, std::string keyspace, std::string table): _meta(session)
				{
					BENCHMARK;
					if ((_keyspace = cass_schema_meta_keyspace_by_name(_meta.get(), keyspace.c_str())) == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot get keyspace meta");

					if ((_table = cass_keyspace_meta_table_by_name(_keyspace, table.c_str())) == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot get table meta");
				}
				~description() {
				}

				size_t size() { return cass_table_meta_column_count(_table); }
				size_t idxcnt() { return cass_table_meta_index_count(_table); }
				bool column_exists(std::string name) { 
					BENCHMARK;
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
						DBGPRN("%s", name);
					}

					cass_iterator_free(xd);
				}

				private:
					metadata _meta;
					const CassKeyspaceMeta* _keyspace;
					const CassTableMeta *_table;
			};

			struct session {

				session(): _session(NULL) {
					_session = cass_session_new();
				};
				session(const session&) = delete;
				~session() {
					if (_session) cass_session_free(_session);
				}
				CassSession *get() {
					if (!_session)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "session not ready");
					return _session; 
				}
				operator CassSession*() { return get(); }
				private:
					CassSession *_session;
			};

			struct cluster {

				cluster(): _cluster(NULL) {
					_cluster = cass_cluster_new();
				};

				cluster(const cluster&) = delete;

				~cluster() {
					if (_cluster) cass_cluster_free(_cluster);
				}

				CassCluster *get() {
					if(!_cluster)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cluster not ready");
					return _cluster;
				}

				private:
					CassCluster *_cluster;
			};

			class future {

				CassFuture *_future;
				CassError _errcode;
				std::string _errmsg;

				public:
					future(): _future(NULL) { };
					future(CassFuture *cf): _future(cf) { };
					~future() {
						if (_future != NULL) cass_future_free(_future);
					}

					future& wait() {
						BENCHMARK;
						if (_future == NULL)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "future not set");
						cass_future_wait(_future);
						_errcode = cass_future_error_code(_future);

						const char *msg;
						size_t mlen;
						cass_future_error_message(_future, &msg, &mlen);
						if (mlen) _errmsg = msg;

						return *this;
					}

					bool failed() { return (_errcode != CASS_OK) ? true:false;}

					const char *what() { return _errmsg.c_str(); }

					const CassPrepared *prepared() {
						return cass_future_get_prepared(_future);
					}

					future& operator=(CassFuture *cf) {
						if (_future != NULL)
							cass_future_free(_future);
						_future = cf;

						return *this;
					}

					bool operator!() {
						if (_errcode != CASS_OK)
							return true;
						return false;
					}

					CassFuture *get() {
						if (_future == NULL)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "future not ready");
						return _future;
					}
			};

			class connection {

				bool _connected { false };

				axon::database::sci::session _session;
				axon::database::sci::cluster _cluster;

				public:
					connection() { }

					connection(const connection&) = delete;

					~connection() { };

					CassSession *get() {
						return _session.get();
					}

					bool connected() {
						return _connected;
					}

					void connect(const std::string& hostname, const std::string& username, const std::string& password, const uint16_t port, const std::string& keyspace) {
						if (_connected)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already connected");

						if (hostname.empty() || username.empty() || password.empty() || keyspace.empty())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Connection credentials missing");

						if (port > 1024) cass_cluster_set_port(_cluster.get(), port);
						cass_cluster_set_contact_points(_cluster.get(), hostname.c_str());
						cass_cluster_set_core_connections_per_host(_cluster.get(), AXON_DATABASE_SCYLLA_CONN_PER_HOST);
						cass_cluster_set_connection_heartbeat_interval(_cluster.get(), AXON_DATABASE_SCYLLA_HEARTBEAT);
						cass_cluster_set_connection_idle_timeout(_cluster.get(), AXON_DATABASE_SCYLLA_IDLE);
						cass_cluster_set_dse_plaintext_authenticator(_cluster.get(), username.c_str(), password.c_str());

						future fq;

						if (!keyspace.empty())
							fq = cass_session_connect_keyspace(_session.get(), _cluster.get(), keyspace.c_str());
						else
							fq = cass_session_connect(_session.get(), _cluster.get());

						if (fq.wait().failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot connect to database, driver: %s", fq.what());

						_connected = true;
					}

					void connect(const std::string& hostname, const std::string& username, const std::string& password, const uint16_t port) {
						connect(hostname, username, password, port, "");
					}

					void disconnect()
					{
						if (!_connected) return;

						future fq = cass_session_close(_session.get());
						if (fq.wait().failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "there was an error closing the database connection, driver: %s", fq.what());
						_connected = false;
					}
			};

			struct prepared {

				prepared() = delete;
				prepared(future &cf): _pointer(NULL) {
					if ((_pointer = cass_future_get_prepared(cf.get())) == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create prepared statement");
				};

				~prepared() { 
					if (_pointer) cass_prepared_free(_pointer);
				}

				const CassPrepared *get() {
					if (_pointer == NULL)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "prepared statement not ready");
					return _pointer;
				}
				operator const CassPrepared*() { return get(); }

				private:
					const CassPrepared *_pointer;
			};

			class statement {

				CassStatement *_statement;
				std::shared_ptr<axon::database::sci::connection> _connection;

				public:

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

					void prepare(std::shared_ptr<axon::database::sci::connection> connection, std::string sql) {

						future fq = cass_session_prepare(connection->get(), sql.c_str());
						if (fq.wait().failed())
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, fq.what());

						if (_statement != NULL) cass_statement_free(_statement);
						_statement = cass_prepared_bind(axon::database::sci::prepared(fq));
						_connection = connection;
					};

					std::unique_ptr<axon::database::sci::future> execute() {
						if (_statement == NULL)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "No prepared statement to execute!");
						return std::make_unique<axon::database::sci::future>(cass_session_execute(_connection->get(), _statement));
					}
			};
		}

		class scylla: public connector {

			bool _query, _prepared;
			std::string _hostname, _username, _password, _keyspace;
			int _port { 9042 };

			std::shared_ptr<axon::database::sci::connection> _connection;
			std::shared_ptr<axon::database::sci::statement> _statement;

			const CassResult *_result { nullptr };
			CassIterator *_iterator { nullptr };

			std::mutex _safety;

			int replace(std::string &);
			int bind(std::shared_ptr<axon::database::sci::statement>&);

			public:

				scylla();
				~scylla();

				scylla(const axon::database::scylla&) = delete;

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

				std::shared_ptr<axon::database::sci::description> describe(std::string& keyspace, std::string table) {
					return std::make_shared<axon::database::sci::description>(_connection->get(), keyspace, table);
				}

				std::string& operator[] (char);
				int& operator[] (int);
		};
	}
}

#endif

