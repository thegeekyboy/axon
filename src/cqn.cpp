#include <axon/cqn.h>
#include <axon/util.h>

namespace axon {

	namespace stream {

		void csubscription::attach(axon::stream::topic_t &topic)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_subscribing)
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "statement already attached");

			ub4 timeout = 0;
			bool rowids_needed = true;
			ub4 ns = OCI_SUBSCR_NAMESPACE_DBCHANGE;

			_topic = topic;
			_topic.sub = this;

			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid *) _uuid.c_str(), _uuid.size(), OCI_ATTR_SUBSCR_NAME, _error)).failed())					// set the subscription name
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &ns, sizeof(ub4), OCI_ATTR_SUBSCR_NAMESPACE, _error)).failed())						// set the namespace to DBCHANGE
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			// if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid*) fnptr, 0, OCI_ATTR_SUBSCR_CALLBACK, _error)).failed())	// set callback function
			// if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid*) &csubscription::notify, 0, OCI_ATTR_SUBSCR_CALLBACK, _error)).failed())	// set callback function
			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, reinterpret_cast<void*>(&csubscription::notify), 0, OCI_ATTR_SUBSCR_CALLBACK, _error)).failed())	// set callback function
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			// if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid*) this, sizeof(this), OCI_ATTR_SUBSCR_PAYLOAD, _error)).failed())							// attach payload
			// 	throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (void *) &_topic, sizeof(_topic), OCI_ATTR_SUBSCR_CTX, _error)).failed())						// set callback context
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &rowids_needed, sizeof(ub4), OCI_ATTR_CHNF_ROWIDS, _error)).failed())					// Allow extraction of rowid information
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			if ((_error = OCIAttrSet(_pointer, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &timeout, 0, OCI_ATTR_SUBSCR_TIMEOUT, _error)).failed())								// Set a timeout value of half an hour
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			if ((_error = OCISubscriptionRegister(_context->get(), &_pointer, 1, _error, OCI_DEFAULT)).failed())														// Create a new registration in the	DBCHANGE namespace
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());

			// statement attach
			axon::database::statement stmt(*_context);
			stmt.prepare(topic.topic);
			stmt.bind(_pointer);
			stmt.execute(axon::database::exec_type::select);

			_subscribing = true;
		}

		void csubscription::detach()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			if ((_error = OCISubscriptionUnRegister(_context->get(), _pointer, _error.get(), OCI_DEFAULT)).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, _error.what());
			_subscribing = false;
		}

		ub4 csubscription::notify([[maybe_unused]] dvoid *ctx, [[maybe_unused]] OCISubscription *subscrhp, [[maybe_unused]] dvoid *payload, [[maybe_unused]] ub4 *size, dvoid *descriptor, [[maybe_unused]] ub4 mode)
		{
			axon::timer t(__PRETTY_FUNCTION__);
			axon::database::environment env;
			axon::database::error err;

			ub4 event_type;
			OCIColl *changes = (OCIColl *) 0;
			sb4 count = 0;

			if ((err = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &event_type, NULL, OCI_ATTR_CHDES_NFYTYPE, err.get())).failed())
				throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

			if (event_type == OCI_EVENT_OBJCHANGE)
			{
				// Obtain the collection of table change descriptors
				if ((err = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &changes, NULL, OCI_ATTR_CHDES_TABLE_CHANGES, err.get())).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

				// Obtain the size of the collection(i.e number of tables modified)
				if(changes)
				{
					if ((err = OCICollSize(env.get(), err.get(), (CONST OCIColl *) changes, &count)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());
				}
				else
					count = 0;
			}
			else
			{
				DBGPRN("unhandled event type: %d", event_type);
				return OCI_CONTINUE;
			}

			DBGPRN("Received notification => %d, change count: %d", event_type, count);

			for (int i = 0; i < count; i++)
			{
				boolean exist;
				dvoid **tprt, **rptr;
				dvoid *elemind = (dvoid *) 0;
				char *table_name;
				ub4 table_op, row_op;
				sb4 num_rows;
				OCIColl *row_changes = (OCIColl	*) 0;
				char *rowid;
				ub4 rowid_size;

				if ((err = OCICollGetElem(env.get(), err.get(), (OCIColl *) changes, i, &exist, (void**)&tprt, &elemind)).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

				if ((err = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES, &table_name,  NULL, OCI_ATTR_CHDES_TABLE_NAME, err.get())).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

				if ((err = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES,  (dvoid *) &table_op, NULL, OCI_ATTR_CHDES_TABLE_OPFLAGS, err.get())).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

				if (table_op & axon::database::change::allrows) {

					if (table_op & axon::database::change::insert)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - insert"<<std::endl;
					else if (table_op & axon::database::change::update)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - update"<<std::endl;
					else if (table_op & axon::database::change::remove)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - remove"<<std::endl;
					else if (table_op & axon::database::change::alter)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - alter"<<std::endl;
					else if (table_op & axon::database::change::drop)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - drop"<<std::endl;
					else if (table_op & axon::database::change::unknown)
						std::cout<<"table: "<<table_name<<" = OCI_OPCODE_ALLROWS <> "<<table_op<<" - unknown"<<std::endl;

					continue;
				}

				if ((err = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES, &row_changes, NULL, OCI_ATTR_CHDES_TABLE_ROW_CHANGES, err.get())).failed())
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

				if(row_changes)
				{
					if ((err = OCICollSize(env.get(), err.get(), row_changes, &num_rows)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());
				}
				else
					num_rows = 0;

				for (int j = 0; j < num_rows; j++)
				{
					if ((err = OCICollGetElem(env.get(), err.get(), (OCIColl *) row_changes, j, &exist, (void**) &rptr, &elemind)).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

					if ((err = OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, (dvoid *) &rowid, &rowid_size, OCI_ATTR_CHDES_ROW_ROWID, err.get())).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());
					if ((err = OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, (dvoid *) &row_op, NULL, OCI_ATTR_CHDES_ROW_OPFLAGS, err.get())).failed())
						throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, err.what());

					topic_t *t = static_cast<topic_t*>(ctx);
					csubscription *sub = std::any_cast<csubscription*>(t->sub);

					sub->callback((axon::database::operation)event_type, (axon::database::change)table_op, table_name, rowid);
				}
			}

			return 0;
		}

		void csubscription::callback([[maybe_unused]]axon::database::operation op, [[maybe_unused]]axon::database::change ch, std::string table, std::string rowid)
		{
			axon::stream::message_t data = {false, op, ch, table, rowid};
			_pipe.push_back(std::move(data));
			// _pipe.emplace_front(blah);

			// std::string sql = "SELECT * FROM " + table + " WHERE ROWID = '" + rowid + "'";

			// std::shared_ptr<axon::database::statement> stmt = std::make_shared<axon::database::statement>(*_context);
			// stmt->prepare(sql);
			// stmt->execute(axon::database::exec_type::select);

			// std::unique_ptr<axon::database::resultset> rs = std::make_unique<axon::database::resultset>(&stmt);
			// _topic.callback(op, ch, table, rowid, std::move(rs));
			_topic.callback(op, ch, table, rowid, this);
		}

		cqn::cqn(axon::database::oracle *ora, std::string consumer): cqn(ora)
		{
			_consumer = consumer;
		}

		cqn::cqn(axon::database::oracle *ora): _oracle(ora)
		{
			_uuid = util::uuid();
			_port = 7776;
			_connected = false;
			_subscribing = false;
		}

		cqn::~cqn()
		{
			disconnect();
		}

		void cqn::connect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			_connected = true;
		}

		void cqn::disconnect()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_connected)
			{
				while(!_list.empty()) _list.pop_back();
				_connected = false;
			}
		}

		// std::string cqn::subscribe(std::string topic, std::function<void(axon::stream::recordset*)> interceptor)
		std::string cqn::subscribe(std::string topic, cbfn interceptor)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			std::string id = util::uuid();
			_topics.push_back({id, topic, this, interceptor, (OCISubscription *) 0});

			return id;
		}

		std::string cqn::subscribe(std::string topic)
		{
			// return subscribe(topic, &test);
			return subscribe(topic, [](axon::database::operation, axon::database::change, std::string, std::string rowid, void *) {
				std::cout<<rowid<<std::endl;
			});
		}

		void cqn::start()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_subscribing)
				return;

			for (auto &elm : _topics)
			{
				std::unique_ptr<axon::stream::csubscription> temp = std::make_unique<axon::stream::csubscription>(_oracle->get_context());

				temp->attach(elm);
				_list.emplace_back(std::move(temp));
			}

			_subscribing = true;
		}

		void cqn::stop()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
		}

		std::tuple<std::string, std::unique_ptr<axon::database::resultset>> cqn::next()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			for (size_t i = 0; i < _list.size(); i++)
			{
				message_t data = _list[i]->pop();

				if (!data.empty) {
					DBGPRN("rowid: %s, table: %s", data.rowid.c_str(), data.table.c_str());

					std::string sql = "SELECT * FROM " + data.table + " WHERE ROWID = '" + data.rowid + "'";

					std::shared_ptr<axon::database::statement> stmt = std::make_shared<axon::database::statement>(*_oracle->get_context());
					stmt->prepare(sql);
					stmt->execute(axon::database::exec_type::select);
					return std::make_tuple(data.table, std::make_unique<axon::database::resultset>(stmt));
				}

			}
			return std::make_tuple("", nullptr);
		}
	}
}
