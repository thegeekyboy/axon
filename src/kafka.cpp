#include <string>
#include <thread>

#include <unistd.h>
#include <limits.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

#include <librdkafka/rdkafka.h>

#include <axon.h>
#include <axon/kafka.h>
#include <axon/util.h>

namespace axon
{
	namespace stream
	{
		namespace rdk
		{
			subscription::subscription(int count): _subscription(nullptr)
			{
				_subscription = rd_kafka_topic_partition_list_new(count);
			}

			subscription::~subscription()
			{
				if (_subscription)
					rd_kafka_topic_partition_list_destroy(_subscription);
			}

			void subscription::add(std::string topic)
			{
				if (rd_kafka_topic_partition_list_add(_subscription, topic.c_str(), RD_KAFKA_PARTITION_UA) == NULL) {
					WRNPRN("null returned!");
				}
			}

			int subscription::count()
			{
				return _subscription ? _subscription->cnt : 0;
			}

			rd_kafka_topic_partition_list_t *subscription::get()
			{
				if (_subscription == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "subscription not ready");
				return _subscription;
			}

			void subscription::info()
			{
				for (int i = 0; i < _subscription->cnt; i++)
				{
					rd_kafka_topic_partition_t *p = &_subscription->elems[i];

					DBGPRN("Topic \"%s\" partition %" PRId32, p->topic, p->partition);

					if (p->err)
					{
						INFPRN("error %s", rd_kafka_err2str(p->err));
					}
					else {
						INFPRN("offset %" PRId64 "", p->offset);

						if (p->metadata_size) {
							INFPRN(" (%d bytes of metadata)", (int)p->metadata_size);
						}
					}
				}
			}
		}

		void kafka::_avro_to_recordset(avro_value_t &val, const std::string &topic_name, axon::resultset &rs)
		{
			size_t field_count = 0;
			avro_value_get_size(&val, &field_count);

			for (size_t i = 0; i < field_count; i++)
			{
				avro_value_t field_val;
				const char  *field_name = nullptr;

				avro_value_get_by_index(&val, i, &field_val, &field_name);

				avro_type_t atype = avro_value_get_type(&field_val);
				axon::column_type ct = axon::column_type::string_t;

				if (atype == AVRO_UNION)
				{
					avro_schema_t union_schema = avro_value_get_schema(&field_val);
					atype = AVRO_NULL;

					size_t branch_count = avro_schema_union_size(union_schema);
					for (size_t bi = 0; bi < branch_count; bi++)
					{
						avro_schema_t branch_schema = avro_schema_union_branch(union_schema, bi);
						if (avro_typeof(branch_schema) != AVRO_NULL)
						{
							atype = avro_typeof(branch_schema);
							break;
						}
					}
				}

				switch (atype)
				{
					case AVRO_INT32:   ct = axon::column_type::int64_t;  break;
					case AVRO_INT64:   ct = axon::column_type::int64_t;  break;
					case AVRO_FLOAT:   ct = axon::column_type::double_t; break;
					case AVRO_DOUBLE:  ct = axon::column_type::double_t; break;
					case AVRO_BOOLEAN: ct = axon::column_type::bool_t;   break;
					case AVRO_BYTES:
					case AVRO_FIXED:   ct = axon::column_type::bytes_t;  break;
					default:           ct = axon::column_type::string_t; break;
				}

				rs.add_column(field_name ? field_name : "", ct);
			}

			// Push row data
			rs.set_source(topic_name);
			rs.begin_row();

			for (size_t i = 0; i < field_count; i++)
			{
				avro_value_t field_val;
				avro_value_get_by_index(&val, i, &field_val, nullptr);

				avro_type_t atype = avro_value_get_type(&field_val);

				// Unwrap union — check for null branch
				if (atype == AVRO_UNION)
				{
					int disc = 0;
					avro_value_get_discriminant(&field_val, &disc);

					avro_value_t branch;
					avro_value_get_current_branch(&field_val, &branch);

					// disc == 0 typically means null branch in nullable union
					if (avro_value_get_type(&branch) == AVRO_NULL)
					{
						rs.push_null();
						continue;
					}

					field_val = branch;
					atype = avro_value_get_type(&field_val);
				}

				switch (atype)
				{
					case AVRO_INT32:
					{
						int32_t v = 0;
						avro_value_get_int(&field_val, &v);
						rs.push_int(static_cast<int64_t>(v));
						break;
					}
					case AVRO_INT64:
					{
						int64_t v = 0;
						avro_value_get_long(&field_val, &v);
						rs.push_int(v);
						break;
					}
					case AVRO_FLOAT:
					{
						float v = 0.0f;
						avro_value_get_float(&field_val, &v);
						rs.push_double(static_cast<double>(v));
						break;
					}
					case AVRO_DOUBLE:
					{
						double v = 0.0;
						avro_value_get_double(&field_val, &v);
						rs.push_double(v);
						break;
					}
					case AVRO_BOOLEAN:
					{
						int v = 0;
						avro_value_get_boolean(&field_val, &v);
						rs.push_bool(v != 0);
						break;
					}
					case AVRO_BYTES:
					{
						const void *buf = nullptr;
						size_t	  sz  = 0;
						avro_value_get_bytes(&field_val, &buf, &sz);
						rs.push_bytes(buf, sz);
						break;
					}
					case AVRO_FIXED:
					{
						const void *buf = nullptr;
						size_t	  sz  = 0;
						avro_value_get_fixed(&field_val, &buf, &sz);
						rs.push_bytes(buf, sz);
						break;
					}
					case AVRO_STRING:
					{
						const char *buf = nullptr;
						size_t	  sz  = 0;
						avro_value_get_string(&field_val, &buf, &sz);
						rs.push_string(buf && sz > 0 ? std::string(buf, sz - 1) : "");
						// Avro strings include null terminator in sz — subtract 1
						break;
					}
					default:
						rs.push_null();
						break;
				}
			}

			rs.end_row();
		}

		std::unique_ptr<axon::resultset> kafka::_deserialise(axon::stream::rdk::message &msg)
		{
			if (msg.empty()) return nullptr;

			char error[1024];
			avro_value_t avro_val;

			if (serdes_deserialize_avro(_serdes.get(), &avro_val, nullptr, msg.payload(), msg.size(), error, sizeof(error)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, error);

			auto rs = std::make_unique<axon::resultset>(msg.name());

			try
			{
				_avro_to_recordset(avro_val, msg.name(), *rs);
			}
			catch (...)
			{
				avro_value_decref(&avro_val);
				throw;
			}

			avro_value_decref(&avro_val);
			return rs;
		}

		void kafka::_rebalance(rd_kafka_t *rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t *partitions, [[maybe_unused]] void *opaque)
		{
			rd_kafka_error_t *error = nullptr;
			rd_kafka_resp_err_t ret_err = RD_KAFKA_RESP_ERR_NO_ERROR;

#if DEBUG >= 2
			axon::stream::kafka *kinst = static_cast<axon::stream::kafka*>(opaque);
			INFPRN("rebalance: subscribed to %zu topic(s)", kinst->count());
#endif

			switch (err)
			{
				case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
					for (int i = 0; i < partitions->cnt; i++)
						if (partitions->elems[i].partition == 0)
							INFPRN("%s rebalanced", partitions->elems[i].topic);

					if (!strcmp(rd_kafka_rebalance_protocol(rk), "COOPERATIVE"))
						error = rd_kafka_incremental_assign(rk, partitions);
					else
						ret_err = rd_kafka_assign(rk, partitions);
					break;

				case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
					if (!strcmp(rd_kafka_rebalance_protocol(rk), "COOPERATIVE"))
						error = rd_kafka_incremental_unassign(rk, partitions);
					else
						ret_err = rd_kafka_assign(rk, nullptr);
					break;

				default:
					ERRPRN("rebalance error: %s", rd_kafka_err2str(err));
					rd_kafka_assign(rk, nullptr);
					break;
			}

			if (error) {
				ERRPRN("incremental assign failure: %s", rd_kafka_error_string(error));
				rd_kafka_error_destroy(error);
			} else if (ret_err) {
				ERRPRN("assign failure: %s", rd_kafka_err2str(ret_err));
			}
		}

		void kafka::_stop()
		{
			_runnable = false;
		}

		kafka::kafka(std::string bootstrap, std::string schema_reg, std::string consumer, axon::stream::rdk::config config)
		:axon::stream::connector(bootstrap, consumer, ""), _bootstrap_hosts(std::move(bootstrap)), _schema_hosts(std::move(schema_reg)), _consumer(std::move(consumer))
		{
			char hostname[128];
			if (gethostname(hostname, sizeof(hostname)))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to lookup hostname");

			std::string client_id = std::string(hostname) + "-" + std::to_string(getuid()) + "-" + std::to_string(getpid()) + "-" + axon::util::uuid();

			if (config.is_new())
			{
				config.set("enable.partition.eof", "false");
				config.set("enable.auto.commit", "true");
				config.set("auto.offset.reset", "latest");
				config.set("bootstrap.servers", _bootstrap_hosts);
				config.set("session.timeout.ms",    "10000");  // 10s — how long before broker considers consumer dead
				config.set("max.poll.interval.ms",  "10000");  // 10s — max time between polls before broker kicks consumer
				config.set("heartbeat.interval.ms", "3000");   // 3s — heartbeat frequency
		#if DEBUG >= 3
				config.set("debug", "topic,metadata,feature");
		#endif
				rd_kafka_conf_set_rebalance_cb(config, _rebalance);
				rd_kafka_conf_set_opaque(config, this);
			}

			config.set("client.id", client_id);
			config.set("group.id", _consumer);

			_serdes.set("schema.registry.url", _schema_hosts);
			_serdes.init();

			_rdk = std::make_shared<axon::stream::rdk::rdkafka>(config, RD_KAFKA_CONSUMER);
			rd_kafka_poll_set_consumer(_rdk->get());

			config.reset();
			_counter = 0;
		}

		kafka::~kafka()
		{
			if (_runnable)
				stop();

			if (_rdk && _rdk->valid())
			{
				DBGPRN("[kafka:%s] closing consumer", _id.c_str());

				// Do a final explicit commit before leaving the group
				rd_kafka_commit(_rdk->get(), nullptr, 0 /* sync */);

				rd_kafka_resp_err_t err = rd_kafka_consumer_close(_rdk->get());
				if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
					ERRPRN("[kafka:%s] consumer_close: %s", _id.c_str(), rd_kafka_err2str(err));

				// Drain — sends the LeaveGroupRequest to the broker
				rd_kafka_message_t *msg = nullptr;
				while ((msg = rd_kafka_consumer_poll(_rdk->get(), 100)) != nullptr)
					rd_kafka_message_destroy(msg);

				DBGPRN("[kafka:%s] consumer closed", _id.c_str());
			}
		}

		void kafka::subscribe()
		{
			if (!_rdk->valid())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kafka not initialized");

			if (_topic.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no topics added — call add() before subscribe()");

			axon::stream::rdk::subscription sb((int) _topic.size());

			for (const auto &t : _topic)
				sb.add(t.name);

			rd_kafka_resp_err_t err;
			if ((err = rd_kafka_subscribe(_rdk->get(), sb)) != RD_KAFKA_RESP_ERR_NO_ERROR)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "rd_kafka_subscribe: %s => %s", rd_kafka_err2str(err), _topic[0].name.c_str());

			_subscribed = true;
			DBGPRN("[kafka:%s] subscribed to %zu topics", _id.c_str(), _topic.size());
		}

		void kafka::unsubscribe()
		{
			if (!_rdk->valid() || !_subscribed) return;

			if (!_autocommit)
        		rd_kafka_commit(_rdk->get(), nullptr, 1 /* async */);

			_subscribed = false;
		}

		bool kafka::start()
		{
			if (_runnable)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already running");

			if (!_subscribed)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not subscribed — call subscribe() before start()");

			_runnable = true;
			_running = true;

			_runner = std::thread([this](){

				axon::util::set_thread_name(pthread_self(), "axon/kafka");

				while (_runnable)
				{
					try
					{
						axon::stream::rdk::message msg(*_rdk);
						auto rs = _deserialise(msg);

						if (!rs) continue;

						// Find matching topic callback or use global callback
						axon::stream::cbfn cb = _callback;

						if (!cb)
						{
							for (const auto &t : _topic)
							{
								if (t.name == rs->source() || t.target == rs->source())
								{
									cb = t.callback;
									break;
								}
							}
						}

						if (cb)
						{
							cb(std::move(rs));
							_counter++;
						}

						// Manual commit every MIN_COMMIT_COUNT messages
						if (!_autocommit && _counter > 0 && (_counter % axon::stream::MIN_COMMIT_COUNT) == 0)
						{
							rd_kafka_resp_err_t err;
							if ((err = rd_kafka_commit(_rdk->get(), nullptr, 0)) != RD_KAFKA_RESP_ERR_NO_ERROR)
								ERRPRN("[kafka:%s] commit error: %s", _id.c_str(), rd_kafka_err2str(err));
						}
					} catch (axon::exception &e) {
						ERRPRN("[kafka:%s] %s", _id.c_str(), e.what());
					} catch (const std::bad_function_call &e) {
						DBGPRN("[kafka:%s] bad_function_call: %s", _id.c_str(), e.what());
					}
				}

				_running = false;
				DBGPRN("[kafka:%s] runner exited", _id.c_str());
			});

			return true;
		}

		bool kafka::start(axon::stream::cbfn cb)
		{
			_callback = std::move(cb);
			return start();
		}

		void kafka::del(const std::string &bootstrap, const std::string &group, int timeout)
		{
			del(bootstrap, std::vector<std::string>{group}, timeout);
		}

		void kafka::del(const std::string &bootstrap, const std::vector<std::string> &groups, int timeout)
		{
			if (groups.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no groups specified");

			char error[512];

			axon::stream::rdk::config config;
			config.set("bootstrap.servers", bootstrap);

			axon::stream::rdk::rdkafka admin(config);   // PRODUCER — correct for admin API

			std::vector<rd_kafka_DeleteGroup_t*> del_groups;
			del_groups.reserve(groups.size());
			for (const auto &g : groups)
				del_groups.push_back(rd_kafka_DeleteGroup_new(g.c_str()));

			axon::stream::rdk::options opts(admin, RD_KAFKA_ADMIN_OP_DELETEGROUPS);
			rd_kafka_AdminOptions_set_operation_timeout(opts, timeout, error, sizeof(error));

			axon::stream::rdk::queue queue(admin);

			rd_kafka_DeleteGroups(admin.get(), del_groups.data(), (int) del_groups.size(), opts, queue);

			for (auto *dg : del_groups)
				rd_kafka_DeleteGroup_destroy(dg);

			// Poll for result
			axon::stream::rdk::event event;
			for (int elapsed = 0; elapsed < timeout && !event.valid(); elapsed += 100)
				event = rd_kafka_queue_poll(queue, 100);

			if (!event.valid())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "DeleteGroups timed out after %d ms", timeout);

			// Validate event type
			if (event.type() != RD_KAFKA_EVENT_DELETEGROUPS_RESULT)
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "unexpected event type from DeleteGroups");
			}

			// Read results
			const rd_kafka_DeleteGroups_result_t *result = rd_kafka_event_DeleteGroups_result(event);

			size_t result_count = 0;
			const rd_kafka_group_result_t **results = rd_kafka_DeleteGroups_result_groups(result, &result_count);

			std::string errors_out;
			for (size_t i = 0; i < result_count; i++)
			{
				const rd_kafka_error_t *err = rd_kafka_group_result_error(results[i]);
				if (err)
				{
					errors_out += rd_kafka_group_result_name(results[i]);
					errors_out += ": ";
					errors_out += rd_kafka_error_string(err);
					errors_out += "; ";
				}
				else
					DBGPRN("[kafka] deleted consumer group: %s",
						rd_kafka_group_result_name(results[i]));
			}

			// admin, queue, opts destroyed here by RAII — safe because event is already freed

			if (!errors_out.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "DeleteGroups errors: %s", errors_out.c_str());
		}
	}
}

