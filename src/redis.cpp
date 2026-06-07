#include <cstdarg>
#include <cstring>
#include <cstdio>

#include <axon.h>
#include <axon/util.h>
#include <axon/redis.h>

namespace axon
{
	namespace cache
	{
		axon::cache::reply redis::_command(const char *fmt, ...)
		{
			if (!_ctx || !_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: not connected");

			va_list ap;
			va_start(ap, fmt);
			redisReply *r = static_cast<redisReply *>(redisvCommand(_ctx, fmt, ap));
			va_end(ap);

			if (!r)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: null axon::cache::reply — connection lost? (%s)", _ctx->errstr);

			if (r->type == REDIS_REPLY_ERROR)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: command error: %s", r->str);

			return axon::cache::reply(r);
		}

		void redis::_append(const char *fmt, ...)
		{
			if (!_ctx || !_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: not connected");

			va_list ap;
			va_start(ap, fmt);
			redisvAppendCommand(_ctx, fmt, ap);
			va_end(ap);

			_pipeline_depth++;
		}

		axon::cache::reply redis::_collect()
		{
			if (!_ctx || !_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: not connected");

			redisReply *r = nullptr;
			if (redisGetReply(_ctx, reinterpret_cast<void **>(&r)) != REDIS_OK || !r)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: failed to collect pipelined reply: %s", _ctx->errstr);

			return axon::cache::reply(r);
		}

		redis::redis() {}

		redis::redis(std::string_view host, int port, int timeout_ms): _host(host), _port(port), _timeout_ms(timeout_ms)
		{
			connect();
		}

		redis::~redis()
		{
			close();
		}

		bool redis::connect()
		{
			if (_host.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "host not set — use connect(host, port) or the parameterised constructor");

			return connect(_host, _port, _timeout_ms);
		}

		bool redis::connect(std::string_view host, int port, int timeout_ms)
		{
			close();

			_host = host;
			_port = port;
			_timeout_ms = timeout_ms;

			timeval tv { 0, timeout_ms * 1000 };
			_ctx = redisConnectWithTimeout(_host.c_str(), _port, tv);

			if (!_ctx)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: failed to allocate context for %s:%d", _host.c_str(), _port);

			if (_ctx->err)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: connection to %s:%d failed: %s", _host.c_str(), _port, _ctx->errstr);

			_connected = true;
			INFPRN("redis: connected to %s:%d", _host.c_str(), _port);

			return true;
		}

		bool redis::login(std::string_view username, std::string_view password)
		{
			if (!_ctx || !_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "redis: not connected");

			auto r = _command("AUTH %b %b", username.data(), username.size(), password.data(), password.size());
			return r.ok() && r->type == REDIS_REPLY_INTEGER && r->integer > 0;
		}

		bool redis::close()
		{
			if (_ctx)
			{
				redisFree(_ctx);
				_ctx = nullptr;
			}
			_connected    = false;
			_pipeline_depth = 0;
			return true;
		}

		bool redis::ping()
		{
			auto r = _command("PING");
			return r.ok() && _ctx && !_ctx->err;
		}

		bool redis::exists(std::string_view key)
		{
			auto r = _command("EXISTS %b", key.data(), key.size());
			return r.ok() && r->type == REDIS_REPLY_INTEGER && r->integer > 0;
		}

		bool redis::del(std::string_view key)
		{
			auto r = _command("DEL %b", key.data(), key.size());
			return r.ok() && r->integer > 0;
		}

		bool redis::expire(std::string_view key, int seconds)
		{
			auto r = _command("EXPIRE %b %d", key.data(), key.size(), seconds);
			return r.ok() && r->integer == 1;
		}

		long long redis::ttl(std::string_view key)
		{
			auto r = _command("TTL %b", key.data(), key.size());
			return r.ok() ? r->integer : -2LL;
		}

		bool redis::set(std::string_view key, std::string_view value, int ex)
		{
			axon::cache::reply r = ex > 0
				? _command("SET %b %b EX %d", key.data(), key.size(), value.data(), value.size(), ex)
				: _command("SET %b %b", key.data(), key.size(), value.data(), value.size());
			return r.ok();
		}

		std::optional<std::string> redis::get(std::string_view key)
		{
			auto r = _command("GET %b", key.data(), key.size());
			if (r.is_null()) return std::nullopt;
			return std::string(r->str, r->len);
		}

		long long redis::incr(std::string_view key, long long by)
		{
			auto r = _command("INCRBY %b %lld", key.data(), key.size(), by);
			return r.ok() ? r->integer : 0LL;
		}

		bool redis::hset(std::string_view key, std::string_view field, std::string_view value)
		{
			auto r = _command("HSET %b %b %b",
				key.data(),   key.size(),
				field.data(), field.size(),
				value.data(), value.size());
			return r.ok();
		}

		std::optional<std::string> redis::hget(std::string_view key, std::string_view field)
		{
			auto r = _command("HGET %b %b", key.data(), key.size(), field.data(), field.size());
			if (r.is_null()) return std::nullopt;

			return std::string(r->str, r->len);
		}

		long long redis::hincrby(std::string_view key, std::string_view field, long long by)
		{
			auto r = _command("HINCRBY %b %b %lld",
				key.data(),   key.size(),
				field.data(), field.size(),
				by);

			return r.ok() ? r->integer : 0LL;
		}

		double redis::hincrbyfloat(std::string_view key, std::string_view field, double by)
		{
			auto r = _command("HINCRBYFLOAT %b %b %f",
				key.data(),   key.size(),
				field.data(), field.size(),
				by);

			if (!r.ok() || r->type != REDIS_REPLY_STRING)
				return 0.0;

			return std::stod(std::string(r->str, r->len));
		}

		std::vector<std::pair<std::string, std::string>> redis::hgetall(std::string_view key)
		{
			std::vector<std::pair<std::string, std::string>> result;

			auto r = _command("HGETALL %b", key.data(), key.size());
			if (!r.ok() || r->type != REDIS_REPLY_ARRAY)
				return result;

			/* HGETALL returns [field, value, field, value, ...] */
			for (size_t i = 0; i + 1 < r->elements; i += 2)
			{
				std::string f(r->element[i]->str, r->element[i]->len);
				std::string v(r->element[i+1]->str, r->element[i+1]->len);
				result.emplace_back(std::move(f), std::move(v));
			}

			return result;
		}

		void redis::pipeline_begin()
		{
			if (_pipeline_depth > 0)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__,
					"redis: pipeline_begin() called while a pipeline is already open "
					"(%d commands pending) — call pipeline_commit() first", _pipeline_depth);
		}

		void redis::pipeline_hincrby(std::string_view key, std::string_view field, long long by)
		{
			_append("HINCRBY %b %b %lld",
				key.data(),   key.size(),
				field.data(), field.size(),
				by);
		}

		void redis::pipeline_hincrbyfloat(std::string_view key, std::string_view field, double by)
		{
			_append("HINCRBYFLOAT %b %b %f",
				key.data(),   key.size(),
				field.data(), field.size(),
				by);
		}

		void redis::pipeline_hset(std::string_view key, std::string_view field, std::string_view value)
		{
			_append("HSET %b %b %b",
				key.data(),   key.size(),
				field.data(), field.size(),
				value.data(), value.size());
		}

		void redis::pipeline_hgetall(std::string_view key)
		{
			_append("HGETALL %b", key.data(), key.size());
		}

		void redis::pipeline_expire(std::string_view key, int seconds)
		{
			_append("EXPIRE %b %d", key.data(), key.size(), seconds);
		}
/*
		void redis::pipeline_commit()
		{
			for (int i = 0; i < _pipeline_depth; ++i)
			{
				auto r = _collect();

				// replies are discarded — errors are silently swallowed
				// to match fire-and-forget pipeline semantics.
				// If you need per-command error checking, use the
				// non-pipeline methods instead.
				(void) r;
			}
			_pipeline_depth = 0;
		}
*/
		std::vector<axon::cache::reply> redis::pipeline_run()
		{
			std::vector<reply> replystack;
			replystack.reserve(_pipeline_depth);

			for (int i = 0; i < _pipeline_depth; ++i)
				replystack.push_back(_collect());

			_pipeline_depth = 0;
			return replystack;
		}

	} // namespace cache
} // namespace axon