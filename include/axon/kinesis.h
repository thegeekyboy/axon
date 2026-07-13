#ifndef AXON_KINESIS_H_
#define AXON_KINESIS_H_

#include <algorithm>

#include <axon/stream.h>
#include <axon/connection.h>

#include <axon/resultset.h>

#include <aws/core/Aws.h>
#include <aws/kinesis/KinesisClient.h>

#define AXON_KINESIS_ROW_GET 100
#define AXON_KINESIS_DATA_SIZE 8192
#define AXON_KINESIS_POLL_INTERVAL 250   // ms between GetRecords calls per shard
#define AXON_KINESIS_IDLE_SLEEP 500   // ms to sleep when shard returns 0 records

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

		namespace aws {

			struct event {

				std::string  topic_name;							// stream/topic name this record came from
				std::string  shard_id;								// shard the record arrived on
				std::string  sequence_number;
				std::string  partition_key;
				int64_t	  timestamp { 0 };						// epoch ms
				std::string  data;									// raw record payload bytes
			};

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
				std::vector<axon::stream::aws::partition> _partitions;

				std::string _id, _name, _arn;
				bool _ready { false };
				bool _busy  { false };

				std::string _resolve_arn(const std::string&);
				std::vector<partition> _get_partitions(const std::string&);

				public:
					stream() = delete;

					stream(std::shared_ptr<Aws::Kinesis::KinesisClient>, const std::string&);
					stream(const stream&);

					~stream() = default;

					const std::string& name() const { return _name; }
					const std::string& arn() const { return _arn; }
					bool ready() const { return _ready; }
					bool busy() const { return _busy; }
					explicit operator bool() const { return _ready; }

					const std::vector<axon::stream::aws::partition> &partitions() const { return _partitions; }
					std::vector<axon::stream::aws::partition> &partitions() { return _partitions; }

					void create();
					void remove();
			};

			class consumer {

				std::shared_ptr<Aws::Kinesis::KinesisClient> _client;
				std::string _name;
				std::vector<std::string> _arns;
				bool _ready { false };
				uint8_t _count { 0 };

				bool _is_attached(const std::string&) const;
				bool _validate_arn(const std::string&) const;
				std::string _find_arn(const std::string&, const std::string&) const;

				static std::string _register(std::shared_ptr<Aws::Kinesis::KinesisClient>, const std::string&, const std::string&);
				bool _deregister(std::shared_ptr<Aws::Kinesis::KinesisClient>, const std::string&);

				public:
					consumer() = delete;
					consumer(std::shared_ptr<Aws::Kinesis::KinesisClient>, const std::string&);

					const std::string& name() const { return _name; }
					const std::vector<std::string>& arns() const { return _arns; }
					bool ready() const { return _ready; }
					explicit operator bool() const { return _ready; }

					std::string attach(const std::string&);
					std::string attach(axon::stream::aws::stream&);

					bool detach(const std::string&);
					bool detach(axon::stream::aws::stream&);
					void detach();
			};

		} // namespace aws

		class kinesis: public axon::stream::connector {


			axon::AwsStack _aws;
			std::string _account, _proxy;

			std::shared_ptr<Aws::Kinesis::KinesisClient> _client;

			bool _sync { true };
			uint8_t _consumer_count { 0 };

			std::vector<axon::stream::aws::stream> _streams;
			std::unique_ptr<axon::stream::aws::consumer> _consumer;

			std::deque<axon::stream::aws::event> _queue;
			std::mutex _queue_mtx;
			std::condition_variable _queue_cv;

			std::vector<std::thread> _shard_threads;
			std::atomic<bool> _shard_running { false };

			void _stop() override;
			void _run();

			void _poll_shard(const std::string&, const std::string&, axon::stream::aws::partition);
			static std::unique_ptr<axon::resultset> _build_recordset(const axon::stream::aws::event&);

			public:
				kinesis() = delete;

				kinesis(std::string, std::string, std::string);
				kinesis(std::string, std::string, std::string, uint16_t);

				~kinesis();

				static void asynccb(const Aws::Kinesis::KinesisClient*, const Aws::Kinesis::Model::SubscribeToShardRequest&, const Aws::Kinesis::Model::SubscribeToShardOutcome&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&);

				std::string& account() { return _account; };

				void subscribe() override;
				void unsubscribe() override;

				bool start() override;
				bool start(axon::stream::cbfn) override;

				void fetch(axon::resultset&, int) override
				{
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kinesis is callback-only — use start() with a callback");
				}
		};
	}
}


#endif

