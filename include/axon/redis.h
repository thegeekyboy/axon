#ifndef AXON_REDIS_H_
#define AXON_REDIS_H_

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <ctime>
#include <array>
#include <optional>

#include <hiredis/hiredis.h>

#include <axon.h>

namespace axon
{
	namespace cache
	{
		class reply
		{
			redisReply *_r;

		public:
			reply() = delete;
			explicit reply(redisReply *r) : _r(r) {}
			~reply() { if (_r) freeReplyObject(_r); }

			reply(const reply &) = delete;
			reply &operator=(const reply &) = delete;

			reply(reply &&other) noexcept : _r(other._r) { other._r = nullptr; }
			reply &operator=(reply &&other) noexcept
			{
				if (this != &other)
				{
					if (_r) freeReplyObject(_r);
					_r = other._r;
					other._r = nullptr;
				}
				return *this;
			}

			/* pointer access */
			redisReply *get()  const { return _r; }
			redisReply *operator->() const { return _r; }

			bool ok() const { return _r && _r->type != REDIS_REPLY_ERROR; }
			bool is_null() const { return !_r || _r->type == REDIS_REPLY_NIL; }

			std::string error() const
			{
				if (_r && _r->type == REDIS_REPLY_ERROR)
					return std::string(_r->str, _r->len);
				return {};
			}
		};

		/* ─────────────────────────────────────────────────────────────
			redis  — main connection class

			Follows the same style as axon::database::scylladb and
			axon::stream::kafka:
			• constructor connects, destructor cleans up
			• throws axon::exception on errors
			• non-copyable, moveable
		───────────────────────────────────────────────────────────── */
		class redis
		{
			redisContext *_ctx { nullptr };
			std::string _host;
			int _port;
			int _timeout_ms;
			bool _connected { false };

			void _append(const char *, ...);
			axon::cache::reply _collect();
			axon::cache::reply _command(const char *, ...);

		public:
			/* ── construction / connection ─────────────────────────── */
			redis();
			redis(std::string_view, int, int = 500);
			~redis();

			redis(const redis &) = delete;
			redis &operator=(const redis &) = delete;

			bool connect();
			bool connect(std::string_view, int, int = 500);
			bool login(std::string_view, std::string_view);
			bool close();
			bool ping();

			bool connected() const { return _connected; }
			std::string host() const { return _host; }
			int port() const { return _port; }

			// generic key operations
			bool exists(std::string_view);
			bool del(std::string_view);
			bool expire(std::string_view, int);
			long long ttl(std::string_view);

			// string (GET / SET / INCR)
			bool set(std::string_view, std::string_view, int = 0);
			std::optional<std::string> get(std::string_view);
			long long incr(std::string_view, long long = 1);

			// hash (HGET / HSET / HINCRBY / HGETALL)
			bool hset(std::string_view, std::string_view, std::string_view);
			std::optional<std::string> hget(std::string_view, std::string_view);

			long long hincrby(std::string_view, std::string_view, long long);
			double hincrbyfloat(std::string_view, std::string_view, double);

			// Returns field→value pairs for all fields in the hash
			std::vector<std::pair<std::string, std::string>> hgetall(std::string_view);

			/* ── pipeline support ──────────────────────────────────── */

			/*  Pipeline lets you send N commands and collect N replies
				in a single TCP round trip.

				Usage:
					redis r("127.0.0.1", 6379);
					r.pipeline_begin();
					r.pipeline_hincrby("entity:123:p2p:2026041816", "cnt", 1);
					r.pipeline_hincrby("entity:123:p2p:2026041816", "sum", 5000);
					r.pipeline_expire("entity:123:p2p:2026041816", 90000);
					r.pipeline_commit(); // one round trip
			*/
			void pipeline_begin();
			void pipeline_hincrby(std::string_view, std::string_view, long long);
			void pipeline_hincrbyfloat(std::string_view, std::string_view, double);
			void pipeline_hset(std::string_view, std::string_view, std::string_view);
			void pipeline_hgetall(std::string_view);
			void pipeline_expire(std::string_view, int);
			void pipeline_commit(); /* flush + discard all replies */
			std::vector<axon::cache::reply> pipeline_run();

		private:
			int _pipeline_depth { 0 };
		};

	} // namespace cache
} // namespace axon

#endif // AXON_REDIS_H_