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

	enum class protocol : int8_t {
		UNKNOWN  = -1,
		NOTHING  = 0,
		FILE     = 1,
		SFTP     = 2,
		FTP      = 3,
		S3       = 4,
		SAMBA    = 5,
		HDFS     = 6,
		AWS      = 7,
		SCP      = 8,
		DATABASE = 9,
		KAFKA    = 10,
		KINESIS  = 11,
		HTTP     = 12
	};

	enum class authtype : int8_t {
		UNKNOWN    = -1,
		PASSWORD   = 0,
		PRIVATEKEY = 1,
		KERBEROS   = 2,
		NTLM       = 3
	};

	constexpr std::string_view protoname(axon::protocol p) noexcept
	{
		switch (p)
		{
			case axon::protocol::NOTHING:  return "axon::protocol::NOTHING";
			case axon::protocol::FILE:     return "axon::protocol::FILE";
			case axon::protocol::SFTP:     return "axon::protocol::SFTP";
			case axon::protocol::FTP:      return "axon::protocol::FTP";
			case axon::protocol::S3:       return "axon::protocol::S3";
			case axon::protocol::SAMBA:    return "axon::protocol::SAMBA";
			case axon::protocol::HDFS:     return "axon::protocol::HDFS";
			case axon::protocol::AWS:      return "axon::protocol::AWS";
			case axon::protocol::SCP:      return "axon::protocol::SCP";
			case axon::protocol::DATABASE: return "axon::protocol::DATABASE";
			case axon::protocol::KAFKA:    return "axon::protocol::KAFKA";
			case axon::protocol::KINESIS:  return "axon::protocol::KINESIS";
			case axon::protocol::HTTP:     return "axon::protocol::HTTP";
			default:                       return "axon::protocol::UNKNOWN";
		}
	}

	constexpr axon::protocol protoid(std::string_view name) noexcept
	{
		if (name == "NOTHING"  || name == "AXON::PROTOCOL::NOTHING")  return axon::protocol::NOTHING;
		if (name == "FILE"     || name == "AXON::PROTOCOL::FILE")     return axon::protocol::FILE;
		if (name == "SFTP"     || name == "AXON::PROTOCOL::SFTP")     return axon::protocol::SFTP;
		if (name == "FTP"      || name == "AXON::PROTOCOL::FTP")      return axon::protocol::FTP;
		if (name == "S3"       || name == "AXON::PROTOCOL::S3")       return axon::protocol::S3;
		if (name == "SAMBA"    || name == "AXON::PROTOCOL::SAMBA")    return axon::protocol::SAMBA;
		if (name == "HDFS"     || name == "AXON::PROTOCOL::HDFS")     return axon::protocol::HDFS;
		if (name == "AWS"      || name == "AXON::PROTOCOL::AWS")      return axon::protocol::AWS;
		if (name == "SCP"      || name == "AXON::PROTOCOL::SCP")      return axon::protocol::SCP;
		if (name == "DATABASE" || name == "AXON::PROTOCOL::DATABASE") return axon::protocol::DATABASE;
		if (name == "KAFKA"    || name == "AXON::PROTOCOL::KAFKA")    return axon::protocol::KAFKA;
		if (name == "KINESIS"  || name == "AXON::PROTOCOL::KINESIS")  return axon::protocol::KINESIS;
		if (name == "HTTP"     || name == "AXON::PROTOCOL::HTTP")     return axon::protocol::HTTP;
		return axon::protocol::UNKNOWN;
	}

	constexpr std::string_view authname(axon::authtype a) noexcept
	{
		switch (a)
		{
			case axon::authtype::PASSWORD:   return "axon::authtype::PASSWORD";
			case axon::authtype::PRIVATEKEY: return "axon::authtype::PRIVATEKEY";
			case axon::authtype::KERBEROS:   return "axon::authtype::KERBEROS";
			case axon::authtype::NTLM:       return "axon::authtype::NTLM";
			default:                         return "axon::authtype::UNKNOWN";
		}
	}

	constexpr axon::authtype authid(std::string_view name) noexcept
	{
		if (name == "PASSWORD"   || name == "AXON::AUTHTYPE::PASSWORD")   return axon::authtype::PASSWORD;
		if (name == "PRIVATEKEY" || name == "AXON::AUTHTYPE::PRIVATEKEY") return axon::authtype::PRIVATEKEY;
		if (name == "KERBEROS"   || name == "AXON::AUTHTYPE::KERBEROS")   return axon::authtype::KERBEROS;
		if (name == "NTLM"       || name == "AXON::AUTHTYPE::NTLM")       return axon::authtype::NTLM;
		return axon::authtype::UNKNOWN;
	}

	struct entry {

		std::string name;
		int type;
		long long size;
		flags flag;
		protocol proto;
		struct stat stats;
	};

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

