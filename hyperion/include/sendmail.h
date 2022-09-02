#ifndef SENTINEL_SENDMAIL_H_
#define SENTINEL_SENDMAIL_H_

#include <axon.h>

#include <iomanip>
#include <chrono>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>

#include <curl/curl.h>

#define SENTINEL_SENDMAIL_SERVER 's'
#define SENTINEL_SENDMAIL_USERNAME 'u'
#define SENTINEL_SENDMAIL_PASSWORD 'p'
#define SENTINEL_SENDMAIL_FROM 'f'
#define SENTINEL_SENDMAIL_TO 't'
#define SENTINEL_SENDMAIL_CC 'c'
#define SENTINEL_SENDMAIL_BODY 'b'
#define SENTINEL_SENDMAIL_SUBJECT 'j'

#define SENTINEL_SENDMAIL_TEMPLATE 'T'
#define SENTINEL_SENDMAIL_LOGO 'l'

#define SENTINEL_SENDMAIL_PORT 1

namespace hyperion {

	struct upload_status {
		size_t bytes_read;
		char *payload_text;

		upload_status() {
			bytes_read = 0;
			payload_text = NULL;
		}
	};

	class sendmail {

		std::string _server, _username, _password;
		std::string _to, _from, _cc;
		std::string _body, _subject;
		std::string _template, _title, _logo;

		int _port;

		std::string build();

		public:
			// sendmail();
			// ~sendmail();

			static size_t callback(char *, size_t, size_t, void *);
			bool send();
			bool send_ex();

			std::string& operator[] (char);
			int& operator[] (int);
	};
}


#endif