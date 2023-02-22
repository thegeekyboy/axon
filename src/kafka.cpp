#include <iostream>
#include <string>
#include <thread>

#include <unistd.h>
#include <limits.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

#include <librdkafka/rdkafka.h>

#include <axon.h>
#include <axon/kafka.h>

namespace axon
{
	namespace transport
	{
		namespace transfer
		{
            kafka::kafka(std::string ke, std::string sr): kafkaEndpoint(ke), schemaRegistry(sr)
            {
                if (gethostname(hostname, sizeof(hostname)))
                {
                    throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Failed to lookup hostname");
                }

                sconf = serdes_conf_new(NULL, 0, "schema.registry.url", sr.c_str(), NULL);

                if (!(serdes = serdes_new(sconf, strerr, sizeof(strerr))))
                {
                    throw strerr;
                }

                kconf = rd_kafka_conf_new();

                if (rd_kafka_conf_set(kconf, "client.id", hostname, strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                {
                    throw strerr;
                }

                if (rd_kafka_conf_set(kconf, "group.id", "KafkaConsumer", strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                {
                    rd_kafka_conf_destroy(kconf);
                    throw strerr;
                }

                // if (rd_kafka_conf_set(kconf, "enable.partition.eof", "true", strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                // {
                // 	rd_kafka_conf_destroy(kconf);
                // 	throw strerr;
                // }

                if (rd_kafka_conf_set(kconf, "enable.auto.commit", "true", strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                {
                    rd_kafka_conf_destroy(kconf);
                    throw strerr;
                }

                if (rd_kafka_conf_set(kconf, "auto.offset.reset", "latest", strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                {
                    rd_kafka_conf_destroy(kconf);
                    throw strerr;
                }

                if (rd_kafka_conf_set(kconf, "bootstrap.servers", ke.c_str(), strerr, sizeof(strerr)) != RD_KAFKA_CONF_OK)
                {
                    throw strerr;
                }

                runnable = false;
                counter = 0;
            }

            kafka::~kafka()
            {
                if (runner.joinable())
                    runner.join();

                rd_kafka_consumer_close(rk);
                rd_kafka_destroy(rk);

                serdes_destroy(serdes);
            }

            bool kafka::connect(std::string table)
            {
                if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, kconf, strerr, sizeof(strerr))))
                    throw strerr;

                kconf = NULL;
                rd_kafka_poll_set_consumer(rk);

                subscription = rd_kafka_topic_partition_list_new(1);
                rd_kafka_topic_partition_list_add(subscription, table.c_str(), RD_KAFKA_PARTITION_UA);
                
                if ((err = rd_kafka_subscribe(rk, subscription)))
                {
                    // fprintf(stderr, "=> Failed to subscribe to %d topics: %s\n", subscription->cnt, rd_kafka_err2str(err));
                    rd_kafka_topic_partition_list_destroy(subscription);
                    rd_kafka_destroy(rk);

                    throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "failed to subscribe to topic");
                }

                // fprintf(stderr, "=> Subscribed to %d topic(s), waiting for rebalance and messages...\n", subscription->cnt);
                rd_kafka_topic_partition_list_destroy(subscription);

                return true;
            }

            bool kafka::start(std::function<void(rd_kafka_message_t *rkm, serdes_t *s)> fn)
            {
                if (runnable == true)
                    return false;

                f = fn;
                runnable = true;

                runner = std::thread([this] () {

                    while (runnable)
                    {
                        rd_kafka_message_t *rkm;

                        if (!(rkm = rd_kafka_consumer_poll(rk, 300)))
                        {
                            counter = 0;
                            continue;
                        }

                        if (rkm->err)
                        {
                            fprintf(stderr, "=> Consumer error: %s\n", rd_kafka_message_errstr(rkm));
                            rd_kafka_message_destroy(rkm);

                            continue;
                        }

                        try {
                            f(rkm, serdes);
                        } catch(const std::bad_function_call& e) {
                            std::cout<<e.what()<<std::endl;
                        }
                        rd_kafka_message_destroy(rkm);

                        if ((counter % MIN_COMMIT_COUNT) == 0)
                        {
                            err = rd_kafka_commit(rk, NULL, 0);
                            if (err) {
                                fprintf(stderr, "=> Consumer error: %s\n", rd_kafka_err2str(err));
                            }
                        }

                        counter++;
                    }
                });

                return true;
            }

            void kafka::stop()
            {
                if (runnable)
                {
                    runnable = false;
                }
            }

            void kafka::eventloop(rd_kafka_message_t *rkm, serdes_t *serdes)
            {
                while (runnable)
                {
                    rd_kafka_message_t *rkm;

                    if (!(rkm = rd_kafka_consumer_poll(rk, 1000)))
                    {
                        counter = 0;
                        continue;
                    }

                    if (rkm->err)
                    {
                        // fprintf(stderr, "=> Consumer error: %s\n", rd_kafka_message_errstr(rkm));
                        rd_kafka_message_destroy(rkm);

                        continue;
                    }

                    f(rkm, serdes);
                    rd_kafka_message_destroy(rkm);

                    counter++;
                }

                return;
            }
        }
    }
}