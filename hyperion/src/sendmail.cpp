#include <fstream>

#include <main.h>
#include <sendmail.h>

namespace hyperion {

	std::string& sendmail::operator[] (char x)
	{
		switch (x)
		{
			case SENTINEL_SENDMAIL_SERVER:
				return _server;
				break;

			case SENTINEL_SENDMAIL_USERNAME:
				return _username;
				break;

			case SENTINEL_SENDMAIL_PASSWORD:
				return _password;
				break;

			case SENTINEL_SENDMAIL_TO:
				return _to;
				break;

			case SENTINEL_SENDMAIL_FROM:
				return _from;
				break;

			case SENTINEL_SENDMAIL_CC:
				return _cc;
				break;
			
			case SENTINEL_SENDMAIL_BODY:
				return _body;
				break;
			
			case SENTINEL_SENDMAIL_SUBJECT:
				return _subject;
				break;

			case SENTINEL_SENDMAIL_TEMPLATE:
				return _template;
				break;

			case SENTINEL_SENDMAIL_LOGO:
				return _logo;
				break;
		}

		return _server;
	}

	int& sendmail::operator[] (int i)
	{
		switch (i)
		{
			case SENTINEL_SENDMAIL_PORT:
				return _port;
				break;
		}

		return _port;
	}

	size_t sendmail::callback(char *ptr, size_t size, size_t nmemb, void *userp)
	{
		struct upload_status *upload_ctx = (struct upload_status *)userp;
		const char *data;
		size_t room = size * nmemb;
	
		if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
			return 0;
	
		data = &(upload_ctx->payload_text[upload_ctx->bytes_read]);
	
		if (data)
		{
			size_t len = strlen(data);
			
			if(room < len)
				len = room;
			
			memcpy(ptr, data, len);
			upload_ctx->bytes_read += len;
		
			return len;
		}
	
		return 0;
	}

	std::string sendmail::build()
	{
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		std::string msgid = boost::uuids::to_string(uuid) + "@hyperion.binutil.com";

		auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		std::stringstream ss;

		ss<<"Date: "<<std::put_time(std::localtime(&in_time_t), "%a, %d %b %Y %X %z")<<"\r\n";
  		ss<<"To: "<<_to<<"\r\n";
		ss<<"From: "<<_from<<"\r\n";
		ss<<"Cc: "<<_cc<<"\r\n";
  		ss<<"Message-ID: "<<msgid<<"\r\n";
  		ss<<"Subject: "<<_subject<<"\r\n";
  		ss<<"\r\n"; /* empty line to divide headers from body, see RFC5322 */
  		ss<<_body<<"\r\n";

		return ss.str();
	}

	bool sendmail::send()
	{
		CURL *curl;
		CURLcode res = CURLE_OK;

		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		std::string msgid = boost::uuids::to_string(uuid) + "@hyperion.binutil.com";

		auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		std::stringstream ss;

		static const char inline_text[] =
			"This email contains MIME Encoded components\r\n"
			"\r\n"
			"Please use a MIME-Compatible client to view this email\r\n";

		std::ifstream file(_template);
		std::string htmlbody((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		boost::replace_all(htmlbody, "${BODY}", _body);
		boost::replace_all(htmlbody, "${SUBJECT}", _subject);

		ss<<"Date: "<<std::put_time(std::localtime(&in_time_t), "%a, %d %b %Y %X %z")<<"\n";
		ss<<"To: "<<_to<<"\n";
		ss<<"From: "<<_from<<"\n";
		ss<<"Message-ID: <"<<msgid<<">\n";
		ss<<"Subject: "<<_subject<<"\n";

		std::string token;

		if ((curl = curl_easy_init()))
		{
			struct curl_slist *headers = NULL;
			struct curl_slist *recipients = NULL;
			struct curl_slist *slist = NULL;

			curl_mime *mime;
			curl_mime *alt;
			curl_mimepart *part;

			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(curl, CURLOPT_URL, _server.c_str());

			curl_easy_setopt(curl, CURLOPT_MAIL_FROM, _from.c_str());
			recipients = curl_slist_append(recipients, _to.c_str());
			if (_cc.size() > 3) recipients = curl_slist_append(recipients, _cc.c_str());
			curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

			/* Build and set the message header list. */
			while (std::getline(ss, token, '\n'))
				headers = curl_slist_append(headers, token.data());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

			mime = curl_mime_init(curl);

			/* The inline part is an alternative proposing the html and the text
			versions of the email. */
			alt = curl_mime_init(curl);

			/* add image */
			part = curl_mime_addpart(alt);
			curl_mime_filedata(part, _logo.c_str());
			curl_mime_type(part, "image/png");
			curl_mime_name(part, "bkash-orange.png");

			/* HTML message. */
			part = curl_mime_addpart(alt);
			curl_mime_data(part, htmlbody.c_str(), CURL_ZERO_TERMINATED);
			curl_mime_type(part, "text/html");

			/* Text message. */
			part = curl_mime_addpart(alt);
			curl_mime_data(part, inline_text, CURL_ZERO_TERMINATED);

			// /* Create the inline part. */
			part = curl_mime_addpart(mime);
			curl_mime_subparts(part, alt);
			curl_mime_type(part, "multipart/alternative");
			slist = curl_slist_append(NULL, "Content-Disposition: inline");
			curl_mime_headers(part, slist, 1);


			// /* Add the current source program as an attachment. */
			curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

			// curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

			/* Send the message */
			res = curl_easy_perform(curl);

			/* Check for errors */
			if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

			/* Free lists. */
			curl_slist_free_all(recipients);
			curl_slist_free_all(headers);

			/* curl will not send the QUIT command until you call cleanup, so you
			* should be able to re-use this connection for additional messages
			* (setting CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and
			* calling curl_easy_perform() again. It may not be a good idea to keep
			* the connection open for a very long time though (more than a few
			* minutes may result in the server timing out the connection), and you do
			* want to clean up in the end.
			*/
			curl_easy_cleanup(curl);

			/* Free multipart message. */
			curl_mime_free(mime);
		}

		return true;
	}

	bool sendmail::send_ex()
	{
		std::string body = build();

		CURL *curl;
		CURLcode res = CURLE_OK;
		struct curl_slist *recipients = NULL;
		struct upload_status upload_ctx;

		upload_ctx.payload_text = body.data();

		curl = curl_easy_init();

		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

			curl_easy_setopt(curl, CURLOPT_URL, _server.c_str());

			curl_easy_setopt(curl, CURLOPT_MAIL_FROM, _from.c_str());

			recipients = curl_slist_append(recipients, _to.c_str());
			recipients = curl_slist_append(recipients, _cc.c_str());
			curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

			curl_easy_setopt(curl, CURLOPT_READFUNCTION, sendmail::callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

			res = curl_easy_perform(curl);

			if(res != CURLE_OK)
      			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "There was an error sending email: "+std::string(curl_easy_strerror(res)));

			curl_slist_free_all(recipients);
			curl_easy_cleanup(curl);
		}

		return true;
	}
}