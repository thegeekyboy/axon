#ifndef AXON_OCN_H_
#define AXON_OCN_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <oci.h>
#include <boost/regex.hpp>

#include <axon.h>
#include <axon/util.h>
#include <axon/resultset.h>
#include <axon/database.h>
#include <axon/stream.h>
#include <axon/oci.h>

// -----------------------------------------------------------------------
// axon::stream::ocn
//
// Oracle OCN (Object Change Notification) stream connector.
// Derives from axon::stream::connector.
//
// Self-contained — owns its own OCI session. Does NOT depend on
// axon::database::oracle.
//
// THREADING
//   OCI background thread → _notify() static callback
//     ↓  push ocn_event + notify _queue_cv
//   _daemon thread         → dequeue → ROWID fetch → resultset → callback
//
// USAGE
//   axon::stream::ocn ocn("mydb", "user", "pass");
//   ocn.connect();
//   ocn.add("ORDERS", "SELECT * FROM TBL_ORDERS", [](auto rs) { ... });
//   ocn.subscribe();
//   ocn.start();   // or start(global_cb)
//   // ... application runs ...
//   ocn.stop();
//   ocn.disconnect();
// -----------------------------------------------------------------------

namespace axon {

	namespace stream {

		struct ocn_event {
			size_t topic_index { 0 };
			std::string table;
			std::string rowid;      // empty = ALLROWS bulk change
			ub4 row_op { 0 };
		};

		// Per-topic OCI subscription handle
		struct ocn_sub {
			OCISubscription *handle      { nullptr };
			size_t topic_index { 0 };
			bool attached { false };
		};

		class ocn : public axon::stream::connector {

			std::shared_ptr<axon::database::oci::connection> _connection;
			axon::database::oci::error _error;

			std::vector<axon::stream::ocn_sub> _subs;

			std::deque<axon::stream::ocn_event> _queue;
			std::mutex _queue_mtx;
			std::condition_variable _queue_cv;

			void _attach(ocn_sub &sub, const axon::stream::topic &t);
			void _detach(ocn_sub &sub);
			void _dispatch_loop();
			void _handle_event(const ocn_event &ev);
			void _fetch_row(const std::string &table, const std::string &rowid, const axon::stream::cbfn &cb);

			void _stop() override;

		public:

			ocn() = delete;
			ocn(const ocn&) = delete;

			ocn(std::string, std::string, std::string);
			ocn(std::shared_ptr<axon::database::oci::connection>);
			~ocn();

			void connect();
			void disconnect();

			void subscribe() override;
			void unsubscribe() override;

			bool start() override;
			bool start(axon::stream::cbfn) override;

			void fetch(axon::resultset &, int) override {
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "ocn is callback-only — use start() with a callback");
			}

			static ub4 _notify(dvoid*, OCISubscription*, dvoid*, ub4*, dvoid*, ub4);
		};

	}
}

#endif