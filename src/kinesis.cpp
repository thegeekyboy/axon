#include <axon.h>
#include <axon/kinesis.h>

#include <aws/kinesis/KinesisClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <aws/kinesis/model/ListShardsRequest.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/SubscribeToShardRequest.h>

#include <aws/kinesis/model/GetRecordsRequest.h>

#include <aws/kinesis/model/ListStreamsRequest.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/CreateStreamRequest.h>
#include <aws/kinesis/model/DeleteStreamRequest.h>

#include <aws/kinesis/model/ListStreamConsumersRequest.h>
#include <aws/kinesis/model/RegisterStreamConsumerRequest.h>
#include <aws/kinesis/model/DeregisterStreamConsumerRequest.h>
#include <aws/kinesis/model/DescribeStreamConsumerRequest.h>
#include <aws/kinesis/model/DescribeStreamConsumerResult.h>

namespace axon {

	namespace stream {

		namespace aws {

			// Normal
			std::string stream::_resolve_arn(const std::string &name)
			{
				BENCHMARK;

				boost::regex pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/([A-Za-z0-9_.-]{1,128})$", boost::regex::extended);
				boost::smatch what;

				bool matched = boost::regex_match(name, what, pattern);

				Aws::Kinesis::Model::DescribeStreamRequest request;

				if (matched)
					request.SetStreamARN(name);
				else
					request.SetStreamName(name);

				Aws::Kinesis::Model::DescribeStreamOutcome outcome = _client->DescribeStream(request);

				if (!outcome.IsSuccess())
				{
					if (outcome.GetError().GetErrorType() == Aws::Kinesis::KinesisErrors::RESOURCE_NOT_FOUND)
						return {};

					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());
				}

				if (matched && !std::string(what[1]).empty()) _name = what[1];

				return outcome.GetResult().GetStreamDescription().GetStreamARN();
			};

			std::vector<axon::stream::aws::partition> stream::_get_partitions(const std::string &arn)
			{
				BENCHMARK;

				Aws::Kinesis::Model::ListShardsRequest request;

				request.SetStreamARN(arn);

				Aws::Kinesis::Model::ListShardsOutcome outcome = _client->ListShards(request);

				if (!outcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

				std::vector<partition> parts;

				for (const auto& shard : outcome.GetResult().GetShards())
				{
					Aws::Kinesis::Model::GetShardIteratorRequest getShardIteratorRequest;

					getShardIteratorRequest.SetStreamARN(arn);
					getShardIteratorRequest.SetShardId(shard.GetShardId());
					getShardIteratorRequest.SetShardIteratorType(Aws::Kinesis::Model::ShardIteratorType::LATEST);

					Aws::Kinesis::Model::GetShardIteratorOutcome getShardIteratorOutcome = _client->GetShardIterator(getShardIteratorRequest);

					if (!getShardIteratorOutcome.IsSuccess())
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, getShardIteratorOutcome.GetError().GetMessage());

					parts.emplace_back(shard.GetShardId(), getShardIteratorOutcome.GetResult().GetShardIterator());
				}

				return parts;
			}

			stream::stream(std::shared_ptr<Aws::Kinesis::KinesisClient> client, const std::string &name):
			_client(client), _id(axon::util::uuid()), _name(name)
			{
				BENCHMARK;
				DBGPRN("%s %s stream creating", _id.c_str(), _name.c_str());

				if (name.empty()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream name cannot be empty");

				_arn = _resolve_arn(name);

				if (!_arn.empty())
				{
					_partitions = _get_partitions(_arn);

					if (_partitions.size()) _ready = true;
				}

				DBGPRN("[kinesis] stream: %s arn: %s shards: %zu", _name.c_str(), _arn.c_str(), _partitions.size());
			}

			stream::stream(const aws::stream& other):
			_client(other._client), _partitions(other._partitions), _id(axon::util::uuid()), _name(other._name), _arn(other._arn), _ready(other._ready), _busy(other._busy)
			{
				DBGPRN("%s copy invoked from %s to %s", _name.c_str(), _id.c_str(), other._id.c_str());
			}

			void stream::create()
			{
				BENCHMARK;

				if (_ready) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream already exists");

				Aws::Kinesis::Model::CreateStreamRequest request;

				request.SetStreamName(_name);
				request.SetShardCount(1);

				Aws::Kinesis::Model::CreateStreamOutcome outcome = _client->CreateStream(request);

				if (!outcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

				_arn = _resolve_arn(_name);
				if (!_arn.empty())
				{
					_partitions = _get_partitions(_arn);
					if (_partitions.size()) _ready = true;
				}
			}

			void stream::remove()
			{
				BENCHMARK;
				if (!_ready) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kinesis stream does not exists");

				Aws::Kinesis::Model::DeleteStreamRequest request;

				request.SetStreamARN(_arn);
				request.SetEnforceConsumerDeletion(true);

				Aws::Kinesis::Model::DeleteStreamOutcome outcome = _client->DeleteStream(request);

				if (!outcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

				_arn = {};
				_partitions.clear();
				_ready = false;
			}

			// Consumer
			bool consumer::_is_attached(const std::string &arn) const
			{
				return (std::find(_arns.begin(), _arns.end(), arn) != _arns.end());
			}

			bool consumer::_validate_arn(const std::string &arn) const
			{
				BENCHMARK;

				Aws::Kinesis::Model::DescribeStreamConsumerRequest req;
				req.SetConsumerARN(arn);
				return _client->DescribeStreamConsumer(req).IsSuccess();
			}

			std::string consumer::_find_arn(const std::string &stream_arn, const std::string &consumer_name) const
			{
				BENCHMARK;
				Aws::Kinesis::Model::ListStreamConsumersRequest request;

				request.SetStreamARN(stream_arn);

				Aws::Kinesis::Model::ListStreamConsumersOutcome outcome = _client->ListStreamConsumers(request);

				if (!outcome.IsSuccess())
					return {};

				const Aws::Vector<Aws::Kinesis::Model::Consumer> consumers = outcome.GetResult().GetConsumers();

				for (const auto& consumer : consumers)
				{
					DBGPRN("arn: %s", consumer.GetConsumerARN().c_str());
					if (consumer.GetConsumerARN().find(consumer_name) != std::string::npos) return consumer.GetConsumerARN();
				}

				return {};
			}

			std::string consumer::_register(std::shared_ptr<Aws::Kinesis::KinesisClient> client, const std::string &stream_arn, const std::string &consumer_name)
			{
				BENCHMARK;

				Aws::Kinesis::Model::RegisterStreamConsumerRequest req;

				req.SetStreamARN(stream_arn);
				req.SetConsumerName(consumer_name);

				auto outcome = client->RegisterStreamConsumer(req);

				if (!outcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

				return std::string(outcome.GetResult().GetConsumer().GetConsumerARN());
			}

			bool consumer::_deregister(std::shared_ptr<Aws::Kinesis::KinesisClient> client, const std::string &arn)
			{
				BENCHMARK;
				Aws::Kinesis::Model::DeregisterStreamConsumerRequest request;

				request.SetConsumerARN(arn);

				Aws::Kinesis::Model::DeregisterStreamConsumerOutcome outcome = client->DeregisterStreamConsumer(request);

				if (!outcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

				return true;
			}

			consumer::consumer(std::shared_ptr<Aws::Kinesis::KinesisClient> client, const std::string &name):
			_client(client), _name(name)
			{
				boost::regex pattern("^[A-Za-z0-9_.-]{1,128}$");

				if (!boost::regex_match(name, pattern)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid consumer name");
				if (client == nullptr) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "client not valid");
			}

			std::string consumer::attach(const std::string &arn)
			{
				BENCHMARK;
				boost::regex c_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/([A-Za-z0-9_.-]{1,128})\\/consumer\\/([A-Za-z0-9_.-]{1,128}):\\d{10,}$", boost::regex::extended);
				boost::regex s_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/[A-Za-z0-9_.-]{1,128}$");
				boost::smatch what;

				std::string retarn;

				if (boost::regex_match(arn, what, c_pattern))
				{
					if (!_is_attached(arn))
					{
						if (!_validate_arn(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer arn not valid");

						if (_name != what[2]) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer name do not match");

						_arns.push_back(arn);
						_count++;
					}

					retarn = arn;
				}
				else if (boost::regex_match(arn, s_pattern))
				{
					retarn = _find_arn(arn, _name);

					if (retarn.empty()) retarn = _register(_client, arn, _name);

					if (!_is_attached(retarn)) {
						_arns.push_back(retarn);
						_count++;
					}
				}
				else throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream arn not in correct format");

				_ready = true;

				return retarn;
			}

			std::string consumer::attach(axon::stream::aws::stream &sh)
			{
				if (!sh.ready()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream is not ready");
				if (sh.busy()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream is already busy");

				return attach(sh.arn());
			};

			void consumer::detach()
			{
				BENCHMARK;

				DBGPRN("consumer count = %d", _count);

				for (int i = 0; i < _count; i++) {
					DBGPRN("removing => %s", _arns[i].c_str());
					if (!_arns[i].empty()) _deregister(_client, _arns[i]);
				}

				_arns.clear();
				_count = 0;
				_ready = false;
			}

			bool consumer::detach(const std::string &arn_)
			{
				BENCHMARK;
				std::string arn = arn_;
				axon::stream::ARN arntype = axon::stream::resolve(arn);

				if (arntype == axon::stream::ARN::UNKNOWN) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "arn is not in correct format");

				if (arntype == axon::stream::ARN::STREAM) arn = _find_arn(arn, _name);

				if (!_is_attached(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer not connected");

				if (!_validate_arn(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer arn not valid");

				if (_deregister(_client, arn))
				{
					_arns.erase(std::remove(_arns.begin(), _arns.end(), arn), _arns.end());
					if (!(_count = _arns.size())) _ready = false;
					return true;
				}

				return false;
			}

			bool consumer::detach(axon::stream::aws::stream &sh)
			{
				return detach(sh.arn());
			};
		} // aws

		// KINESIS
		void kinesis::_stop()
		{
			_runnable = false;
			_shard_running = false;
			_queue_cv.notify_all();   // wake _run() so it can exit
		}

		void kinesis::_poll_shard(const std::string &topic_name, [[maybe_unused]]const std::string &stream_arn, axon::stream::aws::partition part)   // owns iterator state
		{
			// NOTE: When a shard is closed (split or merge), GetNextShardIterator
			// returns empty. This thread exits and the child shards are NOT
			// automatically discovered. Call stop()/subscribe()/start() to
			// pick up the new shard topology after a reshard event.

			DBGPRN("[kinesis:%s] shard poll started: %s %s", _id.c_str(), topic_name.c_str(), part.id.c_str());

			while (_shard_running)
			{
				if (part.iterator.empty())
				{
					WRNPRN("[kinesis:%s] shard %s iterator expired — exiting", _id.c_str(), part.id.c_str());
					break;
				}

				try
				{
					Aws::Kinesis::Model::GetRecordsRequest req;
					req.SetShardIterator(part.iterator);
					req.SetLimit(AXON_KINESIS_ROW_GET);

					auto outcome = _client->GetRecords(req);

					if (!outcome.IsSuccess())
					{
						ERRPRN("[kinesis:%s] GetRecords error on %s: %s", _id.c_str(), part.id.c_str(), std::string(outcome.GetError().GetMessage()).c_str());
						std::this_thread::sleep_for(std::chrono::milliseconds(AXON_KINESIS_POLL_INTERVAL));

						continue;
					}

					const auto &result = outcome.GetResult();
					const auto &records = result.GetRecords();

					// Advance iterator for next call
					part.iterator = result.GetNextShardIterator();

					if (records.empty())
					{
						// No new records — sleep to avoid burning the API quota
						// (standard polling: 5 GetRecords calls/sec/shard limit)
						std::this_thread::sleep_for(std::chrono::milliseconds(AXON_KINESIS_IDLE_SLEEP));
						continue;
					}

					// Push each record onto the queue for the runner thread
					for (const auto &rec : records)
					{
						axon::stream::aws::event ev;

						ev.topic_name = topic_name;
						ev.shard_id = part.id;
						ev.sequence_number = std::string(rec.GetSequenceNumber());
						ev.partition_key = std::string(rec.GetPartitionKey());
						ev.timestamp = rec.GetApproximateArrivalTimestamp().Millis();

						// Copy raw payload bytes into std::string for transport
						const auto &payload = rec.GetData();
						ev.data.assign(reinterpret_cast<const char*>(payload.GetUnderlyingData()), payload.GetLength());

						{
							std::lock_guard<std::mutex> lk(_queue_mtx);
							_queue.push_back(std::move(ev));
						}

						_queue_cv.notify_one();
					}

					DBGPRN("[kinesis:%s] shard %s: fetched %zu records", _id.c_str(), part.id.c_str(), records.size());

					if (result.GetMillisBehindLatest() < 1000 || records.empty())
    					std::this_thread::sleep_for(std::chrono::milliseconds(AXON_KINESIS_IDLE_SLEEP));
				}
				catch (axon::exception &e)
				{
					ERRPRN("[kinesis:%s] shard %s: %s", _id.c_str(), part.id.c_str(), e.what());
				}
			}

			DBGPRN("[kinesis:%s] shard poll exited: %s", _id.c_str(), part.id.c_str());
		}

		std::unique_ptr<axon::resultset> kinesis::_build_recordset(const axon::stream::aws::event &ev)
		{
			auto rs = std::make_unique<axon::resultset>(ev.topic_name);

			rs->add_column("data", axon::column_type::bytes_t);
			rs->add_column("sequence_number", axon::column_type::string_t);
			rs->add_column("partition_key", axon::column_type::string_t);
			rs->add_column("timestamp", axon::column_type::int64_t);

			rs->begin_row();

			if (ev.data.empty())
				rs->push_null();
			else
				rs->push_bytes(ev.data.data(), ev.data.size());

			rs->push_string(ev.sequence_number);
			rs->push_string(ev.partition_key);
			rs->push_int(ev.timestamp);

			rs->end_row();

			return rs;
		}

		void kinesis::_run()
		{
			DBGPRN("[kinesis:%s] runner started", _id.c_str());

			while (_runnable)
			{
				axon::stream::aws::event ev;

				{
					std::unique_lock<std::mutex> lk(_queue_mtx);
					_queue_cv.wait(lk, [this] {
						return !_queue.empty() || !_runnable;
					});

					if (!_runnable && _queue.empty()) break;

					ev = std::move(_queue.front());
					_queue.pop_front();
				}

				try
				{
					// Find callback — global takes priority over per-topic
					axon::stream::cbfn cb = _callback;

					if (!cb)
					{
						for (const auto &t : _topic)
						{
							if (t.name == ev.topic_name || t.target == ev.topic_name)
							{
								cb = t.callback;
								break;
							}
						}
					}

					if (cb)
					{
						auto rs = _build_recordset(ev);
						cb(std::move(rs));
						_counter++;
					}
				}
				catch (axon::exception &e)
				{
					ERRPRN("[kinesis:%s] runner error: %s", _id.c_str(), e.what());
				}
				catch (...)
				{
					ERRPRN("[kinesis:%s] runner unknown error", _id.c_str());
				}
			}

			_running = false;
			DBGPRN("[kinesis:%s] runner exited", _id.c_str());
		}

		kinesis::kinesis(std::string hostname, std::string username, std::string password, uint16_t port):
		connector(hostname, username, password, port), _aws(axon::AwsStack()), _consumer_count(0)
		{
			BENCHMARK;

			Aws::Auth::AWSCredentials auth = Aws::Auth::AWSCredentials(_username, _password);
			Aws::Client::ClientConfiguration cfg;

			cfg.region = Aws::Region::ComputeSignerRegion(_hostname);

			if (_proxy.size() > 3)
			{
				std::vector<std::string> proxy = axon::util::split(_proxy, ':');
				cfg.proxyHost = proxy[0];
				if (proxy.size() > 1) cfg.proxyPort = std::stoi(proxy[1]);
				cfg.proxyScheme = Aws::Http::Scheme::HTTP;
			}

			_client = std::make_shared<Aws::Kinesis::KinesisClient>(auth, cfg);

			Aws::Kinesis::Model::ListStreamsRequest probe;

			probe.SetLimit(1);
			Aws::Kinesis::Model::ListStreamsOutcome outcome = _client->ListStreams(probe);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] " + outcome.GetError().GetMessage());

			_connected = true;
		}

		kinesis::kinesis(std::string hostname, std::string username, std::string password)
		: kinesis(hostname, username, password, 443)
		{
		}

		kinesis::~kinesis()
		{
			BENCHMARK;

			if (_runnable || _running)
				stop();

			_shard_running = false;
			for (auto &t : _shard_threads)
				if (t.joinable()) t.join();

			if (_connected)
			{
				if (_consumer) _consumer->detach();
				_connected = false;
			}
		}

		void kinesis::subscribe()
		{
			if (_topic.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no topics — call add() before subscribe()");

			if (!_connected)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not connected");

			for (auto &t : _topic)
			{
				axon::stream::aws::stream sh(_client, t.name);

				if (!sh)
				{
					WRNPRN("[kinesis] stream does not exist: %s", t.name.c_str());
					continue;
				}

				// For enhanced fan-out, attach consumer
				if (!_sync)
				{
					if (!_consumer) _consumer = std::make_unique<axon::stream::aws::consumer>(_client, _name);
					_consumer->attach(sh);
				}

				_streams.emplace_back(std::move(sh));
			}

			if (_streams.empty())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no valid streams found");

			_subscribed = true;
			DBGPRN("[kinesis:%s] subscribed to %zu streams", _id.c_str(), _streams.size());
		}

		void kinesis::unsubscribe()
		{
			//_client->DisableRequestProcessing();
			if (!_subscribed) return;

			_shard_running = false;

			for (auto &t : _shard_threads)
				if (t.joinable()) t.join();

			_shard_threads.clear();

			_streams.clear();
			_subscribed = false;

			DBGPRN("[kinesis:%s] unsubscribed", _id.c_str());
		}

		bool kinesis::start()
		{
			if (_runnable)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "already running");

			if (!_subscribed)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "not subscribed — call subscribe() before start()");

			_runnable = true;
			_running = true;
			_shard_running = true;

			// Spawn one shard poll thread per partition per stream
			for (auto &sh : _streams)
			{
				for (const auto &part : sh.partitions())
				{
					_shard_threads.emplace_back(&kinesis::_poll_shard, this, sh.name(), sh.arn(), part);
				}
			}

			// Start runner thread on _runner (connector base member)
			_runner = std::thread(&kinesis::_run, this);

			DBGPRN("[kinesis:%s] started — %zu shard threads + 1 runner", _id.c_str(), _shard_threads.size());

			return true;
		}

		bool kinesis::start(axon::stream::cbfn cb)
		{
			_callback = std::move(cb);
			return start();
		}

		void kinesis::asynccb(
			[[maybe_unused]] const Aws::Kinesis::KinesisClient *client,
			[[maybe_unused]] const Aws::Kinesis::Model::SubscribeToShardRequest &req,
			[[maybe_unused]] const Aws::Kinesis::Model::SubscribeToShardOutcome &outcome,
			[[maybe_unused]] const std::shared_ptr<const Aws::Client::AsyncCallerContext> &ctx
		)
		{
			// TODO: enhanced fan-out implementation
			// When implemented:
			//   1. Check outcome.IsSuccess()
			//   2. Iterate outcome.GetResult().GetEventStream() events
			//   3. For each SubscribeToShardEvent, extract records
			//   4. Build kinesis_event and push to _queue
			//   5. Notify _queue_cv
			//
			// The kinesis* instance is available via:
			//   auto *self = static_cast<kinesis*>(ctx->GetUUID() ... )
			WRNPRN("[kinesis] asynccb called but enhanced fan-out not yet implemented");
		}
	};
};

