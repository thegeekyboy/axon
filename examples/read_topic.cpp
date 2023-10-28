#include <iostream>
#include <variant>

#include <axon.h>
#include <axon/kafka.h>

#include <signal.h>

extern "C" {
	#include <avro.h>
	#include <libserdes/serdes.h>
	#include <libserdes/serdes-avro.h>
}

// axon::transport::transfer::kafka c("10.96.8.11,10.96.8.12,10.96.8.13,10.96.8.14", "10.96.8.11:8081,10.96.8.12:8081,10.96.8.13:8081,10.96.8.14:8081");
axon::transport::transfer::kafka c("10.96.38.152,10.96.38.153,10.96.38.154", "10.96.38.152:8081,10.96.38.153:8081,10.96.38.154:8081", "KafkaConsumer");

typedef std::variant<std::string, double, int64_t, int> avt;

static void stop (int sig)
{
	c.stop();
	fprintf(stderr, "stopping eventloop! received signal: %d\n", sig);
	fflush(stderr);
}

void dbg(avro_value_t *avro)
{
	char *as_json;
	
	if (avro_value_to_json(avro, 1, &as_json))
		fprintf(stderr, "=> avro_to_json failed: %s\n", avro_strerror());
	else {
		printf("%15s\n", as_json);
		free(as_json);
	}
}

avt get(avro_value_t *avro, std::string name)
{
	avro_value_t init_iden_value;
	avt retval;

	if (avro_value_get_by_name(avro, name.c_str(), &init_iden_value, NULL) == 0)
	{
		avro_value_t branch;

		// union_schema = avro_value_get_schema(&init_iden_value);
		avro_value_set_branch(&init_iden_value, 1, &branch);

		switch (avro_value_get_type(&branch))
		{
			case AVRO_BOOLEAN:
			{
				int value;

				avro_value_get_boolean(&branch, &value);
			}
			break;

			case AVRO_BYTES:
			{
				const void *buffer;
				size_t  size;

				avro_value_get_bytes(&branch, &buffer, &size);

				return reinterpret_cast<const char*>(buffer);
			}
			break;

			case AVRO_STRING:
			{
				const char *buffer;
				size_t size;

				avro_value_get_string(&branch, &buffer, &size);
				return buffer;
			}
			break;

			case AVRO_INT64:
			{
				int64_t value;

            	avro_value_get_long(&branch, &value);
				return value;
			}
			break;

			default:
				std::cout<<"unknown avro type: "<<avro_value_get_type(&branch)<<std::endl;
				return 0;
				break;
		}
		// std::cout<<name<<disc<<" ("<<avro_value_get_type(&branch)<<"): "<<p<<std::endl;

		// fprintf(stdout, "%d => %s\n", rkm->partition, p);
	}

	return 1;
}

void parse_message(avro_value_t *avro)
// void parse_message(rd_kafka_message_t *rkm, serdes_t *serdes)
{
	// rd_kafka_timestamp_type_t tstype;
	// rd_kafka_headers_t *hdrs;

	// serdes_err_t err;
	// serdes_schema_t *schema;

	// avro_value_t avro;

	// char error[512];
	// // size_t size;
	// // int64_t timestamp = rd_kafka_message_timestamp(rkm, &tstype);

	// err = serdes_deserialize_avro(serdes, &avro, &schema, rkm->payload, rkm->len, error, sizeof(error));
	// if (err)
	// {
	// 	fprintf(stderr, "=> serdes_deserialize_avro failed: %s\n", error);

	// 	return;
	// }

#ifdef DEBUG
//	printf(" arrived %ds ago\n", (int)time(NULL) - (int)(timestamp/1000));

	char *as_json;
	
	if (avro_value_to_json(avro, 1, &as_json))
		fprintf(stderr, "=> avro_to_json failed: %s\n", avro_strerror());
	else {
		printf("%15s\n", as_json);
		free(as_json);
	}
#endif

	avt retval;
	
	std::cout<<std::get<std::string>(get(avro, "ORDERID"))<<std::endl;
	// std::cout<<std::get<int64_t>(get(avro, "LASTUPDDATE"))<<std::endl;
	// std::cout<<std::get<std::string>(get(avro, "INITIATOR_ID"));

	// get(avro, "LASTUPDDATE");
	// get(avro, "ORDERID");
	// get(avro, "LASTUPDDATE");
	// get(avro, "LASTUPDDATE");

	//std::cout<<retval.index()<<std::endl;
}

int main()
{
	c.add("TOPIC1");
	c.add("TOPIC2");
	c.connect();
	// c.add("TOPIC3");
	c.start(&parse_message);
	
	signal(SIGINT, stop);
	
	// std::cout<<"return value is: "<<tcn::version()<<std::endl;

	return 0;
}
