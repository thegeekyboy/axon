#include <cstring>
#include <boost/regex.hpp>
#include <axon/ocn.h>

namespace axon {

	namespace stream2r {

		ocn::ocn(std::string hostname, std::string username, std::string password): axon::stream2r::connector(hostname, username, password), _connection(std::make_shared<axon::database2r::oci::connection>())
		{
			_port = -1;
			DBGPRN("[%s] constructed", _id.c_str());
		}

		ocn::ocn(std::shared_ptr<axon::database2r::oci::connection> connection)
		: axon::stream2r::connector("localhost", "localuser", "localpassword"), _connection(connection)
		// ↑ dummy credentials — this ctor accepts a pre-built connection.
  		// _hostname/_username/_password are unused; real credentials are in _connection.
		{
			DBGPRN("[%s] constructed", _id.c_str());
		}

		ocn::~ocn()
		{
			if (_running)
        		stop();

			if (_connection->connected())
				disconnect();
		}

		void ocn::connect()
		{
			if (_connection->connected())
			{
				DBGPRN("[%s] using pre-connected connection", _id.c_str());
				return;
			}

			_connection->connect(_hostname, _username, _password);
		}

		void ocn::disconnect()
		{
			if (!_connection->connected()) return;

			_connection->disconnect();
		}

		void ocn::_stop()
		{
			_runnable = false;
			_queue_cv.notify_all();
		}

		void ocn::_attach(ocn_sub &sub, const axon::stream2r::topic &t)
		{
			axon::database2r::oci::error err;

			if (OCIHandleAlloc(axon::database2r::oci::environment::get(), (dvoid**) &sub.handle, OCI_HTYPE_SUBSCRIPTION, 0, nullptr) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "cannot allocate OCISubscription handle");

			// 1. Namespace = DBCHANGE (mandatory for OCN/CQN)
			ub4 ns = OCI_SUBSCR_NAMESPACE_DBCHANGE;
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, &ns, sizeof(ub4), OCI_ATTR_SUBSCR_NAMESPACE, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// 2. Static C callback — OCI_ATTR_SUBSCR_CALLBACK expects dvoid*
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, reinterpret_cast<dvoid*>(&ocn::_notify), 0, OCI_ATTR_SUBSCR_CALLBACK, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// 3. Context = this — _notify() casts it back to ocn*
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, static_cast<dvoid*>(this), sizeof(void*), OCI_ATTR_SUBSCR_CTX, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// 4. Enable ROWID extraction
			boolean rowids = true;
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, (dvoid*) &rowids, sizeof(boolean), OCI_ATTR_CHNF_ROWIDS, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// 5. Unique name — required for durable subscriptions
			std::string subname = _id + "." + t.name;
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, (dvoid*) subname.c_str(), (ub4) subname.size(), OCI_ATTR_SUBSCR_NAME, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// 6. Timeout 0 = persist until explicit unregister
			ub4 timeout = 0;
			if ((err = OCIAttrSet(sub.handle, OCI_HTYPE_SUBSCRIPTION, &timeout, 0, OCI_ATTR_SUBSCR_TIMEOUT, err)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// Register with Oracle — creates server-side persistent registration
			if ((err = OCISubscriptionRegister(_connection->ctx(), &sub.handle, 1, err, OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// Associate the registration SELECT with the subscription.
			// Executes t.target so Oracle knows which tables to watch.
			// Result rows from this execute are discarded.
			OCIStmt *regstmt = nullptr;

			if ((err = OCIStmtPrepare2(_connection->ctx(), &regstmt, err.get(), (text*) t.target.c_str(), (ub4) t.target.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			// Bind subscription handle to statement — this is the link
			if ((err = OCIAttrSet(regstmt, OCI_HTYPE_STMT, sub.handle, 0, OCI_ATTR_CHNF_REGHANDLE, err)).failed())
			{
				OCIStmtRelease(regstmt, err.get(), nullptr, 0, OCI_DEFAULT);
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());
			}

			// Execute — registers the query, result is discarded
			if ((err = OCIStmtExecute(_connection->ctx(), regstmt, err.get(), 0, 0, nullptr, nullptr, OCI_DEFAULT)).failed())
			{
				OCIStmtRelease(regstmt, err.get(), nullptr, 0, OCI_DEFAULT);
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());
			}

			OCIStmtRelease(regstmt, err.get(), nullptr, 0, OCI_DEFAULT);

			sub.attached = true;
			DBGPRN("[ocn:%s] attached: %s => %s", _id.c_str(), t.name.c_str(), t.target.c_str());
		}

		void ocn::_detach(ocn_sub &sub)
		{
			if (!sub.attached || !sub.handle) return;

			axon::database2r::oci::error err;

			OCISubscriptionUnRegister(_connection->ctx(), sub.handle, err.get(), OCI_DEFAULT);
			OCIHandleFree(sub.handle, OCI_HTYPE_SUBSCRIPTION);

			sub.handle   = nullptr;
			sub.attached = false;

			DBGPRN("[ocn:%s] detached subscription", _id.c_str());
		}

		void ocn::subscribe()
		{
			if (_subscribed)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already subscribed");

			if (!_connection->connected())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected — call connect() before subscribe()");

			if (_topic.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no topics — call add() before subscribe()");

			for (size_t i = 0; i < _topic.size(); i++)
			{
				ocn_sub sub;
				sub.topic_index = i;
				_attach(sub, _topic[i]);
				_subs.emplace_back(std::move(sub));
			}

			_subscribed = true;
			DBGPRN("[ocn:%s] subscribed %zu topics", _id.c_str(), _topic.size());
		}

		void ocn::unsubscribe()
		{
			if (!_subscribed) return;

			for (auto &sub : _subs)
				_detach(sub);

			_subs.clear();
			_subscribed = false;
		}

		bool ocn::start()
		{
			if (_running)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already running");

			if (!_subscribed)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not subscribed — call subscribe() before start()");

			_runnable = true;
			_running = true;
			_daemon = std::thread(&ocn::_dispatch_loop, this);

			DBGPRN("[ocn:%s] started", _id.c_str());
			return true;
		}

		bool ocn::start(axon::stream2r::cbfn cb)
		{
			// Set global callback — overrides per-topic callbacks
			_callback = std::move(cb);
			return start();
		}

		ub4 ocn::_notify(dvoid *ctx, [[maybe_unused]] OCISubscription *subscrhp, [[maybe_unused]] dvoid *payload, [[maybe_unused]] ub4 *size, dvoid *descriptor, [[maybe_unused]] ub4 mode)
		{
			axon::stream2r::ocn *self = static_cast<axon::stream2r::ocn*>(ctx);

			if (!self || !self->_runnable) return OCI_CONTINUE;

			try
			{
				// Use a local error handle — never share with the daemon thread
				axon::database2r::oci::error err;

				ub4 event_type = 0;
				if ((err = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &event_type, nullptr, OCI_ATTR_CHDES_NFYTYPE, err)).failed())
					return OCI_CONTINUE;

				if (event_type != OCI_EVENT_OBJCHANGE)
				{
					DBGPRN("[ocn] event type %u ignored", event_type);
					return OCI_CONTINUE;
				}

				OCIColl *table_changes = nullptr;
				if ((err = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &table_changes, nullptr, OCI_ATTR_CHDES_TABLE_CHANGES, err)).failed() || !table_changes)
					return OCI_CONTINUE;

				sb4 table_count = 0;
				OCICollSize(axon::database2r::oci::environment::get(), err.get(), table_changes, &table_count);

				for (sb4 i = 0; i < table_count; i++)
				{
					boolean exist = false;
					dvoid **tptr = nullptr;
					dvoid *elemind = nullptr;

					if (OCICollGetElem(axon::database2r::oci::environment::get(), err.get(), table_changes, i, &exist, (void**) &tptr, &elemind) != OCI_SUCCESS || !exist)
						continue;

					char *table_name = nullptr;
					ub4 table_op = 0;

					OCIAttrGet(*tptr, OCI_DTYPE_TABLE_CHDES, &table_name, nullptr, OCI_ATTR_CHDES_TABLE_NAME, err.get());
					OCIAttrGet(*tptr, OCI_DTYPE_TABLE_CHDES, &table_op, nullptr, OCI_ATTR_CHDES_TABLE_OPFLAGS, err.get());

					// Match table to a subscription by topic index
					size_t topic_idx = 0;
					bool found = false;
					std::string tname = table_name ? table_name : "";

					for (const auto &sub : self->_subs)
					{
						const std::string &tgt = self->_topic[sub.topic_index].target;

						if (tgt.find(tname) != std::string::npos || tname.find(self->_topic[sub.topic_index].name) != std::string::npos)
						{
							topic_idx = sub.topic_index;
							found = true;
							break;
						}
					}

					if (!found && !self->_subs.empty())
						topic_idx = 0;

					// ALLROWS — bulk change, no individual ROWIDs
					if (table_op & OCI_OPCODE_ALLROWS)
					{
						std::lock_guard<std::mutex> lk(self->_queue_mtx);

						self->_queue.push_back({ topic_idx, tname, "", table_op });
						self->_queue_cv.notify_one();

						continue;
					}

					// Individual row changes
					OCIColl *row_changes = nullptr;
					OCIAttrGet(*tptr, OCI_DTYPE_TABLE_CHDES, &row_changes, nullptr, OCI_ATTR_CHDES_TABLE_ROW_CHANGES, err.get());

					if (!row_changes) continue;

					sb4 row_count = 0;
					OCICollSize(axon::database2r::oci::environment::get(), err.get(), row_changes, &row_count);

					for (sb4 j = 0; j < row_count; j++)
					{
						dvoid **rptr = nullptr;
						if (OCICollGetElem(axon::database2r::oci::environment::get(), err.get(), row_changes, j, &exist, (void**) &rptr, &elemind) != OCI_SUCCESS || !exist)
							continue;

						char *rowid = nullptr;
						ub4 rowid_sz = 0;
						ub4 row_op   = 0;

						OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, &rowid, &rowid_sz, OCI_ATTR_CHDES_ROW_ROWID, err.get());
						OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, &row_op, nullptr, OCI_ATTR_CHDES_ROW_OPFLAGS, err.get());

						std::string rid = (rowid && rowid_sz)?std::string(rowid, rowid_sz):"";

						DBGPRN("[ocn] notify: %s ROWID=%s op=%u", tname.c_str(), rid.c_str(), row_op);

						std::lock_guard<std::mutex> lk(self->_queue_mtx);
						self->_queue.push_back({ topic_idx, tname, rid, row_op });
						self->_queue_cv.notify_one();
					}
				}
			}
			catch (...)
			{
				ERRPRN("[ocn] exception suppressed in _notify()");
			}

			return 0;
		}

		void ocn::_dispatch_loop()
		{
			DBGPRN("[ocn:%s] dispatch loop started", _id.c_str());

			while (_runnable)
			{
				ocn_event ev;

				{
					std::unique_lock<std::mutex> lk(_queue_mtx);

					_queue_cv.wait(lk, [this] {
						return !_queue.empty() || !_runnable;
					});

					if (!_runnable && _queue.empty()) break;

					ev = std::move(_queue.front());
					_queue.pop_front();
				}

				try
				{
					_handle_event(ev);
					_counter++;
				} catch (axon::exception &e) {

					ERRPRN("[ocn:%s] dispatch error: %s", _id.c_str(), e.what());
				} catch (...) {

					ERRPRN("[ocn:%s] unknown dispatch error", _id.c_str());
				}
			}

			_running = false;
			DBGPRN("[ocn:%s] dispatch loop exited", _id.c_str());
		}

		void ocn::_handle_event(const ocn_event &ev)
		{
			if (ev.topic_index >= _topic.size()) return;

			// Use global callback if set, otherwise per-topic callback
			const axon::stream2r::cbfn &cb = _callback?_callback:_topic[ev.topic_index].callback;

			if (!cb) return;

			// Bulk change — no ROWID — signal caller that something changed
			if (ev.rowid.empty())
			{
				auto rs = std::make_unique<axon::recordset2r>(ev.table);

				rs->set_eof();
				cb(std::move(rs));

				return;
			}

			_fetch_row(ev.table, ev.rowid, cb);
		}

		void ocn::_fetch_row(const std::string &table, const std::string &rowid, const axon::stream2r::cbfn &cb)
		{
			static const boost::regex valid_rowid("^[A-Za-z0-9+/]{18}$");

			if (!boost::regex_match(rowid, valid_rowid))
			{
				ERRPRN("[%s] invalid ROWID: %s", _id.c_str(), rowid.c_str());
				return;
			}

			std::string sql = "SELECT * FROM " + table + " WHERE ROWID = '" + rowid + "'";

			axon::database2r::oci::error err;
			OCIStmt *stmt = nullptr;

			if ((err = OCIStmtPrepare2(_connection->ctx(), &stmt, err.get(), (text*) sql.c_str(), (ub4) sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)).failed())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, err.what());

			if ((err = OCIStmtExecute(_connection->ctx(), stmt, err.get(), 0, 0, nullptr, nullptr, OCI_DEFAULT)).failed())
			{
				OCIStmtRelease(stmt, err.get(), nullptr, 0, OCI_DEFAULT);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, err.what());
			}

			ub4 col_count = 0;
			OCIAttrGet(stmt, OCI_HTYPE_STMT, &col_count, nullptr, OCI_ATTR_PARAM_COUNT, err.get());
			DBGPRN("OCIAttrGet(OCI_ATTR_PARAM_COUNT) returned %d columns", col_count);

			auto rs = std::make_unique<axon::recordset2r>(table);

			struct col_buf {

				std::vector<char> data;
				sb2 ind { 0 };
				ub2 rlen { 0 };
				ub2 dtype { SQLT_STR };
				axon::column_type ctype { axon::column_type::string_t };
			};

			std::vector<col_buf> bufs(col_count);

			for (ub4 i = 0; i < col_count; i++)
			{
				OCIParam *param   = nullptr;
				text *col_name = nullptr;
				ub4 namelen = 0;
				ub4 col_type = 0;
				ub4 col_size = 0;

				OCIParamGet(stmt, OCI_HTYPE_STMT, err.get(), (dvoid**) &param, i + 1);
				OCIAttrGet(param, OCI_DTYPE_PARAM, (dvoid**) &col_name, &namelen, OCI_ATTR_NAME, err.get());
				OCIAttrGet(param, OCI_DTYPE_PARAM, &col_type, nullptr, OCI_ATTR_DATA_TYPE, err.get());
				OCIAttrGet(param, OCI_DTYPE_PARAM, &col_size, nullptr, OCI_ATTR_DATA_SIZE, err.get());
				OCIDescriptorFree(param, OCI_DTYPE_PARAM);

				std::string name(reinterpret_cast<char*>(col_name), namelen);

				auto &b = bufs[i];

				switch (col_type)
				{
					case SQLT_NUM:
					case SQLT_INT:
					case SQLT_UIN:
						b.ctype = axon::column_type::int64_t;
						b.dtype = SQLT_VNU;
						b.data.resize(sizeof(OCINumber) + 1, 0);
						break;

					case SQLT_FLT:
					case SQLT_BFLOAT:
					case SQLT_BDOUBLE:
					case 100:
					case 101:
						b.ctype = axon::column_type::double_t;
						b.dtype = SQLT_BDOUBLE;
						b.data.resize(sizeof(double), 0);
						break;

					case SQLT_BIN:
					case SQLT_LBI:
						b.ctype = axon::column_type::bytes_t;
						b.dtype = SQLT_BIN;
						b.data.resize((col_size * 4) + 1, 0);
						break;

					default:
						b.ctype = axon::column_type::string_t;
						b.dtype = SQLT_STR;
						b.data.resize((col_size * 4) + 1, 0);
						break;
				}

				rs->add_column(name, b.ctype);
				DBGPRN("column: %s, type: %d", name.c_str(), b.ctype);

				OCIDefine *def = nullptr;
				OCIDefineByPos(stmt, &def, err.get(), i + 1, b.data.data(), (sb4) b.data.size(), b.dtype, &b.ind, &b.rlen, nullptr, OCI_DEFAULT);
			}

			sword rc = OCIStmtFetch2(stmt, err.get(), 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
			err = rc;
			if (err.failed()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "OCIStmtFetch2: %s — sql: %s", err.what().c_str(), sql.c_str());

			if (rc == OCI_SUCCESS || rc == OCI_NO_DATA)
			{
				ub4 fetched = 0;
				OCIAttrGet(stmt, OCI_HTYPE_STMT, &fetched, nullptr, OCI_ATTR_ROWS_FETCHED, err.get());
				WRNPRN("fetched %d", fetched);

				if (fetched > 0)
				{
					rs->begin_row();

					for (ub4 i = 0; i < col_count; i++)
					{
						auto &b = bufs[i];

						if (b.ind < 0) { rs->push_null(); continue; }

						switch (b.ctype)
						{
							case axon::column_type::int64_t: {
								long val = 0;
								OCINumberToInt(err.get(), reinterpret_cast<OCINumber*>(b.data.data()), sizeof(long), OCI_NUMBER_SIGNED, &val);
								rs->push_int(static_cast<int64_t>(val));
								break;
							}

							case axon::column_type::double_t: {
								double val = 0.0;
								std::memcpy(&val, b.data.data(), sizeof(double));
								rs->push_double(val);
								break;
							}

							case axon::column_type::bytes_t:
								rs->push_bytes(b.data.data(), b.rlen);
								break;

							default:
								rs->push_string(std::string(b.data.data()));
								break;
						}
					}

					rs->end_row();
				}
			}

			OCIStmtRelease(stmt, err.get(), nullptr, 0, OCI_DEFAULT);

			cb(std::move(rs));
		}

	}
}