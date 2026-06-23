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
#include <axon/recordset2r.h>
#include <axon/database2r.h>
#include <axon/stream2r.h>
#include <axon/oracle.h>

// -----------------------------------------------------------------------
// axon::stream2r::ocn
//
// Oracle OCN (Object Change Notification) stream connector.
// Derives from axon::stream2r::connector.
//
// Self-contained — owns its own OCI session. Does NOT depend on
// axon::database2r::oracle.
//
// THREADING
//   OCI background thread → _notify() static callback
//     ↓  push ocn_event + notify _queue_cv
//   _daemon thread         → dequeue → ROWID fetch → recordset2r → callback
//
// USAGE
//   axon::stream2r::ocn ocn("mydb", "user", "pass");
//   ocn.connect();
//   ocn.add("ORDERS", "SELECT * FROM TBL_ORDERS", [](auto rs) { ... });
//   ocn.subscribe();
//   ocn.start();   // or start(global_cb)
//   // ... application runs ...
//   ocn.stop();
//   ocn.disconnect();
// -----------------------------------------------------------------------

namespace axon {

	namespace stream2r {

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

		class ocn : public axon::stream2r::connector {

			// ── OCI session ──────────────────────────────────────────────────
			axon::database2r::environment _environment;
			std::shared_ptr<axon::database2r::context> _context;
			axon::database2r::error _error;
			axon::database2r::server _server;
			axon::database2r::session _session;

			// ── Subscriptions ────────────────────────────────────────────────
			std::vector<ocn_sub> _subs;

			// ── Dispatch queue ───────────────────────────────────────────────
			std::deque<ocn_event> _queue;
			std::mutex _queue_mtx;
			std::condition_variable _queue_cv;

			// ── Internal methods ──────────────────────────────────────────────
			void _attach(ocn_sub &sub, const axon::stream2r::topic &t);
			void _detach(ocn_sub &sub);
			void _dispatch_loop();
			void _handle_event(const ocn_event &ev);
			void _fetch_row(const std::string &table, const std::string &rowid, const axon::stream2r::cbfn &cb);

			// Required by connector::stop()
			void _stop() override;

		public:

			ocn() = delete;
			ocn(const ocn&) = delete;

			// hostname = Oracle SID/TNS, username, password
			ocn(std::string, std::string, std::string);
			~ocn();

			// OCI session lifecycle — separate from stream lifecycle
			void connect();
			void disconnect();

			// connector pure virtuals
			void subscribe() override;
			void unsubscribe() override;

			bool start() override;
			bool start(axon::stream2r::cbfn) override;

			// OCN is callback-only — fetch() throws to signal misuse
			void fetch(axon::recordset2r &, int) override
			{
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "ocn is callback-only — use start() with a callback");
			}

			// Static OCI C callback — fires on OCI's background thread.
			// Must not call oracle, must not throw, must return promptly.
			static ub4 _notify(dvoid*, OCISubscription*, dvoid*, ub4*, dvoid*, ub4);
		};

	} // namespace stream2r
} // namespace axon

#endif