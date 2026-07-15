#ifndef AXON_AWS_H_
#define AXON_AWS_H_

#include <csignal>

#include <aws/core/Aws.h>

namespace axon {

	namespace aws {

		class stack {

			static std::atomic<int> _instance;
			static std::mutex _lock;

			Aws::SDKOptions _options;

			public:
				stack();
				~stack();
		};
	}
}

#endif