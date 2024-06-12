#include <string>
#include <sstream>

#include <axon/util.h>
#include <axon/rabbit.h>

namespace axon
{
	namespace queue
	{
		rabbit::rabbit(): _id(axon::util::uuid()), _connected(false), _port(0), _channel(1), _socket(&_connection)
		{
		}

		rabbit::~rabbit()
		{
			close();
		}

		void rabbit::connect(std::string hostname, uint16_t port, std::string vhost, std::string username, std::string password)
		{
			axon::queue::error re;
			int ec;

			_hostname = hostname; // check if name ok
			_port = port; // check if port ok
			_vhost = vhost;

			if ((ec = amqp_socket_open(_socket.get(), _hostname.c_str(), _port)) != AMQP_STATUS_OK)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot connect to host - " + axon::queue::error::message(ec));

			if ((re = amqp_login(_connection.get(), _vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username.c_str(), password.c_str())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot connect to host - " + re.what());

			amqp_channel_open(_connection.get(), _channel);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot connect to host - " + re.what());

			_connected = true;
		}

		void rabbit::close()
		{
			axon::queue::error re;

			if (_connected && ((re = amqp_channel_close(_connection.get(), _channel, AMQP_REPLY_SUCCESS)) != AMQP_RESPONSE_NORMAL))
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot close connection - " + re.what());

			_connected = false;
		}

		void producer::push(std::string message, std::string& exchange, std::string& routing)
		{
			amqp_bytes_t msg, xcng, route;
			amqp_basic_properties_t props;

			props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
			props.content_type = amqp_cstring_bytes("text/plain");
			props.delivery_mode = 2; /* persistent delivery mode */

			msg.len = message.size();
			msg.bytes = const_cast<char*>(message.c_str());

			xcng = amqp_cstring_bytes(const_cast<char*>(exchange.c_str()));
			route = amqp_cstring_bytes(const_cast<char*>(routing.c_str()));

			amqp_basic_publish(_connection.get(), _channel, xcng, route, 0, 0, &props, msg);

			// amqp_basic_properties_t props;
			// props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
			// props.content_type = amqp_cstring_bytes("text/plain");
			// props.delivery_mode = 2; /* persistent delivery mode */
			// die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), 0, 0, &props, amqp_cstring_bytes(messagebody)), "Publishing");
		}

		void consumer::attach(std::string queue, std::string exchange, std::string routing)
		{
			axon::queue::error re;
			amqp_bytes_t que, xcng, bndkey;

			que = amqp_cstring_bytes(const_cast<char*>(queue.c_str()));
			xcng = amqp_cstring_bytes(const_cast<char*>(exchange.c_str()));
			bndkey = amqp_cstring_bytes(const_cast<char*>(routing.c_str()));

			amqp_queue_bind(_connection.get(), _channel, que, xcng, bndkey, amqp_empty_table);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot bind to queue - " + re.what());

			amqp_basic_consume(_connection.get(), _channel, que, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);

			if ((re = amqp_get_rpc_reply(_connection.get())) != AMQP_RESPONSE_NORMAL)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot attach consumer - " + re.what());

			_enabled = true;
		}

		void consumer::start(std::function<void(std::string)> fn)
		{
			while (_enabled)
			{
				axon::queue::error re;
				amqp_envelope_t envelope;

				amqp_maybe_release_buffers(_connection.get());

				if ((re = amqp_consume_message(_connection.get(), &envelope, NULL, 0)) != AMQP_RESPONSE_NORMAL)
					break;

				// printf("Delivery %u, exchange %.*s routingkey %.*s\n",
				// 		(unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
				// 		(char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
				// 		(char *)envelope.routing_key.bytes);

				// if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
				// printf("Content-type: %.*s\n",
				// 		(int)envelope.message.properties.content_type.len,
				// 		(char *)envelope.message.properties.content_type.bytes);
				// }
				// printf("----\n");
				// char *buf = char*>(envelope.message.body.bytes);
				std::stringstream ss;
				unsigned char *buf = reinterpret_cast<unsigned char *>(envelope.message.body.bytes);
				for (size_t i = 0; i < envelope.message.body.len; i++)
					ss<<buf[i];
				// printf("---%s---\n", buf);
				fn(ss.str());

				amqp_destroy_envelope(&envelope);
			}
		}

		void consumer::stop()
		{
			_enabled = false;
		}

		void consumer::pop(std::string&)
		{

		}
	}
}