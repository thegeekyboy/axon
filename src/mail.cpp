#include <iostream>
#include <fstream>

#include <boost/regex.hpp>
#include <boost/asio/ip/host_name.hpp>

#include <axon.h>
#include <axon/mail.h>
#include <axon/util.h>

namespace axon
{

	mail::mail()
	{
		_connected = false;
		_localhost = boost::asio::ip::host_name();

		_socket.init();
	}

	mail::~mail()
	{
		if (_connected)
		{
			_socket.writeline("QUIT");
			usleep(100000);

			_socket.stop();
			_connected = false;

			_th.join();
		}
	}

	std::string& mail::operator[] (char x)
	{
		switch (x)
		{
			case AXON_MAIL_LOCALHOST:
				return _localhost;
				break;

			case AXON_MAIL_SERVER:
				return _server;
				break;

			case AXON_MAIL_USERNAME:
				return _username;
				break;

			case AXON_MAIL_PASSWORD:
				return _password;
				break;

			case AXON_MAIL_FROM:
				return _from;
				break;
		}

		return _server;
	}

	int& mail::operator[] (int i)
	{
		switch (i)
		{
			case AXON_MAIL_PORT:
				return _port;
				break;
		}

		return _port;
	}

	bool mail::_wait(uint16_t rc)
	{
		usleep(10000);
		if (!_connected)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected to mail server");
		
		while (_socket.alive())
		{
			if (_socket.linewaiting())
			{
				std::string resp = _socket.line();

				if (resp.size())
				{
					std::vector<std::string> tokens = axon::util::split(resp, ' ');
					uint16_t code = std::stoi(tokens[0]);
					
					if (code == SMTP_OK_CONNECTION_CLOSED || rc == SMTP_ERR_CONNECTION_CLOSED || rc == SMTP_ERR_ABORTED_LOCAL_ISSUE)
						return false;
					else if (code == rc)
						return true;
					else
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response - " + resp);
				}
			}
			usleep(10000);
		}

		return false;
	}

	bool mail::connect()
	{
		if (_connected)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already connected to mail server");

		_socket.open(_server, _port);

		_th = std::thread(&axon::transport::tcpip::socks::readline, &_socket);
		
		while (_socket.alive())
		{
			if (_socket.linewaiting())
			{
				std::string resp = _socket.line();

				if (resp.size())
				{
					std::vector<std::string> tokens = axon::util::split(resp, ' ');
					uint16_t code = std::stoi(tokens[0]);
					
					switch (code)
					{
						case SMTP_OK_CONNECTION_CLOSED:
						case SMTP_ERR_CONNECTION_CLOSED:
						case SMTP_ERR_ABORTED_LOCAL_ISSUE:
							return false;
							break;
						
						case SMTP_OK_READY:
							usleep(100000);
							_socket.writeline("HELO " + _localhost);
							continue;
							break;

						case SMTP_OK_COMMAND_SUCCESS:
							_connected = true;
							return true;

						default:
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] Unexpected response (" + std::to_string(code) + ") - " + resp);
							break;
					}
				}
			}
			usleep(10000);
		}

		_connected = true;

		return true;
	}

	void mail::to(std::string value)
	{
		const static boost::regex pattern("^\\w+([-+.']\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$");

		if (!boost::regex_match(value, pattern))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid email address format - " + value);

		_to.push_back(value);
	}

	void mail::cc(std::string value)
	{
		const static boost::regex pattern("^\\w+([-+.']\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$");

		if (!boost::regex_match(value, pattern))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid email address format - " + value);

		_cc.push_back(value);
	}

	void mail::subject(std::string value)
	{
		_subject = value;
	}

	void mail::text(std::string value)
	{
		_simple = value;
	}

	void mail::html(std::string value)
	{
		_html = axon::util::base64_encode(value);
	}

	void mail::html(std::string value, [[maybe_unused]] bool binary)
	{
		// TODO: what if not binary?

		std::ifstream fin(value, std::ios::binary);
		std::ostringstream ostr;
		ostr << fin.rdbuf();;
		_html = axon::util::base64_encode(ostr.str());
	}

	void mail::attach(std::string filename)
	{
		if (!axon::util::isfile(filename))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot attach file, does not exist");

		auto [path, fname] = axon::util::splitpath(filename);

		attach(filename, fname);
	}

	void mail::attach(std::string filename, std::string cid)
	{
		if (!axon::util::isfile(filename))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot attach file, does not exist");

		auto [magic, encoding] = axon::util::magic(filename);

		attach(filename, cid, magic);
	}

	void mail::attach(std::string filename, std::string cid, std::string magic)
	{
		if (!axon::util::isfile(filename))
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot attach file, does not exist");

		struct attachment atch;

		std::ifstream fin(filename, std::ios::binary);
		std::ostringstream ostr;
		ostr << fin.rdbuf();
		// std::string content = axon::util::base64_encode(ostr.str());
		atch.buffer = axon::util::base64_encode(ostr.str());
		atch.cid = cid;
		atch.magic = magic;
		_attachment.push_back(atch);
	}

	bool mail::send()
	{
		std::string final_to, final_cc;
		std::string boundary = axon::util::uuid();
		std::string secondary = axon::util::uuid();

		if (!_connected)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected to mail server");

		if (_simple.size() < 2)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "mail body cannot be empty");

		if (axon::util::count(_to) <= 0 || _from.size() < 2)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "from and/or to cannot be empty");

		_socket.writeline("MAIL FROM: " + _from);
		_wait(SMTP_OK_COMMAND_SUCCESS);

		for (auto &rcpt : _to)
		{
			_socket.writeline("RCPT TO: " + rcpt);
			_wait(SMTP_OK_COMMAND_SUCCESS);

			final_to = final_to + rcpt + ";";
		}

		for (auto &rcpt : _cc)
		{
			_socket.writeline("RCPT TO: " + rcpt);
			_wait(SMTP_OK_COMMAND_SUCCESS);

			final_cc = final_cc + rcpt + ";";
		}

		_socket.writeline("DATA");
		_wait(SMTP_OK_SEND_BODY);

		_socket.writeline("From: " + _from);
		
		if (axon::util::count(_to) > 0)
			_socket.writeline("To: " + final_to);

		if (axon::util::count(_cc) > 0)
			_socket.writeline("Cc: " + final_cc);

		_socket.writeline("Subject: " + _subject);
		_socket.writeline("MIME-Version: 1.0");
		_socket.writeline("Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n");

		_socket.writeline("--" + boundary);
		_socket.writeline("Content-Type: multipart/alternative; boundary=\"" + secondary +"\"\r\n");

		_socket.writeline("--" + secondary + "\r\n");
		_socket.writeline(_simple);

		if (_html.size() > 0)
		{
			_socket.writeline("\r\n--" + secondary);
			_socket.writeline("Content-Type: text/html; charset=\"UTF-8\"");
			_socket.writeline("Content-Description: None exists");
			_socket.writeline("Content-Disposition: inline");
			_socket.writeline("Content-Transfer-Encoding: base64\r\n");
			_socket.writeline(_html);
		}
		
		_socket.writeline("\r\n--" + secondary + "--\r\n");

		for (auto &atch : _attachment)
		{
			_socket.writeline("\r\n--" + boundary);
			_socket.writeline("Content-Type: " + atch.magic + "; charset=\"UTF-8\"");
			_socket.writeline("Content-Description: " + atch.cid);
			_socket.writeline("Content-Disposition: attachment; filename=\"" + atch.cid + "\"");
			_socket.writeline("Content-Transfer-Encoding: base64");
			_socket.writeline("Content-ID: <" + atch.cid + ">\r\n");
			_socket.writeline(atch.buffer);
		}

		_socket.writeline("\r\n--" + boundary);

		_socket.writeline("\r\n.\r\n");
		_wait(SMTP_OK_COMMAND_SUCCESS);

		return true;
	}
}