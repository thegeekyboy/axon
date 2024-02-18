#ifndef AXON_MAIL_H_
#define AXON_MAIL_H_

#include <thread>

#include <axon/socket.h>

#define AXON_MAIL_LOCALHOST 'a'
#define AXON_MAIL_SERVER    'b'
#define AXON_MAIL_USERNAME  'c'
#define AXON_MAIL_PASSWORD  'd'
#define AXON_MAIL_FROM      'e'

#define AXON_MAIL_PORT     0x01

#define SMTP_OK_SYSTEM_STATUS					211
#define SMTP_OK_HELP_MESSAGE					214
#define SMTP_OK_READY							220
#define SMTP_OK_CONNECTION_CLOSED				221
#define SMTP_OK_AUTHENTICATION_SUCCESSFUL		235
#define SMTP_OK_REMOTE_USER						251
#define SMTP_OK_COMMAND_SUCCESS					250
#define SMTP_OK_SECURITY_ACCEPTED				334
#define SMTP_OK_SEND_BODY						354

#define SMTP_ERR_CONNECTION						101
#define SMTP_ERR_USER_VERIFY					252
#define SMTP_ERR_CONNECTION_CLOSED				421
#define SMTP_ERR_MAILBOX_FULL					422
#define SMTP_ERR_TOO_MANY_REQUESTS				431
#define SMTP_ERR_NO_REMOTE_RESPONSE				441
#define SMTP_ERR_CONNECTION_DROPPED				442
#define SMTP_ERR_INTERNAL_LOOP					446
#define SMTP_ERR_ABORTED_MAILBOX_BUSY			450
#define SMTP_ERR_ABORTED_LOCAL_ISSUE			451
#define SMTP_ERR_ABORTED_INSUFFICIENT_STORAGE	452
#define SMTP_ERR_TLS_UNAVAILABLE				454
#define SMTP_ERR_UNACCEPTABLE_PARAMETER			455
#define SMTP_ERR_REJECTED_SPAM					471
#define SMTP_ERR_INVALID_COMMAND				500
#define SMTP_ERR_INVALID_PARAMETER				501
#define SMTP_ERR_COMMAND_NOT_IMPLEMENTED		502
#define SMTP_ERR_IMPROPER_SEQUENCE				503
#define SMTP_ERR_PARAMETER_NOT_IMPLEMENTED		504
#define SMTP_ERR_INVALID_EMAIL_ADDRESS			510
#define SMTP_ERR_DNS_DOMAIN						512
#define SMTP_ERR_MAIL_TOO_LARGE					523
#define SMTP_ERR_USE_TLS						530
#define SMTP_ERR_AUTHENTICATION_FAILED			535
#define SMTP_ERR_ENCRYPTION_REQUIRED			538
#define SMTP_ERR_REJECTED_SPAM2					541
#define SMTP_ERR_MAILBOX_UNAVAILABLE			550
#define SMTP_ERR_USER_NOT_LOCAL					551
#define SMTP_ERR_MAILBOX_FULL2					552
#define SMTP_ERR_INVALID_EMAIL_ADDRESS2			553
#define SMTP_ERR_NO_SMTP						554
#define SMTP_ERR_PARAMETER_NOT_RECOGNIZED		555

namespace axon {

	class mail {

		std::string _id;
		bool _connected;

		std::string _localhost, _server, _username, _password, _from;
		int _port;

		std::vector<std::string> _to, _cc;
		std::string _subject, _simple, _html;

		std::thread _th;
		axon::transport::tcpip::socks _socket;

		struct attachment {
			std::string buffer;
			std::string cid;
			std::string magic;
		};

		std::vector<attachment> _attachment;

		bool _wait(uint16_t);

		public:
			mail();
			~mail();

			std::string& operator[] (char);
			int& operator[] (int);

			bool connect();
			void disconnect();
			void noop();

			void to(std::string);
			void cc(std::string);
			void subject(std::string);
			void text(std::string);
			void html(std::string);
			void html(std::string, bool);
			void attach(std::string);
			void attach(std::string, std::string);
			void attach(std::string, std::string, std::string);

			bool send();
	};
}

#endif