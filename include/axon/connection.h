#ifndef AXON_CONNECTION_H_
#define AXON_CONNECTION_H_

#include <boost/regex.hpp>
#include <aws/core/Aws.h>

#include <axon/util.h>

#define AXON_TRANSFER_CONNECTION_PORT 0x01

namespace axon
{
	enum class flags: int8_t {

		UNKNOWN = -1,
		DIR = 0,
		FILE = 1,
		LINK = 2,
		CHAR = 3,
		BLOCK = 4,
		FIFO = 5,
		SOCKET = 6
	};

	enum class protocol: int8_t {

		UNKNOWN = -1,
		NOTHING = 0, // done
		FILE = 1, // done
		SFTP = 2, // done
		FTP = 3, // done
		S3 = 4, // done
		SAMBA = 5,
		HDFS = 6, // done
		AWS = 7, // done
		SCP = 8, // done
		DATABASE = 9,
		KAFKA = 10,
		KINESIS = 11,
		HTTP = 12
	};

	enum class authtype: int8_t {

		UNKNOWN = -1,
		PASSWORD = 0,
		PRIVATEKEY = 1,
		KERBEROS = 2,
		NTLM = 3
	};

	struct entry {

		std::string name;
		int type;
		long long size;
		flags flag;
		protocol proto;
		struct stat stats;
	};

	std::string protoname(axon::protocol);
	axon::protocol protoid(const std::string&);
	std::string authname(axon::authtype);
	axon::authtype authid(const std::string&);

	class AwsStack {

		static std::atomic<int> _instance;
		static std::mutex _lock;

		Aws::SDKOptions _options;

		public:
		AwsStack() {
			axon::timer ctm(__PRETTY_FUNCTION__);
			std::lock_guard<std::mutex> guard(_lock);

			if (_instance <= 0)
			{
				// TODO:
				// need to figure out how to disable IMDS query, specially when non-AWS service
				// provider like minio. for the time export AWS_EC2_METADATA_DISABLED="true" for
				// environment variable as workaround.
				// https://github.com/aws/aws-sdk-ruby/issues/2174
				// WORKAROUND:
				setenv("AWS_EC2_METADATA_DISABLED", "true", 1);
#if DEBUG >= 4
				_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
#endif
				Aws::InitAPI(_options);
				_instance++;
			}
		}
		~AwsStack() {
			axon::timer ctm(__PRETTY_FUNCTION__);
			std::lock_guard<std::mutex> guard(_lock);

			_instance--;
			if (_instance <= 0) Aws::ShutdownAPI(_options);
		}
	};

	namespace transfer {

		// typedef int (*callback) (const struct entry *);
		typedef std::function<void(axon::entry &)> cb;

		class connection {

			protected:
				std::string _id;
				std::string _hostname, _username, _password;
				std::string _path;
				uint16_t _port;
				axon::protocol _proto;

				bool _connected, _fileopen;
				std::ios_base::openmode _om;
				std::vector<boost::regex> _filter;

				virtual bool push(axon::transfer::connection&) = 0;

			public:

				connection(std::string, std::string, std::string, uint16_t);
				connection(std::string, std::string, std::string);
				connection(const connection&);
				~connection();

				// virtual bool set(char, std::string) = 0; <- is causing diamond problem, TODO: need to find a solution for this

				virtual bool connect() = 0;
				virtual bool disconnect() = 0;

				virtual bool filter(std::string) final;
				virtual bool match(std::string) final;

				virtual bool chwd(std::string) = 0;
				virtual std::string pwd() = 0;
				virtual bool mkdir(std::string) = 0;
				virtual size_t list(const cb &) = 0;
				virtual size_t list(std::vector<axon::entry> &) = 0;
				virtual off_t copy(std::string, std::string, bool) = 0;
				virtual off_t copy(std::string, std::string) = 0;
				virtual bool ren(std::string, std::string) = 0;
				virtual bool del(std::string) = 0;

				virtual off_t get(std::string, std::string, bool) = 0;
				virtual off_t put(std::string, std::string, bool) = 0;
				virtual off_t put(std::string src, std::string dest) final { return put(src, dest, false); };
				virtual off_t put(std::string src) final { return put(src, src, false); };

				virtual bool open(std::string, std::ios_base::openmode) = 0;
				virtual bool close() = 0;

				virtual ssize_t read(char*, size_t) = 0;
				virtual ssize_t write(const char*, size_t) = 0;

				friend axon::transfer::connection& operator>>(axon::transfer::connection& src, axon::transfer::connection& dest) {
					src.push(dest);
					return src;
				}
				friend axon::transfer::connection& operator<<(axon::transfer::connection& dest, axon::transfer::connection& src) {
					src.push(dest);
					return dest;
				}
		};
	}
}

#endif
