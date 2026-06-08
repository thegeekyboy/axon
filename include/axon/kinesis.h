#ifndef AXON_KINESIS_H_
#define AXON_KINESIS_H_

#include <algorithm>

#include <axon/stream.h>
#include <axon/connection.h>

#include <axon/recordset.h>

#include <aws/core/Aws.h>
#include <aws/kinesis/KinesisClient.h>

#define AXON_KINESIS_ROW_GET 100
#define AXON_KINESIS_DATA_SIZE 8192

#define AXON_AWS_ACCOUNT_ID 'a'

namespace axon {

	namespace stream {

		enum class ARN {

			UNKNOWN,
			S3,
			STREAM,
			CONSUMER
		};

		inline axon::stream::ARN resolve(std::string arn) {
			boost::regex c_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/[A-Za-z0-9_.-]{1,128}\\/consumer\\/[A-Za-z0-9_.-]{1,128}:\\d{10,}$");
			boost::regex s_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/[A-Za-z0-9_.-]{1,128}$");

			if (boost::regex_match(arn, c_pattern)) return axon::stream::ARN::CONSUMER;
			else if (boost::regex_match(arn, s_pattern)) return axon::stream::ARN::STREAM;

			return axon::stream::ARN::UNKNOWN;
		}

		class kinesis: public axon::stream::interface {

			class stream; // WARNING: Forward declaration
			class consumer; // WARNING: Forward declaration

			class shard {
				bool _status;
				size_t _size;
				Aws::String _id, _name, _iterator;

				public:
					axon::stream::cbfn callback;

					shard(Aws::String name, Aws::String id, Aws::String iterator, axon::stream::cbfn callback): _status(false), _size(0), _id(id), _name(name), _iterator(iterator), callback(callback) {};
					const Aws::String get() const { return _iterator; };
					void set(Aws::String iterator) {
						_iterator = iterator;
					}
					size_t size() { return _size; }
			};

			struct partition {

				std::string id;
				Aws::String iterator;

				partition() = delete;
				partition& operator= (const partition&) = delete;

				partition(std::string id, std::string iterator): id(id), iterator(iterator), _id(axon::util::uuid()) { };
				partition(const partition& lhs): id(lhs.id), iterator(lhs.iterator), _id(axon::util::uuid()) { };

				private:
				std::string _id;
			};

			class stream {

				std::shared_ptr<Aws::Kinesis::KinesisClient> _client;
				std::vector<partition> _partition;

				std::string _id, _name, _arn;
				bool _ready, _sync, _busy;
				axon::stream::cbfn _callback;

				std::string _get_stream_arn(std::string);
				std::vector<axon::stream::kinesis::partition> get_stream_partitions(std::string);

				public:
					stream() = delete;
					stream(std::shared_ptr<Aws::Kinesis::KinesisClient>, std::string, axon::stream::cbfn);

					stream(const stream&);

					~stream() { ERRPRN("%s %s stream destroying", _id.c_str(), _name.c_str()); }

					explicit operator bool() { return _ready; };

					const std::string name() const { return _name; };
					const std::string arn() const { return _arn; };
					bool ready() const { return _ready; }
					bool busy() const { return _busy; }

					void create();
					void remove();

					// void start();
					// void start()
			};

			class consumer {

				std::shared_ptr<Aws::Kinesis::KinesisClient> _client;

				std::string _name;
				std::vector<std::string> _arn;

				bool _fanout, _ready, _busy;
				uint8_t _count;

				bool _is_attached(std::string);
				bool _validate_consumer_arn(std::string);
				std::string _get_consumer_arn(std::string, std::string);

				public:
					consumer() = delete;
					consumer(std::shared_ptr<Aws::Kinesis::KinesisClient>, std::string, bool);

					explicit operator bool() { return _ready; };

					const std::string name() const { return _name; };
					const std::vector<std::string> arn() const { return _arn; };

					static std::string attach(std::shared_ptr<Aws::Kinesis::KinesisClient>, std::string, std::string);
					static bool detach(std::shared_ptr<Aws::Kinesis::KinesisClient>, std::string);

					std::string attach(axon::stream::kinesis::stream&);
					std::string attach(std::string);

					bool detach(axon::stream::kinesis::stream&);
					bool detach(std::string);
					void detach();
			};

			axon::AwsStack _aws;
			std::string _account, _proxy;

			std::shared_ptr<Aws::Kinesis::KinesisClient> _client;

			bool _sync;
			uint8_t _consumer_count;

			std::vector<axon::stream::kinesis::stream> _streams;
			std::unique_ptr<axon::stream::kinesis::consumer> _consumer;

			std::queue<std::unique_ptr<axon::recordset>> _records;

			std::vector<std::thread> th;

			void _stop() override;
			std::unique_ptr<axon::recordset> get();

			public:
				kinesis(std::string, std::string, std::string);
				kinesis(std::string, std::string, std::string, uint16_t);

				kinesis() = delete;
				~kinesis();

				static void asynccb(const Aws::Kinesis::KinesisClient*, const Aws::Kinesis::Model::SubscribeToShardRequest&, const Aws::Kinesis::Model::SubscribeToShardOutcome&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&);

				std::string &account() { return _account; };

				void subscribe() override;
				void unsubscribe() override;

				bool start() override;
				bool start(axon::stream::cbfn) override;

				std::unique_ptr<axon::recordset> next() override;
		};
	}
}


#endif

