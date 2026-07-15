#include <atomic>
#include <mutex>

#include <axon.h>
#include <axon/aws.h>

namespace axon {

	namespace aws {

		std::atomic<int> stack::_instance = 0;
		std::mutex stack::_lock;

		stack::stack()
		{
			std::lock_guard<std::mutex> guard(_lock);

			if (_instance <= 0)
			{
				// The AWS SDK's curl/OpenSSL backend can attempt to write (e.g. a
				// TLS close-notify alert during connection teardown) to a socket
				// whose peer has already reset/closed the connection. The default
				// disposition for SIGPIPE is to terminate the process, which is
				// fatal for a long-lived stream consumer (kinesis, s3, etc.) and
				// shows up as an intermittent crash under load. Ignore it here so
				// the underlying write() instead fails with EPIPE, which curl and
				// the SDK already handle as a normal I/O error.

				signal(SIGPIPE, global_signal_trap);

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

		stack::~stack()
		{
			std::lock_guard<std::mutex> guard(_lock);

			_instance--;
			if (_instance <= 0) Aws::ShutdownAPI(_options);
		}
	}
}