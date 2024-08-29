#ifndef AXON_RABBIT_H_
#define AXON_RABBIT_H_

#include <sstream>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <axon.h>

namespace axon
{
	namespace queue
	{
		enum class encoding:char {
			nothing,
			text,
			binary
		};

		struct envelope {
			std::stringstream ss;
			axon::queue::encoding enc;
			size_t size;

			envelope(): enc(axon::queue::encoding::nothing), size(0) { };
		};

		struct error {

			amqp_rpc_reply_t _reply;

			error() = default;
			~error() = default;
			error(const error&) = delete;

			error& operator=(const amqp_rpc_reply_t& right) {

				_reply = right;

				return *this;
			}

			bool operator==(const amqp_response_type_enum& right)
			{
				return (_reply.reply_type == right);
			}

			bool operator!=(const amqp_response_type_enum& right)
			{
				return (_reply.reply_type != right);
			}

			bool operator==(const amqp_status_enum& right)
			{
				return (_reply.library_error == right);
			}

			bool operator!=(const amqp_status_enum& right)
			{
				return (_reply.library_error != right);
			}

			bool operator==(const amqp_method_number_t& right)
			{
				return (_reply.reply.id == right);
			}

			bool operator!=(const amqp_method_number_t& right)
			{
				return (_reply.reply.id != right);
			}

			std::string what() { return message(_reply); }
			static std::string message(const int errorno) { return amqp_error_string2(errorno); };
			static std::string message(const amqp_rpc_reply_t& reply)
			{
				DBGPRN("%s - %d", __PRETTY_FUNCTION__, reply.reply_type);
				std::stringstream ss;

				switch (reply.reply_type)
				{
					case AMQP_RESPONSE_NORMAL:
						return ss.str();

					case AMQP_RESPONSE_NONE:
						ss<<"missing RPC reply type!";
						break;

					case AMQP_RESPONSE_LIBRARY_EXCEPTION:
						ss<<reply.reply_type<<": "<<amqp_error_string2(reply.library_error);
						break;

					case AMQP_RESPONSE_SERVER_EXCEPTION:
						switch (reply.reply.id)
						{
							case AMQP_CONNECTION_CLOSE_METHOD:
							{
								amqp_connection_close_t *m = (amqp_connection_close_t *) reply.reply.decoded;
								ss<<"server connection error "<<m->reply_code<<"h, message: "<<reinterpret_cast<char*>(m->reply_text.bytes);
								break;
							}
							case AMQP_CHANNEL_CLOSE_METHOD:
							{
								amqp_channel_close_t *m = (amqp_channel_close_t *)reply.reply.decoded;
								ss<<"server channel error "<<m->reply_code<<"h, message: "<<reinterpret_cast<char*>(m->reply_text.bytes);
								break;
							}
							default:
								ss<<"unknown server error, method id 0x"<<reply.reply.id;
								break;
						}
						break;
				}

				return ss.str();
			}
		};

		struct connection {
			connection(): _connection(NULL) {
				_connection = amqp_new_connection();
			}

			~connection() {
				int ec;
				if ((ec = amqp_destroy_connection(_connection)) != AMQP_STATUS_OK)
				{
					DBGPRN("%s", axon::queue::error::message(ec).c_str());
				}
			}
			amqp_connection_state_t get() {
				if (_connection == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "connection not initialized");
				return _connection;
			}
			operator amqp_connection_state_t() { return get(); }
			private:
				amqp_connection_state_t _connection;
		};

		struct socket {
			
			socket(axon::queue::connection *conn):_socket(NULL), _connection(conn) {
				if (!(_socket = amqp_tcp_socket_new(_connection->get())))
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "could not create socket.");
			}

			~socket() {
				axon::queue::error re;

				if ((re = amqp_connection_close(_connection->get(), AMQP_REPLY_SUCCESS)) != AMQP_RESPONSE_NORMAL)
				{
					DBGPRN("%s", re.what().c_str());
				}
			}
			amqp_socket_t *get() {

				if (_socket == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "socket not initialized");
				return _socket;
			}
			operator amqp_socket_t*() { return get(); }
			private:
				amqp_socket_t *_socket;
				axon::queue::connection *_connection;
		};
		

		class rabbit {

			std::string _id, _vhost, _hostname;
			bool _connected;
			uint16_t _port;
			std::vector<std::string> _queue;

			protected:
				uint16_t _channel;
				axon::queue::connection _connection;
				axon::queue::socket _socket;

			public:
				rabbit();
				~rabbit();

				void connect(std::string, uint16_t, std::string, std::string, std::string);
				void close();

				void make_queue(std::string);
				void delete_queue(std::string);

				void make_exchange(std::string);

				size_t count() { return 0; }
				void clear(std::string);
				void clear();
		};

		class producer: public rabbit {

			public:
				producer() = default;
				~producer() = default;

				void push(std::string, std::string, char *, size_t, axon::queue::encoding);
				void push(std::string, std::string, std::string);
		};

		class consumer: public rabbit {

			bool _enabled;

			public:
				consumer(): _enabled(false) { };
				~consumer() = default;

				void attach(std::string, std::string, std::string);
				void start(std::function<void(std::string)>);
				void stop();

				struct envelope pop();
				struct envelope get(std::string);
		};
	}
}

#endif