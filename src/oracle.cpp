#include <axon.h>
#include <axon/oracle.h>

namespace axon {

	namespace database {

		oracle::oracle()
		{
			_port = 7776;
			_prefetch = 100;

			_connected = false;
			_subscribing = false;
			_running = false;
			_executed = false;

			_index = 1;
			_colidx = 0;

			DBGPRN("axon::database - constructing");
		}

		oracle::~oracle()
		{
			if (_subscribing)
				unwatch();

			if (_connected)
				close();

			DBGPRN("axon::database - destroying");
		}

		oracle::oracle(const oracle &lhs)
		{
			_environment = lhs._environment;
			_error = lhs._error;
			_server = lhs._server;
			_context = lhs._context;
			_session = lhs._session;
		}

		OCIError *oracle::getError()
		{
			return _error;
		}

		OCIEnv *oracle::getEnvironment()
		{
			return _environment;
		}

		OCISvcCtx *oracle::getService()
		{
			return _context;
		}

		void oracle::checker(sword status)
		{
			this->checker(_error, status);
		}

		std::string oracle::checker(OCIError *errhand, sword status)
		{
			text errbuf[512];
			sb4 errcode = 0;

			std::stringstream s;

			if (errhand == NULL)
			{
				s<<"OCIError is NULL so- nothing to see here!";
			}
			else
			{
				switch(status)
				{
					case OCI_SUCCESS:
						s<<"Operation successful - no error detected";
						break;

					case OCI_SUCCESS_WITH_INFO:
						s<<"Error - OCI_SUCCESS_WITH_INFO";
						break;

					case OCI_NEED_DATA:
						s<<"Error - OCI_NEED_DATA";
						break;

					case OCI_NO_DATA:
						s<<"Error - OCI_NODATA";
						break;

					case OCI_ERROR:
						(void) OCIErrorGet((dvoid *) errhand, (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
						errbuf[strlen((char*)errbuf)-1] = 0;
						s<<"Error - ("<<errcode<<") "<<errbuf;
						break;

					case OCI_INVALID_HANDLE:
						s<<"Error - OCI_INVALID_HANDLE";
						break;

					case OCI_STILL_EXECUTING:
						s<<"Error - OCI_STILL_EXECUTE";
						break;

					case OCI_CONTINUE:
						s<<"Error - OCI_CONTINUE";
						break;

					default:
						break;
				}
			}

			return s.str();
		}

		bool oracle::connect()
		{
			timer t1(__PRETTY_FUNCTION__);

			if (_sid.size() <= 1 || _username.size() <= 1 || _password.size() <= 1)
				return false;

			if (!_connected)
			{
				sword rc;

				strcpy(_ctx.sid, _sid.c_str());
				strcpy(_ctx.username, _username.c_str());
				strcpy(_ctx.password, _password.c_str());

				_context = (OCISvcCtx *) 0;
				_error = (OCIError *) 0;
				_session = (OCISession *) 0;
				_statement = (OCIStmt *) 0;
				_environment = (OCIEnv *) 0;
				_server = (OCIServer *) 0;

				if ((rc = OCIEnvCreate((OCIEnv **) &_environment, OCI_EVENTS|OCI_OBJECT|OCI_THREADED, NULL, NULL, NULL, NULL, 0, NULL)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::oracle::constructor - OCIEnvCreate", axon::database::oracle::checker(_error, rc));
				
				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_error, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::constructor - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIAttrSet((void *) _environment, (ub4) OCI_HTYPE_ENV, (void *) &_port, (ub4) 0, (ub4) OCI_ATTR_SUBSCR_PORTNO, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::constructor - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_server, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::constructor - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_context, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::constructor - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIServerAttach(_server, _error, (text *) _sid.c_str(), (sb4) _sid.size(), (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIServerAttach", axon::database::oracle::checker(_error, rc));

				// is the following duplicate?
				// if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_context, (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
				// 	throw DBERR(rc, "axon::database::oracle::connect", "OCIHandleAlloc");

				/* set attribute server context in the service context */
				if ((rc = OCIAttrSet((dvoid *) _context, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _server, (ub4) 0, (ub4) OCI_ATTR_SERVER, (OCIError *) _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				/* allocate a user context handle */
				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_session, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIHanleAlloc", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrSet((dvoid *) _session, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _username.c_str()), (ub4) _username.size(), OCI_ATTR_USERNAME, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIAttrSet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrSet((dvoid *) _session, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) _password.c_str()), (ub4) _password.size(), OCI_ATTR_PASSWORD, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIAttrSet", axon::database::oracle::checker(_error, rc));
				
				if ((rc = OCISessionBegin(_context, _error, _session, OCI_CRED_RDBMS, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCISessionBegin", axon::database::oracle::checker(_error, rc));

				/* Allocate a statement handle */
				if ((rc = OCIAttrSet((dvoid *) _context, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _session, (ub4) 0, OCI_ATTR_SESSION, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::connect - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				// OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 52, (dvoid **) &tmp);

				_connected = true;
			}

			return _connected;
		}

		bool oracle::connect(std::string sid, std::string username, std::string password)
		{
			_sid = sid;
			_username = username;
			_password = password;

			return connect();
		}

		bool oracle::close()
		{
			timer t1(__PRETTY_FUNCTION__);

			if (_subscribing)
				unwatch();

			if (_connected)
			{
				checker(OCISessionEnd(_context, _error, _session, (ub4) 0));
				checker(OCIServerDetach(_server, _error, (ub4) OCI_DEFAULT));
			}

			OCIHandleFree((dvoid *) _statement, OCI_HTYPE_STMT);
			OCIHandleFree((dvoid *) _server, (ub4) OCI_HTYPE_SERVER);
			OCIHandleFree((dvoid *) _context, (ub4) OCI_HTYPE_SVCCTX);
			OCIHandleFree((dvoid *) _session, (ub4) OCI_HTYPE_SESSION);
			OCIHandleFree((dvoid *) _error, (ub4) OCI_HTYPE_ERROR);
			OCIHandleFree((dvoid *) _environment, (ub4) OCI_HTYPE_ENV);

			_connected = false;
			_subscribing = false;

			return _connected;
		}

		bool oracle::flush()
		{
			execute("COMMIT");
			return true;
		}

		bool oracle::ping()
		{
			timer t1(__PRETTY_FUNCTION__);

			if (_connected)
			{
				sword rc;

				if ((rc = OCIPing(_context, _error, (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::ping - OCIPing", axon::database::oracle::checker(_error, rc));
			}

			return true;
		}

		void oracle::version()
		{
			timer t1(__PRETTY_FUNCTION__);

			if (_connected)
			{
				sword rc;
				char sver[1024];

				if((rc = OCIServerVersion(_context, _error, (text*) sver, (ub4) sizeof(sver), (ub1) OCI_HTYPE_SVCCTX)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::version - OCIServerVersion", axon::database::oracle::checker(_error, rc));

				DBGPRN("database version %s", sver);
			}

			// return true;
		}
		
		bool oracle::watch(std::string query)
		{
			watch(query, [](axon::database::operation op, axon::database::change ch, std::string table, std::string rowid) {
				if (op == axon::database::operation::object && ch != axon::database::change::alter)
				{
					std::cout<<"row => "<<table<<" - "<<rowid<<std::endl;
				}
				else
				{
					std::cout<<"row => "<<table<<" non-dml operation"<<std::endl;
				}
			});

			return true;
		}

		bool oracle::watch(std::string query, cbfn fnptr)
		{
			// Database change notification trap. For details check the following URL
			// https://web.stanford.edu/dept/itss/docs/oracle/10gR2/appdev.102/b14250/oci09adv.htm

			timer t1(__PRETTY_FUNCTION__);

			bool rowids_needed = true;
			ub4 timeout = 0;
			sword rc;

			if (!_subscribing)
			{
				_subscription = (OCISubscription *) 0;
				ub4 ns = OCI_SUBSCR_NAMESPACE_DBCHANGE;

				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_subscription, OCI_HTYPE_SUBSCRIPTION, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)		// allocate subscription handle
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrSet(_subscription, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &ns, sizeof(ub4), OCI_ATTR_SUBSCR_NAMESPACE, _error)) != OCI_SUCCESS)			// set the namespace to DBCHANGE
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				_ctx.callback = fnptr;

				if ((rc = OCIAttrSet(_subscription, OCI_HTYPE_SUBSCRIPTION, (void *) &oracle::driver, 0, OCI_ATTR_SUBSCR_CALLBACK, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrSet(_subscription, OCI_HTYPE_SUBSCRIPTION, (void *) &_ctx, sizeof(_ctx), OCI_ATTR_SUBSCR_CTX, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIAttrSet(_subscription, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &rowids_needed, sizeof(ub4), OCI_ATTR_CHNF_ROWIDS, _error)) != OCI_SUCCESS)	// Allow extraction of rowid information
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIAttrSet(_subscription, OCI_HTYPE_SUBSCRIPTION, (dvoid *) &timeout, 0, OCI_ATTR_SUBSCR_TIMEOUT, _error)) != OCI_SUCCESS) 				// Set a timeout value of half an hour
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCISubscriptionRegister(_context, &_subscription, 1, _error, OCI_DEFAULT)) != OCI_SUCCESS)												// Create a new registration in the	DBCHANGE namespace */
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCISubscriptionRegister", axon::database::oracle::checker(_error, rc));

				_subscribing = true;			
			}

			OCIStmt *_stmt = (OCIStmt *) 0;

			if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_stmt, (ub4) OCI_HTYPE_STMT, 0, NULL)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));
			if ((rc = OCIStmtPrepare(_stmt, _error, (text *) query.c_str(), query.size(), OCI_V7_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)							// Prepare the statement
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIStmtPrepare", axon::database::oracle::checker(_error, rc));

			if ((rc = OCIAttrSet(_stmt, OCI_HTYPE_STMT, _subscription, 0, OCI_ATTR_CHNF_REGHANDLE, _error)) != OCI_SUCCESS)										// Associate the statement with the subscription handle
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIAttrSet", axon::database::oracle::checker(_error, rc));

			if ((rc = OCIStmtExecute(_context, _stmt, _error, (ub4) 0, (ub4) 0, (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, OCI_DEFAULT)) != OCI_SUCCESS)	// Execute the statement The execution of the statement	performs the
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::watch - OCIStmtExecute", axon::database::oracle::checker(_error, rc));

			OCIHandleFree(_stmt, OCI_HTYPE_STMT);

			return true;
		}

		bool oracle::unwatch()
		{
			timer t1(__PRETTY_FUNCTION__);

			sword rc;
			if (_subscribing)
			{
				if ((rc = OCISubscriptionUnRegister(_context, _subscription, _error, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::unwatch - OCISubscriptionUnRegister", axon::database::oracle::checker(_error, rc));

				OCIHandleFree((dvoid *) _subscription, OCI_HTYPE_SUBSCRIPTION); // this was commented for some reason!
				_subscribing = false;
			}

			return !_subscribing;
		}

		bool oracle::transaction(axon::trans_t ttype)
		{

			return true;
		}

		bool oracle::execute(const std::string &sql)
		{
			// TODO: DONE! How do we select all columns from a select statement? This we need to iterate between the field and see
			// how to pass the result accordingly. For now we are going to only return first field as this is the immediate requirement
			// For further reading, following link is helpful
			// https://docs.oracle.com/cd/B19306_01/appdev.102/b14250/oci04sql.htm

			timer t1(__PRETTY_FUNCTION__);

			// if (!_running)
			{
				std::lock_guard<std::mutex> lock(_lock);
				_running = true;

				sword rc;

				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 0, NULL)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIStmtPrepare(_statement, _error, (text*) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIStmtExecute(_context, _statement, _error, 1, 0, (OCISnapshot *) 0, (OCISnapshot *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, rc));

				OCIHandleFree(_statement, OCI_HTYPE_STMT); // We should call release to release statement handle!

				_running = false;
			}
			// else
			// 	throw axon::exception(__FILE__, __LINE__,"axon::query", "already busy with one query. please release first");

			return true;
		}

		bool oracle::execute(const std::string &sql, axon::database::bind *first, ...)
		{
			timer t1(__PRETTY_FUNCTION__);

			sword rc;
			unsigned int index = 1;
			std::va_list list;

			if (!_running)
			{
				_running = true;
				
				bind *x = first;

				va_start(list, first);

				if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 0, NULL)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrSet((dvoid *) _statement, OCI_HTYPE_STMT, (void*) &_prefetch, sizeof(int), OCI_ATTR_PREFETCH_ROWS, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIAttrSet", axon::database::oracle::checker(_error, rc));

				if ((rc = OCIStmtPrepare(_statement, _error, (const OraText *) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtPrepare", axon::database::oracle::checker(_error, rc));

				while (true)
				{
					if (x == nullptr)
						break;

					DBGPRN("Index: %lu, Pointer: %p", x->index(), x);

					if (x->index() == 0)
					{
						const char *type_owner_name   = "SYS";
						const char *type_name         = "ODCIVARCHAR2LIST";
						OCIType    *type_tdo          = NULL;
						OCIArray   *array             = (OCIArray *) 0;
						OCIBind    *bndp              = (OCIBind *) 0;
						std::vector<std::string> data = std::get<std::vector<std::string>>(*x);

						// pin duration can be either
						// OCI_DURATION_SESSION / OCI_DURATION_TRANS
						OCITypeByName(
							_environment, _error, _context,
							(CONST text *)type_owner_name, strlen(type_owner_name),
							(CONST text *) type_name, strlen(type_name),
							NULL, 0,
							OCI_DURATION_TRANS, OCI_TYPEGET_HEADER,
							&type_tdo
						);

						if ((rc = OCIObjectNew(_environment, _error, _context, OCI_TYPECODE_VARRAY, type_tdo, NULL, OCI_DURATION_TRANS, TRUE, (void **) &array)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIObjectNew", axon::database::oracle::checker(_error, rc));

						for (unsigned int i = 0; i < data.size(); i++)
						{
							OCIString *temp = (OCIString *) 0;

							if ((rc = OCIStringAssignText(_environment, _error, (text *) data[i].c_str(), (ub4) data[i].size(), &temp)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStringAssignText", axon::database::oracle::checker(_error, rc));
							if ((rc = OCICollAppend(_environment, _error, temp, NULL, array)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCICollAppend", axon::database::oracle::checker(_error, rc));
						}

						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, NULL, 0, SQLT_NTY, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));
						if ((rc = OCIBindObject(bndp, _error, type_tdo, (dvoid **) &array, NULL, NULL, NULL)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindObject", axon::database::oracle::checker(_error, rc));
					}
					else if (x->index() == 1)
					{
						const char *type_owner_name = "SYS";               
						const char *type_name       = "ODCINUMBERLIST";
						OCIType *type_tdo           = NULL;
						OCIArray *array             = (OCIArray *) 0;
						OCIBind *bndp               = (OCIBind *) 0;
						std::vector<double> data       = std::get<std::vector<double>>(*x);

						if ((rc = OCITypeByName(_environment, _error, _context, (CONST text *) type_owner_name, strlen(type_owner_name), (CONST text *) type_name, strlen(type_name), NULL, 0, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &type_tdo)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCITypeByName", axon::database::oracle::checker(_error, rc));

						if ((rc = OCIObjectNew(_environment, _error, _context, OCI_TYPECODE_VARRAY, type_tdo, NULL, OCI_DURATION_SESSION, TRUE, (void**) &array)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIObjectNew", axon::database::oracle::checker(_error, rc));

						for (unsigned int i = 0; i < data.size(); i++)
						{
							OCINumber num_val;
							
							if ((rc = OCINumberFromReal(_error, &data[i], sizeof(data[i]), &num_val)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCINumberFromInt", axon::database::oracle::checker(_error, rc));
							if ((rc = OCICollAppend(_environment, _error, &num_val, NULL, array)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCICollAppend", axon::database::oracle::checker(_error, rc));
						}

						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, NULL, 0, SQLT_NTY, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));

						if ((rc = OCIBindObject(bndp, _error, type_tdo, (dvoid **) &array, NULL, NULL, NULL)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindObject", axon::database::oracle::checker(_error, rc));
					}
					else if (x->index() == 2)
					{
						const char *type_owner_name = "SYS";               
						const char *type_name       = "ODCINUMBERLIST";
						OCIType *type_tdo           = NULL;
						OCIArray *array             = (OCIArray *) 0;
						OCIBind *bndp               = (OCIBind *) 0;
						std::vector<int> data       = std::get<std::vector<int>>(*x);

						if ((rc = OCITypeByName(_environment, _error, _context, (CONST text *) type_owner_name, strlen(type_owner_name), (CONST text *) type_name, strlen(type_name), NULL, 0, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &type_tdo)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCITypeByName", axon::database::oracle::checker(_error, rc));

						if ((rc = OCIObjectNew(_environment, _error, _context, OCI_TYPECODE_VARRAY, type_tdo, NULL, OCI_DURATION_SESSION, TRUE, (void**) &array)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIObjectNew", axon::database::oracle::checker(_error, rc));

						for (unsigned int i = 0; i < data.size(); i++)
						{
							OCINumber num_val;
							
							if ((rc = OCINumberFromInt(_error, &data[i], sizeof(data[i]), OCI_NUMBER_SIGNED, &num_val)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCINumberFromInt", axon::database::oracle::checker(_error, rc));
							if ((rc = OCICollAppend(_environment, _error, &num_val, NULL, array)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCICollAppend", axon::database::oracle::checker(_error, rc));
						}

						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, NULL, 0, SQLT_NTY, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));

						if ((rc = OCIBindObject(bndp, _error, type_tdo, (dvoid **) &array, NULL, NULL, NULL)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindObject", axon::database::oracle::checker(_error, rc));
					}
					else if (x->index() == 3)
					{
						OCIBind *bndp = (OCIBind *) 0;
						text *data    = std::get<text *>(*x);

						DBGPRN("# Data: >%s<, Len: %lu", data, strlen((char *) data));
						
						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, data, strlen((char *) data)+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));
					}
					else if (x->index() == 4)
					{
						OCIBind *bndp = (OCIBind *) 0;
						double data   = std::get<double>(*x);

						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, (dvoid *) &data, (sword) sizeof(data), SQLT_BDOUBLE, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));
					}
					else if (x->index() == 5)
					{
						OCIBind *bndp = (OCIBind *) 0;
						int data      = std::get<int>(*x);

						if ((rc = OCIBindByPos(_statement, &bndp, _error, index, (dvoid *) &data, (sword) sizeof(data), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByPos", axon::database::oracle::checker(_error, rc));
					}
					else
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query", "Unknown data type");

					x = va_arg(list, bind*);

					index++;
				}

				if ((rc = OCIStmtExecute(_context, _statement, _error, 1, 0, NULL, NULL, OCI_DEFAULT)) != OCI_SUCCESS) // prefetch is set to 0
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, rc));

				OCIHandleFree(_statement, OCI_HTYPE_STMT); // We should call release to release statement handle!

				_running = false;
				
				va_end(list);
			}
			else
				throw axon::exception(__FILE__, __LINE__,"axon::query", "already busy with one query. please release first");

			return true;
		}

		bool oracle::query(std::string sql, std::vector<std::string> data)
		{
			// This is the bind one list overload of the hard query
			//
			// How to use list bind variable with IN statement was borrowed with thanks from
			// https://stackoverflow.com/questions/18603281/oracle-oci-bind-variables-and-queries-like-id-in-1-2-3

			timer t1(__PRETTY_FUNCTION__);
			
			int prefetch = 100;
			sword rc, retval;

			// char const *owner = "SYS";
			// char const *name = "ODCIVARCHAR2LIST"; // ODCINUMBERLIST, ODCIVARCHAR2LIST
			// OCIType *tdo = NULL;

			const char	*type_owner_name	= "SYS";
			const char	*type_name			= "ODCIVARCHAR2LIST";
			OCIType 	*type_tdo			= NULL;

			OCITypeByName(
				_environment, _error, _context,
				(CONST text *)type_owner_name, strlen(type_owner_name),
				(CONST text *) type_name, strlen(type_name),
				NULL, 0,
				OCI_DURATION_SESSION, OCI_TYPEGET_HEADER,
				&type_tdo
			);
			// if ((rc = OCITypeByName(_environment, _error, _context, (const text *) owner, sizeof(owner), (const text *) name, sizeof(name), NULL, 0, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &tdo)) != OCI_SUCCESS)
			// 	throw DBERR(rc, "axon::database::oracle::query", "OCITypeByName");

			OCIArray *array = (OCIArray *) 0;

			if ((rc = OCIObjectNew(_environment, _error, _context, OCI_TYPECODE_VARRAY, type_tdo, NULL, OCI_DURATION_SESSION, TRUE, (void **) &array)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIObjectNew", axon::database::oracle::checker(_error, rc));

			for (unsigned int i = 0; i < data.size(); i++)
			{
				OCIString *temp = (OCIString *) 0;

				if ((rc = OCIStringAssignText(_environment, _error, (text *) data[i].c_str(), (ub4) data[i].size(), &temp)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStringAssignText", axon::database::oracle::checker(_error, rc));
				if ((rc = OCICollAppend(_environment, _error, temp, NULL, array)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCICollAppend", axon::database::oracle::checker(_error, rc));
			}

			// TODO: For Number type collection
			// for (int i = 0; i < data.size(); i++)
			// {
			// 	OCINumber num_val;
			// 	int int_val=8837;

			// 	checker(OCINumberFromInt(_error, &int_val, sizeof(i), OCI_NUMBER_SIGNED, &num_val));
			// 	checker(OCICollAppend(_environment, _error, &num_val, NULL, array));
			// }

			if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 0, NULL)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));
			if ((rc = OCIAttrSet((dvoid *) _statement, OCI_HTYPE_STMT, (void*) &prefetch, sizeof(int), OCI_ATTR_PREFETCH_ROWS, _error)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIAttrSet", axon::database::oracle::checker(_error, rc));

			if ((rc = OCIStmtPrepare(_statement, _error, (text*) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtPrepare", axon::database::oracle::checker(_error, rc));

			OCIBind *bndp;

			// Bind where-list
			if ((rc = OCIBindByName(_statement, &bndp, _error, (text *)":list", strlen(":list"), NULL, 0, SQLT_NTY, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIBindByName", axon::database::oracle::checker(_error, rc));
			if ((rc = OCIBindObject(bndp, _error, type_tdo, (dvoid **) &array, NULL, NULL, NULL)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCI", axon::database::oracle::checker(_error, rc));

			if ((retval = OCIStmtExecute(_context, _statement, _error, 0, 0, NULL, NULL, OCI_DEFAULT)) != OCI_SUCCESS) // prefetch is set to 0
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, retval));

			OCIHandleFree(_statement, OCI_HTYPE_STMT); // We should call release to release statement handle!

			return true;
		}

		bool oracle::query(std::string sql)
		{
			// This is the bind one list overload of the hard query
			//
			// How to use list bind variable with IN statement was borrowed with thanks from
			// https://stackoverflow.com/questions/18603281/oracle-oci-bind-variables-and-queries-like-id-in-1-2-3

			timer t1(__PRETTY_FUNCTION__);
			
			int prefetch = 100;
			sword rc;

			_executed = false;
			_index = 1;

			if ((rc = OCIHandleAlloc((dvoid *) _environment, (dvoid **) &_statement, (ub4) OCI_HTYPE_STMT, 0, NULL)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIHandleAlloc", axon::database::oracle::checker(_error, rc));
			if ((rc = OCIAttrSet((dvoid *) _statement, OCI_HTYPE_STMT, (void*) &prefetch, sizeof(int), OCI_ATTR_PREFETCH_ROWS, _error)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIAttrSet", axon::database::oracle::checker(_error, rc));

			if ((rc = OCIStmtPrepare(_statement, _error, (text*) sql.c_str(), sql.size(), OCI_NTV_SYNTAX, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtPrepare", axon::database::oracle::checker(_error, rc));

			return true;
		}

		bool oracle::next()
		{
			sword rc;
	
			if (!_executed)
			{
				_colidx = 0;
				if ((rc = OCIStmtExecute(_context, _statement, _error, 0, 0, NULL, NULL, OCI_DEFAULT)) != OCI_SUCCESS) // prefetch is set to 0
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::query - OCIStmtExecute", axon::database::oracle::checker(_error, rc));

				build();

				_executed = true;
			}

			DBGPRN("_fetched: %d, _rownum: %d", _fetched, _rownum);

			if (_rownum+1 < _fetched)
			{
				_rownum++;
			}
			else
			{
				_fetched = 0;
				_rownum = 0;

				timer t1(__PRETTY_FUNCTION__);

				rc = OCIStmtFetch2(_statement, _error, _prefetch, OCI_DEFAULT, 0, OCI_DEFAULT);
				OCIAttrGet(_statement, OCI_HTYPE_STMT, (void*) &_fetched, NULL, OCI_ATTR_ROWS_FETCHED, _error);

				if (rc == OCI_SUCCESS && rc == OCI_NO_DATA && _fetched == 0)
					return false;

				if (_fetched == 0)
					return false;
			}

			_colidx = 0;

			return true;
		}

		void oracle::done()
		{
			_index = 1;
			_colidx = 0;

			if (_dirty)
			{
				for (unsigned int x = 0; x < _colcnt; x++)
				{
					if (_col[x].indicator)
						free(_col[x].indicator);

					if (_col[x].data)
						free(_col[x].data);
				}

				if (_statement != NULL)
					OCIHandleFree(_statement, OCI_HTYPE_STMT);

				_dirty = false;
			}
			
			OCIHandleFree(_statement, OCI_HTYPE_STMT); // We should call release to release statement handle!
		}

		bool oracle::build()
		{
			timer t1(__PRETTY_FUNCTION__);

			int rc;

			ub4 paramcnt;
			OCIParam *_param = (OCIParam *) 0;

			if ((rc = OCIAttrGet((CONST dvoid *) _statement, (ub4) OCI_HTYPE_STMT, (void *) &paramcnt, (ub4 *) 0, (ub4) OCI_ATTR_PARAM_COUNT, _error)) < OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIAttrGet", axon::database::oracle::checker(_error, rc));

			DBGPRN("Number of columns in query is %d", paramcnt);
			_colcnt = paramcnt;
			_dirty = true;

			for (ub4 i = 1; i <= paramcnt; i++)
			{
				OCIDefine *dptr = (OCIDefine *) 0;
				ub4 namelen = 128;

				_col.push_back({NULL, i, 0, 0, 0, NULL, NULL});

				if ((rc = OCIParamGet((dvoid *) _statement, (ub4) OCI_HTYPE_STMT, (OCIError *) _error, (dvoid **) &_param, (ub4) i)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIParamGet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid **) &_col[i-1].name, (ub4 *) &namelen, (ub4) OCI_ATTR_NAME, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIAttrGet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, &_col[i-1].type, (ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIAttrGet", axon::database::oracle::checker(_error, rc));
				if ((rc = OCIAttrGet((dvoid *) _param, (ub4) OCI_DTYPE_PARAM, (dvoid *) &_col[i-1].size, (ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE, _error)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIAttrGet", axon::database::oracle::checker(_error, rc));

				_col[i-1].name[namelen] = 0;
				_col[i-1].size += 1;
				
				DBGPRN("Column %d: name = %s, type = %u, size = %u", i, _col[i-1].name , _col[i-1].type, _col[i-1].size);

				/*
					#define SQLT_CHR         1 (ORANET TYPE) character string
					#define SQLT_NUM         2   (ORANET TYPE) oracle numeric
					#define SQLT_LNG         8                           long
					#define SQLT_DAT        12          date in oracle format
					#define SQLT_AFC        96                Ansi fixed char
					#define SQLT_TIMESTAMP 187                      TIMESTAMP
				*/

				switch (_col[i-1].type)
				{
					case SQLT_CHR:
					case SQLT_STR:
					case SQLT_VCS:
					case SQLT_AFC:
					case SQLT_AVC:
						_col[i-1].memsize = (sizeof(char) * _col[i-1].size * _prefetch);
						_col[i-1].data = malloc(_col[i-1].memsize);
						std::memset(_col[i-1].data, 0, _col[i-1].memsize);

						_col[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement, &dptr, _error, i, _col[i-1].data, _col[i-1].size, SQLT_STR, _col[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIDefineByPos", axon::database::oracle::checker(_error, rc));
						break;
					
					case SQLT_NUM:
					case SQLT_LNG:
						_col[i-1].memsize = (sizeof(OCINumber) * _prefetch);
						_col[i-1].data = malloc(_col[i-1].memsize);
						std::memset(_col[i-1].data, 0, _col[i-1].memsize);

						_col[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement, &dptr, _error, i, _col[i-1].data, sizeof(OCINumber), SQLT_VNU, _col[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIDefineByPos", axon::database::oracle::checker(_error, rc));
						break;
					
					case SQLT_DAT:
					case SQLT_TIMESTAMP:
						_col[i-1].memsize = (sizeof(char) * _col[i-1].size * _prefetch);
						_col[i-1].data = malloc(_col[i-1].memsize);
						std::memset(_col[i-1].data, 0, _col[i-1].memsize);

						_col[i-1].indicator = malloc(sizeof(sb2) * _prefetch);

						if ((rc = OCIDefineByPos(_statement, &dptr, _error, i, _col[i-1].data, _col[i-1].size, SQLT_DAT, _col[i-1].indicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT)) != OCI_SUCCESS)
								throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::build - OCIDefineByPos", axon::database::oracle::checker(_error, rc));
						break;
					
					default:
						DBGPRN("Unknown type: %d", _col[i-1].type);
				}
			}

			return true;
		}

		std::string oracle::get(unsigned int i)
		{
			axon::timer t1(__PRETTY_FUNCTION__);

			std::string value;

			DBGPRN("_index: %d, _colcnt: %d, i: %d, col.type: %d", _index, _colcnt, i, _col[_colidx].type);
			
			if (i >= _colcnt)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Column out of bounds");

			switch (_col[i].type)
			{
				case SQLT_AFC:
				case SQLT_CHR:
					value = reinterpret_cast<char const*>((((text *) _col[i].data)+(_rownum*_col[i].size)));
					break;
				
				case SQLT_DAT:
				case SQLT_TIMESTAMP:
					{
						// detail why I did the following can be found at https://www.sqlines.com/oracle/datatypes/date
						char temp[64];
						char *raw;

						raw = ((char*) _col[i].data) + (_rownum*_col[i].size);

						sprintf(temp, "%02d-%02d-%02d%02d %02d:%02d:%02d", 
									((int)raw[3]), ((int)raw[2]), ((int)raw[0])-100, ((int)raw[1])-100, 
									((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
						value = temp;
					}
					break;

				case SQLT_NUM:
					{
						double temp;
						//OCINumberToInt(_error, (((OCINumber *) _col[_colidx].data)+_rownum), sizeof(int), OCI_NUMBER_SIGNED, &temp);
						OCINumberToReal(_error, (((OCINumber *) _col[i].data)+_rownum), sizeof(double), &temp);
						value = std::to_string(temp);
					}
					break;
					
				default:
					throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Variable type is not compatable with row type");
			}

			return value;
		}

		oracle& oracle::operator<<(int value)
		{
			OCIBind *bndp = (OCIBind *) 0;
			int rc;

			if ((rc = OCIBindByPos(_statement, &bndp, _error, _index, (dvoid *) &value, (sword) sizeof(value), SQLT_INT, NULL, 0, 0, 0, 0, OCI_DEFAULT )) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "oracle::operator(<<) - OCIBindByPos", axon::database::oracle::checker(_error, rc));
			
			_index++;

			return *this;
		}

		oracle& oracle::operator<<(std::string& value)
		{
			OCIBind *bndp = (OCIBind *) 0;
			int rc;

			DBGPRN("# Index: %d, Data: >%s<, Len: %lu", _index, value.data(), value.size());
			
			if ((rc = OCIBindByPos(_statement, &bndp, _error, _index, value.data(), value.size()+1, SQLT_STR, NULL, 0, 0, 0, 0, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "oracle::operator(<<) - OCIBindByPos", axon::database::oracle::checker(_error, rc));
			
			_index++;

			return *this;
		}

		oracle& oracle::operator>>(int &value)
		{
			// it was difficult to figure how to extract SQLT_NUM from Returned object
			// Reference on how to extract long long helped. check the following link
			// https://stackoverflow.com/questions/25808798/ocidefinebypos-extract-number-value-via-cursor-in-c
			//
			// API details on how to extract different type of number via OCI Number functions
			// https://docs.oracle.com/cd/B28359_01/appdev.111/b28395/oci19map003.htm

			axon::timer t1(__PRETTY_FUNCTION__);

			DBGPRN("_index: %d, _colcnt: %d, _colidx: %d, col.type: %d", _index, _colcnt, _colidx, _col[_colidx].type);
			
			if (_colidx > _colcnt)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Column out of bounds");

			if (_col[_colidx].type != SQLT_NUM)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Variable type is not compatable with row type");

			OCINumberToInt(_error, (((OCINumber *) _col[_colidx].data)+_rownum), sizeof(int), OCI_NUMBER_SIGNED, &value);

			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(double &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);

			DBGPRN("_index: %d, _colcnt: %d, _colidx: %d, col.type: %d", _index, _colcnt, _colidx, _col[_colidx].type);

			if (_colidx >= _colcnt)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Column out of bounds");

			if (_col[_colidx].type != 2)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Variable type is not compatable with row type");

			OCINumberToReal(_error, (((OCINumber *) _col[_colidx].data)+_rownum), sizeof(double), &value);

			_index++;
			return *this;
		}

		oracle& oracle::operator>>(std::string &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);

			DBGPRN("_index: %d, _colcnt: %d, _colidx: %d, col.type: %d", _index, _colcnt, _colidx, _col[_colidx].type);
			
			if (_colidx >= _colcnt)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Column out of bounds");

			switch (_col[_colidx].type)
			{
				case SQLT_AFC:
				case SQLT_CHR:
					value = reinterpret_cast<char const*>((((text *) _col[_colidx].data)+(_rownum*_col[_colidx].size)));
					break;
				
				case SQLT_DAT:
				case SQLT_TIMESTAMP:
					{
						// detail why I did the following can be found at https://www.sqlines.com/oracle/datatypes/date
						char temp[64];
						char *raw;

						raw = ((char*) _col[_colidx].data) + (_rownum*_col[_colidx].size);

						sprintf(temp, "%02d-%02d-%02d%02d %02d:%02d:%02d", 
									((int)raw[3]), ((int)raw[2]), ((int)raw[0])-100, ((int)raw[1])-100, 
									((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
						value = temp;
					}
					break;

				case SQLT_NUM:
					{
						double temp;
						//OCINumberToInt(_error, (((OCINumber *) _col[_colidx].data)+_rownum), sizeof(int), OCI_NUMBER_SIGNED, &temp);
						OCINumberToReal(_error, (((OCINumber *) _col[_colidx].data)+_rownum), sizeof(double), &temp);
						value = std::to_string(temp);
					}
					break;
					
				default:
					throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(std::string&)", "Variable type is not compatable with row type");
			}

			_colidx++;
			return *this;
		}

		oracle& oracle::operator>>(std::time_t &value)
		{
			axon::timer t1(__PRETTY_FUNCTION__);

			DBGPRN("_index: %d, _colcnt: %d, _colidx: %d, col.type: %d", _index, _colcnt, _colidx, _col[_colidx].type);
			
			if (_colidx > _colcnt)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Column out of bounds");

			if (_col[_colidx].type != SQLT_DAT && _col[_colidx].type != SQLT_TIMESTAMP)
				throw axon::exception(__FILE__, __LINE__, "recordset::operator>>(int&)", "Variable type is not compatable with row type");

			char *raw;
			struct tm temp;

			raw = ((char*) _col[_colidx].data) + (_rownum*_col[_colidx].size);

			temp.tm_sec = ((int)raw[6]) - 1;
			temp.tm_min = ((int)raw[5]) - 1;
			temp.tm_hour = ((int)raw[4]) - 1;
			temp.tm_mday = ((int)raw[3]);
			temp.tm_mon = ((int)raw[2]) - 1;
			temp.tm_year = ((int)raw[1]);
			
			value = std::mktime(&temp);

			_colidx++;
			return *this;
		}

		void oracle::driver(dvoid *ctx, OCISubscription *subscrhp, dvoid *payload, ub4 *payl, dvoid *descriptor, ub4 mode)
		{
			OCIEnv *_h_env;
			OCIError *_h_err;
			OCIServer *_h_srv;
			OCISvcCtx *_h_svc;
			OCISession *_h_usr;
			OCIStmt *_h_stmt;
			dvoid *tmp;

			ub4 event_type;
			OCIColl *changes = (OCIColl *) 0;
			sb4 count = 0;
			context *pf = (context *) ctx;
			sword rc;

			std::string table, rowid;

			if ((rc = OCIEnvCreate((OCIEnv **) &_h_env, OCI_OBJECT, (dvoid *) 0, (dvoid * (*)(dvoid *, size_t)) 0, (dvoid * (*)(dvoid *, dvoid *, size_t)) 0, (void(*)(dvoid *, dvoid *)) 0, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIEnvCreate", axon::database::oracle::checker(_h_err, rc));
			if ((rc = OCIHandleAlloc((dvoid *) _h_env, (dvoid **) &_h_err, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIHandleAlloc", axon::database::oracle::checker(_h_err, rc));

			if ((rc = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &event_type, NULL, OCI_ATTR_CHDES_NFYTYPE, _h_err)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

			if (event_type == OCI_EVENT_OBJCHANGE)
			{
				{
					/* server contexts */
					if ((rc = OCIHandleAlloc((dvoid *) _h_env, (dvoid **) &_h_srv, OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIHandleAlloc", axon::database::oracle::checker(_h_err, rc));
					if ((rc = OCIHandleAlloc((dvoid *) _h_env, (dvoid **) &_h_svc, OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIHandleAlloc", axon::database::oracle::checker(_h_err, rc));

					/* Allocate a statement handle */
					if ((rc = OCIHandleAlloc((dvoid *) _h_env, (dvoid **) &_h_stmt, (ub4) OCI_HTYPE_STMT, 52, (dvoid **) &tmp)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIHandleAlloc", axon::database::oracle::checker(_h_err, rc));

					/* set attribute server context in the service context */
					if ((rc = OCIAttrSet((dvoid *) _h_svc, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _h_srv, (ub4) 0, (ub4) OCI_ATTR_SERVER, (OCIError *) _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrSet", axon::database::oracle::checker(_h_err, rc));

					if ((rc = OCIServerAttach(_h_srv, _h_err, (text *) pf->sid,  (sb4) strlen(pf->sid), (ub4) OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIServerAttach", axon::database::oracle::checker(_h_err, rc));

					/* allocate a SESSION	handle */
					if ((rc = OCIHandleAlloc((dvoid *) _h_env, (dvoid **) &_h_usr, (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIHandleAlloc", axon::database::oracle::checker(_h_err, rc));

					if ((rc = OCIAttrSet((dvoid *) _h_usr, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) pf->username), (ub4) strlen(pf->username), OCI_ATTR_USERNAME, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrSet", axon::database::oracle::checker(_h_err, rc));
					if ((OCIAttrSet((dvoid *) _h_usr, (ub4) OCI_HTYPE_SESSION, (dvoid *)((text *) pf->password), (ub4) strlen(pf->password), OCI_ATTR_PASSWORD, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrSet", axon::database::oracle::checker(_h_err, rc));

					if ((rc = OCISessionBegin(_h_svc, _h_err, _h_usr, OCI_CRED_RDBMS, OCI_DEFAULT)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCISessionBegin", axon::database::oracle::checker(_h_err, rc));
					if ((rc = OCIAttrSet((dvoid *) _h_svc, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) _h_usr, (ub4) 0, OCI_ATTR_SESSION, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrSet", axon::database::oracle::checker(_h_err, rc));
				}

				/* Obtain the collection of table change descriptors */
				if ((rc = OCIAttrGet(descriptor, OCI_DTYPE_CHDES, &changes, NULL, OCI_ATTR_CHDES_TABLE_CHANGES, _h_err)) != OCI_SUCCESS)
					throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

				/* Obtain the size of the collection(i.e number of tables modified) */

				if(changes)
				{
					if ((rc = OCICollSize(_h_env, _h_err, (CONST OCIColl *) changes, &count)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCICollSize", axon::database::oracle::checker(_h_err, rc));
				}
				else
					count = 0;

				for (int i = 0; i < count; i++)
				{
					dvoid **tprt, **rptr; /* Pointer to Change Descriptor */
					ub4 top, rop;
					OCIColl *row_changes = (OCIColl	*) 0; /* Collection of pointers to row chg. Descriptors */
					text *row_id;
					ub4 rowid_size;
					sb4 num_rows;
					boolean exist;
					dvoid *elemind = (dvoid *) 0;
					text *table_name;

					if ((rc = OCICollGetElem(_h_env, _h_err, (OCIColl *) changes, i, &exist, (void**)&tprt, &elemind)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCICollGetElem", axon::database::oracle::checker(_h_err, rc));

					if ((rc = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES, &table_name,  NULL, OCI_ATTR_CHDES_TABLE_NAME, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

					table = (char *) table_name;
					if ((rc = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES,  (dvoid *) &top, NULL, OCI_ATTR_CHDES_TABLE_OPFLAGS, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

					if (top & OCI_OPCODE_ALLROWS) // Too many changes, cannot track
					{
						// printf("++ %i\n", table_op);
						if (top & OCI_OPCODE_ALTER)
							pf->callback(operation::object, change::alter, table, rowid);
						continue;
					}

					if ((rc = OCIAttrGet(*tprt, OCI_DTYPE_TABLE_CHDES, &row_changes, NULL, OCI_ATTR_CHDES_TABLE_ROW_CHANGES, _h_err)) != OCI_SUCCESS)
						throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

					if(row_changes)
					{
						if ((rc = OCICollSize(_h_env, _h_err, row_changes, &num_rows)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCICollSize", axon::database::oracle::checker(_h_err, rc));
					}
					else
						num_rows = 0;

					DBGPRN("table => %s (%d)", table_name, num_rows);

					for (int j = 0; j < num_rows; j++)
					{
						if ((rc = OCICollGetElem(_h_env, _h_err, (OCIColl *) row_changes, j, &exist, (void**) &rptr, &elemind)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCICollGetElem", axon::database::oracle::checker(_h_err, rc));

						if ((rc = OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, (dvoid *) &row_id, &rowid_size, OCI_ATTR_CHDES_ROW_ROWID, _h_err)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));
						if ((rc = OCIAttrGet(*rptr, OCI_DTYPE_ROW_CHDES, (dvoid *) &rop, NULL, OCI_ATTR_CHDES_ROW_OPFLAGS, _h_err)) != OCI_SUCCESS)
							throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIAttrGet", axon::database::oracle::checker(_h_err, rc));

						rowid = (char *) row_id;

						pf->callback(operation::object, change::insert, table, rowid);
					}
				}
			}
			else
				pf->callback((operation) event_type, change::unknown, table, rowid);

			/* End session and detach from server */
			if ((rc = OCISessionEnd(_h_svc, _h_err, _h_usr, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCISessionEnd", axon::database::oracle::checker(_h_err, rc));
			if ((rc = OCIServerDetach(_h_srv, _h_err, OCI_DEFAULT)) != OCI_SUCCESS)
				throw axon::exception(__FILE__, __LINE__, "axon::database::oracle::driver - OCIServerDetach", axon::database::oracle::checker(_h_err, rc));

			if(_h_stmt)
				OCIHandleFree((dvoid *) _h_stmt, OCI_HTYPE_STMT);
			if(_h_err)
				OCIHandleFree((dvoid *) _h_err, OCI_HTYPE_ERROR);
			if(_h_srv)
				OCIHandleFree((dvoid *) _h_srv, OCI_HTYPE_SERVER);
			if(_h_svc)
				OCIHandleFree((dvoid *) _h_svc, OCI_HTYPE_SVCCTX);
			if(_h_usr)
				OCIHandleFree((dvoid *) _h_usr, OCI_HTYPE_SESSION);
			if(_h_env)
				OCIHandleFree((dvoid *) _h_env, OCI_HTYPE_ENV);
		}

		std::ostream& oracle::printer(std::ostream &stream)
		{
			timer t1(__PRETTY_FUNCTION__);

			for (unsigned int i = 0; i < _colcnt; i++)
			{
				// since C has no concept of passing null values, the indicator param
				// "indicates" which fields are empty. For details check the following
				// https://docs.oracle.com/cd/B28359_01/appdev.111/b28395/oci02bas.htm#LNOCI87630
				
				// sb2 *indicator[] = (sb2 *) _col[i].indicator;
				// memcpy(indicator, _col[i].indicator, _prefetch*sizeof(sb2));
				// std::cout<<(*indicator)[1]<<std::endl;

				switch (_col[i].type)
				{
					case SQLT_CHR:
					case SQLT_STR:
					case SQLT_VCS:
					case SQLT_AFC:
					case SQLT_AVC:
					{
						int indicator = ((sb2 *) _col[i].indicator)[_rownum];

						if (indicator == 0)
							stream<<reinterpret_cast<char const*>((((text *) _col[i].data)+(_rownum*_col[i].size)));
					}
					break;

					case SQLT_NUM:
					{
						int indicator = ((sb2 *) _col[i].indicator)[_rownum];

						if (indicator != -1)
						{
							double value = 0;
							OCINumber data[_prefetch];

							memcpy(data, _col[i].data, _col[i].memsize);
							OCINumber temp = data[_rownum];

							OCINumberToReal(_error, &temp, sizeof(double), &value);

							if (value != (int) value)
								stream<<std::fixed<<value;
							else
								stream<<(int)value;
						}
						else
							stream<<"NULL";
					}
					break;

					case SQLT_DAT:
					case SQLT_TIMESTAMP:
					{
						int indicator = ((sb2 *) _col[i].indicator)[_rownum];

						if (indicator == 0)
						{
							char temp[64];
							char *raw;

							raw = ((char*) _col[i].data) + (_rownum*_col[i].size);

							sprintf(temp, "%02d-%02d-%02d%02d %02d:%02d:%02d", 
										((int)raw[3]), ((int)raw[2]), ((int)raw[0])-100, ((int)raw[1])-100, 
										((int)raw[4])-1, ((int)raw[5])-1, ((int)raw[6])-1);
							stream<<temp;
						}
					}
					break;
				}

				stream<<",";
			}

			return stream;
		}
	}
}
