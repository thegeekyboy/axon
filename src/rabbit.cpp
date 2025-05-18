#include <string>
#include <cstring>
#include <sstream>
#include <functional>

#include <axon/util.h>
#include <axon/rabbit.h>

namespace axon
{
	namespace queue
	{
		char *stringify_bytes(amqp_bytes_t bytes, char *p)
		{
			char *res = p;
			uint8_t *data = reinterpret_cast<unsigned char *>(bytes.bytes);
			size_t i;

			for (i = 0; i < bytes.len; i++) {
				if (data[i] >= 32 && data[i] != 127) {
					*p++ = data[i];
				} else {
					*p++ = '\\';
					*p++ = '0' + (data[i] >> 6);
					*p++ = '0' + (data[i] >> 3 & 0x7);
					*p++ = '0' + (data[i] & 0x7);
				}
			}

			*p = 0;
			return res;
		}

		rabbit::rabbit(): _id(axon::util::uuid()), _connected(false), _port(0), _channel(1), _socket(&_connection)
		{
		}

		rabbit::~rabbit()
		{
			close();
		}

		void rabbit::connect(std::string hostname, uint16_t port, std::string vhost, std::string username, std::string password)
		{
			DBGPRN("[%s] = %s, %d, %s, %s, %s", __PRETTY_FUNCTION__, hostname.c_str(), port, vhost.c_str(), username.c_str(), password.c_str());
			axon::queue::error re;
			int ec;

			_hostname = hostname;	// check if name ok
			_port = port;			// check if port ok
			_vhost = vhost;

			if ((ec = amqp_socket_open(_socket.get(), _hostname.c_str(), _port)) != AMQP_STATUS_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot open socket to host - " + axon::queue::error::message(ec));

			if ((re = amqp_login(_connection.get(), _vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username.c_str(), password.c_str())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "login issue - " + re.what());

			amqp_channel_open(_connection.get(), _channel);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot open channel - " + re.what());

			_connected = true;
		}

		void rabbit::close()
		{
			axon::queue::error re;

			if (_connected && ((re = amqp_channel_close(_connection.get(), _channel, AMQP_REPLY_SUCCESS)) != AMQP_RESPONSE_NORMAL))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot close connection - " + re.what());

			_connected = false;
		}

		void rabbit::clear(std::string queue)
		{
			DBGPRN("[%s] = %s", __PRETTY_FUNCTION__, queue.c_str());
			axon::queue::error re;

			[[maybe_unused]] amqp_queue_purge_ok_t* response = amqp_queue_purge(_connection.get(), _channel, amqp_cstring_bytes(queue.c_str()));

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot purge queue - " + re.what());
		}

		void rabbit::clear()
		{
			for (std::string &queue : _queue)
				clear(queue)
;		}

		void rabbit::make_queue(std::string queue)
		{
			DBGPRN("[%s] = %s", __PRETTY_FUNCTION__, queue.c_str());
			axon::queue::error re;

			[[maybe_unused]] amqp_queue_declare_ok_t* response = amqp_queue_declare(
				_connection.get(),
				_channel,
				amqp_cstring_bytes(queue.c_str()), 
				0,									// passive
				0,									// durable
				0,									// exclusive
				0,									// auto_delete
				amqp_empty_table					// arguments
			);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create queue - " + re.what());

			// return qname(response);
		}

		void rabbit::delete_queue(std::string queue)
		{
			DBGPRN("[%s] = %s", __PRETTY_FUNCTION__, queue.c_str());
			axon::queue::error re;

			amqp_queue_delete_ok_t *reply = amqp_queue_delete(
				_connection.get(),
				_channel,
				amqp_cstring_bytes(queue.c_str()), 
				0,									// do not delete unless queue is unused
				0									// do not delete unless queue is empty
			);

			if (reply == NULL && (re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot create queue - " + re.what());
		}

		void rabbit::make_exchange(std::string exchange)
		{
			[[maybe_unused]] amqp_exchange_declare_ok_t* response = amqp_exchange_declare (
				_connection.get(),
				_channel,									// channel
				amqp_cstring_bytes (exchange.c_str()),		// exchange
				amqp_cstring_bytes("direct"),				// type: direct, fanout, topic, headers
				0,											// passive
				0,											// durable
#if defined(AMQP_VERSION) && AMQP_VERSION >= 0x00060000
				0,											// auto delete
				0,											// internal
#endif
				amqp_empty_table);							// arguments
		}

		void rabbit::bind(std::string queue, std::string exchange, std::string bindkey)
		{
			DBGPRN("[%s] = %s, %s, %s", __PRETTY_FUNCTION__, queue.c_str(), exchange.c_str(), bindkey.c_str());

			axon::queue::error re;
			amqp_bytes_t queue_, exchange_, bindkey_;

			queue_ = amqp_cstring_bytes(const_cast<char*>(queue.c_str()));
			exchange_ = amqp_cstring_bytes(const_cast<char*>(exchange.c_str()));
			bindkey_ = amqp_cstring_bytes(const_cast<char*>(bindkey.c_str()));

			amqp_queue_bind(
				_connection.get(),
				_channel,
				queue_,
				exchange_,
				bindkey_,
				amqp_empty_table
			);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot bind to queue - " + re.what());
		}

		void producer::push(std::string exchange, std::string route, char *message, size_t size, axon::queue::encoding encode)
		{
			DBGPRN("[%s] = %s, %s", __PRETTY_FUNCTION__, exchange.c_str(), route.c_str());
			amqp_bytes_t msg, exchange_, route_;
			amqp_basic_properties_t props;

			props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
			props.delivery_mode = 2; // persistent delivery mode

			switch (encode)
			{
				case axon::queue::encoding::text:
					props.content_type = amqp_cstring_bytes("text/plain");
					break;

				case axon::queue::encoding::binary:
					props.content_type = amqp_cstring_bytes("application/octet-stream");
					break;

				default:
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid encoding");
			}

			msg.len = size;
			msg.bytes = message;

			exchange_ = amqp_cstring_bytes(const_cast<char*>(exchange.c_str()));
			route_ = amqp_cstring_bytes(const_cast<char*>(route.c_str()));

			amqp_basic_publish(
				_connection.get(),
				_channel,
				exchange_,
				route_,
				0,
				0,
				&props,
				msg
			);

		}

		void producer::push(std::string exchange, std::string route, std::string message)
		{
			push(exchange, route, const_cast<char*>(message.c_str()), message.size(), axon::queue::encoding::text);
		}

		void consumer::attach(std::string queue, std::string exchange, std::string bindkey)
		{
			DBGPRN("[%s] = %s, %s, %s", __PRETTY_FUNCTION__, queue.c_str(), exchange.c_str(), bindkey.c_str());

			axon::queue::error re;
			amqp_bytes_t queue_, exchange_, bindkey_;

			queue_ = amqp_cstring_bytes(const_cast<char*>(queue.c_str()));
			exchange_ = amqp_cstring_bytes(const_cast<char*>(exchange.c_str()));
			bindkey_ = amqp_cstring_bytes(const_cast<char*>(bindkey.c_str()));

			amqp_queue_bind(
				_connection.get(),
				_channel,
				queue_,
				exchange_,
				bindkey_,
				amqp_empty_table
			);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot bind to queue - " + re.what());

			amqp_basic_consume(
				_connection.get(),
				_channel,
				queue_,				// queue
				amqp_empty_bytes,	// tag
				0,					// no Local
				1,					// no Ack
				0,					// exclusive
				amqp_empty_table
			);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot attach consumer - " + re.what());

			_enabled = true;
		}

		struct envelope consumer::pop()
		{
			if (!_enabled)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer not attached");

			axon::queue::error re;
			amqp_envelope_t packet;
			struct timeval timeout;

			timeout.tv_sec = 1;
			timeout.tv_usec = 1000;

			struct envelope env;

			amqp_maybe_release_buffers(_connection.get());

			if ((re = amqp_consume_message(_connection.get(), &packet, &timeout, 0)) != AMQP_RESPONSE_NORMAL && re != AMQP_STATUS_TIMEOUT)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, std::to_string(_channel) + "error while consuming message " + re.what());

			if (packet.message.body.len > 0)
			{
				unsigned char *buf = reinterpret_cast<unsigned char *>(packet.message.body.bytes);
				char cstr[64];

				env.size = packet.message.body.len;
				stringify_bytes(packet.message.properties.content_type, cstr);

				if (strcmp(cstr, "application/octet-stream") == 0)
					env.enc = axon::queue::encoding::binary;
				else if (strcmp(cstr, "text/plain") == 0)
					env.enc = axon::queue::encoding::text;

				for (size_t i = 0; i < packet.message.body.len; i++) env.ss<<buf[i];
			}

			amqp_destroy_envelope(&packet);
			return env;
		}

		struct envelope consumer::get(std::string queue)
		{
			axon::queue::error re;
			envelope env{};

			if ((re = amqp_basic_get(_connection.get(), _channel, amqp_cstring_bytes(queue.c_str()), 1)) == AMQP_RESPONSE_NORMAL && re == AMQP_BASIC_GET_EMPTY_METHOD)
				return env;

			amqp_message_t message;

			if ((re = amqp_read_message(_connection.get(), _channel, &message, 0)) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "error while reading message " + re.what());

			if (message.body.len > 0)
			{
				unsigned char *buf = reinterpret_cast<unsigned char *>(message.body.bytes);
				char cstr[64];

				env.size = message.body.len;
				stringify_bytes(message.properties.content_type, cstr);

				if (strcmp(cstr, "application/octet-stream") == 0)
					env.enc = axon::queue::encoding::binary;
				else if (strcmp(cstr, "text/plain") == 0)
					env.enc = axon::queue::encoding::text;

				for (size_t i = 0; i < message.body.len; i++) env.ss<<buf[i];
			}

			amqp_destroy_message(&message);

			return env;
		}

		void consumer::start(std::function<void(std::string)> fn)
		{
			while (_enabled)
			{
				struct envelope e = pop();
				if (e.size > 0) fn(e.ss.str());
			}
		}

		void consumer::stop()
		{
			_enabled = false;
		}
	}
}
