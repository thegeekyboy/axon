#include <string>
#include <thread>
#include <functional>
#include <limits.h>

#include <librdkafka/rdkafka.h>
#ifndef AXON_KAFKA_H_
#define AXON_KAFKA_H_

extern "C" {
	#include <libserdes/serdes.h>
}

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
            typedef void (*cbfn)(rd_kafka_message_t *, serdes_t *);
            typedef void *(*cbfnptr)(void *);

            const int MIN_COMMIT_COUNT = 500;

            class kafka {

                rd_kafka_t *rk;
                rd_kafka_conf_t *kconf;
                rd_kafka_topic_partition_list_t *subscription;
                rd_kafka_resp_err_t err;

                serdes_conf_t *sconf;
                serdes_t *serdes;

                std::string kafkaEndpoint, schemaRegistry;
                char hostname[HOST_NAME_MAX], strerr[512];

                std::thread runner;
                std::function<void(rd_kafka_message_t *, serdes_t *)> f;
                bool runnable;
                unsigned long long counter;

                public:
                kafka(std::string, std::string);
                ~kafka();

                bool connect(std::string);
                bool start(std::function<void(rd_kafka_message_t *, serdes_t *)>);
                void stop();
                
                void eventloop(rd_kafka_message_t *, serdes_t *);
            };
        }
    }
}

#endif