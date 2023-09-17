#ifndef AXON_URI_H_
#define AXON_URI_H_

#include <string>
#include <algorithm>

#include <boost/algorithm/string.hpp>

/*
	repurposed from https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

	limitations: cannot parse uri with username & password

	axon::util::uri u0 = axon::util::uri::parse("http://localhost:80/foo.html?&q=1:2:3");
	axon::util::uri u1 = axon::util::uri::parse("https://localhost:80/foo.html?&q=1");
	axon::util::uri u2 = axon::util::uri::parse("localhost/foo");
	axon::util::uri u3 = axon::util::uri::parse("https://localhost/foo");
	axon::util::uri u4 = axon::util::uri::parse("localhost:8080");
	axon::util::uri u5 = axon::util::uri::parse("localhost?&foo=1");
	axon::util::uri u6 = axon::util::uri::parse("localhost?&foo=1:2:3");
*/

namespace axon {

	namespace util {

		struct uri
		{
			public:
			std::string QueryString, path, filename, protocol, host, port;

			static uri parse(const std::string &uri_)
			{
				uri result;

				typedef std::string::const_iterator iterator_t;

				if (uri_.length() == 0)
					return result;

				iterator_t uriEnd = uri_.end();

				// get query start
				iterator_t queryStart = std::find(uri_.begin(), uriEnd, '?');

				// protocol
				iterator_t protocolStart = uri_.begin();
				iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');			//"://");

				if (protocolEnd != uriEnd)
				{
					std::string prot = &*(protocolEnd);
					if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
					{
						result.protocol = std::string(protocolStart, protocolEnd);
						boost::algorithm::to_lower(result.protocol);
						protocolEnd += 3;   //	  ://
					}
					else
						protocolEnd = uri_.begin();  // no protocol
				}
				else
					protocolEnd = uri_.begin();  // no protocol

				// host
				iterator_t hostStart = protocolEnd;
				iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

				iterator_t hostEnd = std::find(protocolEnd, 
					(pathStart != uriEnd) ? pathStart : queryStart,	L':');  // check for port

				result.host = std::string(hostStart, hostEnd);
				boost::trim(result.host);

				// port
				if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
				{
					hostEnd++;
					iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
					result.port = std::string(hostEnd, portEnd);
					boost::trim(result.port);
				}

				// path
				if (pathStart != uriEnd)
				{
					result.path = std::string(pathStart, queryStart);
					boost::trim(result.path);
					result.filename = result.path.substr(result.path.find_last_of("/\\") + 1);
					boost::trim(result.filename);
				}

				// query
				if (queryStart != uriEnd)
					result.QueryString = std::string(queryStart, uri_.end());

				return result;

			}   // Parse
		};  // uri
	}
}
#endif