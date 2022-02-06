#pragma once

#include <oci.h>

#include <axon/database.h>

//#define DBERR(rc, errhand, funcsig, funcoci) axon::exception(__FILE__, __LINE__, funcsig, getErrorMessage(rc, errhand))
// #define DBERR(rc, errhand, funcsig, funmsg) axon::exception(__FILE__, __LINE__, funcsig, std::string("nothing"))

namespace axon {

	namespace database {

		enum operation {
			none = 0x0,
			startup = 0x1,
			shutdown = 0x2,
			shutany = 0x3,
			dropdb = 0x4,
			unregister = 0x5,
			object = 0x6,
			querychange = 0x7
		};

		enum change {
			allops = 0x0,
			allrows = 0x1,
			insert = 0x2,
			update = 0x4,
			remove = 0x8,
			alter = 0x10,
			drop = 0x20,
			unknown = 0x40
		};

		typedef void (*ndfn) (dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);
		typedef void (*cbfn) (operation, change, std::string, std::string);

		struct context {
			char sid[256];
			char username[256];
			char password[256];
			cbfn callback;
		};
		typedef context context;

		class oracle:public interface {

			OCISvcCtx *_service;
			OCIError *_error;
			OCISession *_session;
			OCIStmt *_statement;
			OCIEnv *_environment;
			OCIServer *_server;
			OCISubscription *_subscription;

			std::string _sid, _username, _password;

			bool _connected, _subscribing, _running;
			std::string _uuid;
			unsigned int _prefetch;

			std::mutex _lock;

			int _port;
			context _ctx;

			using cb_helper = lambda<50, void, dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4>;

			public:
				oracle();
				~oracle();

				OCIError *getError();
				OCIEnv *getEnvironment();
				OCISvcCtx *getService();

				void checker(sword);
				static std::string checker(OCIError *, sword);

				bool connect();
				bool connect(std::string, std::string, std::string);
				bool close();
				bool flush();
				
				bool ping();
				void version();

				bool watch(std::string);
				bool watch(std::string, cbfn);
				bool unwatch();

				bool execute(const std::string&);
				bool execute(const std::string&, axon::database::bind*, ...);
				void query(std::string, std::vector<std::string>);
				void query(std::string);

				static void driver(dvoid *, OCISubscription *, dvoid *, ub4 *, dvoid *, ub4);

				// friend class dcn::recordset;
		};
	}
}