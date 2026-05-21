#include <axon.h>
#include <axon/kinesis.h>

#include <aws/kinesis/KinesisClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <aws/kinesis/model/ListShardsRequest.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/SubscribeToShardRequest.h>

#include <aws/kinesis/model/GetRecordsRequest.h>

#include <aws/kinesis/model/ListStreamsRequest.h>
#include <aws/kinesis/model/ListStreamsResult.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/CreateStreamRequest.h>
#include <aws/kinesis/model/DeleteStreamRequest.h>

#include <aws/kinesis/model/ListStreamConsumersRequest.h>
#include <aws/kinesis/model/RegisterStreamConsumerRequest.h>
#include <aws/kinesis/model/RegisterStreamConsumerResult.h>
#include <aws/kinesis/model/DeregisterStreamConsumerRequest.h>
#include <aws/kinesis/model/DescribeStreamConsumerRequest.h>
#include <aws/kinesis/model/DescribeStreamConsumerResult.h>

namespace axon {

	namespace stream {

		std::string kinesis::stream::_get_stream_arn(std::string name)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			boost::regex pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/([A-Za-z0-9_.-]{1,128})$", boost::regex::extended);
			boost::smatch what;

			bool matched = boost::regex_match(name, what, pattern);

			Aws::Kinesis::Model::DescribeStreamRequest request;

			if (matched)
				request.SetStreamARN(name);
			else
				request.SetStreamName(name);

			Aws::Kinesis::Model::DescribeStreamOutcome outcome = _client->DescribeStream(request);

			if (!outcome.IsSuccess() && outcome.GetError().GetErrorType() != Aws::Kinesis::KinesisErrors::RESOURCE_NOT_FOUND)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			if (outcome.GetError().GetErrorType() == Aws::Kinesis::KinesisErrors::RESOURCE_NOT_FOUND)
				return {};

			if (matched) _name = what[1];

			return outcome.GetResult().GetStreamDescription().GetStreamARN();
		};

		std::vector<axon::stream::kinesis::partition> kinesis::stream::get_stream_partitions(std::string arn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			Aws::Kinesis::Model::ListShardsRequest request;

			request.SetStreamARN(arn);

			Aws::Kinesis::Model::ListShardsOutcome outcome = _client->ListShards(request);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			std::vector<partition> part;

			for (const auto& shard : outcome.GetResult().GetShards())
			{
				Aws::Kinesis::Model::GetShardIteratorRequest getShardIteratorRequest;

				getShardIteratorRequest.SetStreamARN(arn);
				getShardIteratorRequest.SetShardId(shard.GetShardId());
				getShardIteratorRequest.SetShardIteratorType(Aws::Kinesis::Model::ShardIteratorType::LATEST);

				Aws::Kinesis::Model::GetShardIteratorOutcome getShardIteratorOutcome = _client->GetShardIterator(getShardIteratorRequest);

				if (!getShardIteratorOutcome.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, getShardIteratorOutcome.GetError().GetMessage());

				part.emplace_back(shard.GetShardId(), getShardIteratorOutcome.GetResult().GetShardIterator());
			}

			return part;
		}

		kinesis::stream::stream(std::shared_ptr<Aws::Kinesis::KinesisClient> client, std::string name, axon::stream::cbfn callback = nullptr):
		_client(client), _id(axon::util::uuid()), _name(name), _ready(false), _sync(false), _busy(false), _callback(callback)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			DBGPRN("%s %s stream creating", _id.c_str(), _name.c_str());

			if (name.empty()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream name cannot be empty");

			_arn = _get_stream_arn(name);
			if (!_arn.empty()) {
				_partition = get_stream_partitions(_arn);
				if (_partition.size()) _ready = true;
			}
		}

		kinesis::stream::stream(const kinesis::stream& other):
		_client(other._client), _partition(other._partition), _id(axon::util::uuid()), _name(other._name), _ready(other._ready), _sync(other._sync), _busy(other._busy), _callback(other._callback)
		{
			DBGPRN("%s copy invoked from %s to %s", _name.c_str(), _id.c_str(), other._id.c_str());
			if (!_ready) _partition.clear();
		}

		void kinesis::stream::create()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_ready) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream already exists");

			Aws::Kinesis::Model::CreateStreamRequest request;

			request.SetStreamName(_name);
			request.SetShardCount(1);

			Aws::Kinesis::Model::CreateStreamOutcome outcome = _client->CreateStream(request);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			_arn = _get_stream_arn(_name);
			if (!_arn.empty()) {
				_partition = get_stream_partitions(_arn);
				if (_partition.size()) _ready = true;
			}
		}

		void kinesis::stream::remove()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			if (!_ready) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "kinesis stream does not exists");

			Aws::Kinesis::Model::DeleteStreamRequest request;

			request.SetStreamARN(_arn);
			request.SetEnforceConsumerDeletion(true);

			Aws::Kinesis::Model::CreateStreamOutcome outcome = _client->DeleteStream(request);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			_arn = {};
			_partition.clear();
			_ready = false;
		}

		std::string kinesis::consumer::attach(std::shared_ptr<Aws::Kinesis::KinesisClient> client, std::string arn, std::string name)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			Aws::Kinesis::Model::RegisterStreamConsumerRequest request;

			request.SetStreamARN(arn);
			request.SetConsumerName(name);

			Aws::Kinesis::Model::RegisterStreamConsumerOutcome outcome = client->RegisterStreamConsumer(request);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			return outcome.GetResult().GetConsumer().GetConsumerARN();
		}

		bool kinesis::consumer::detach(std::shared_ptr<Aws::Kinesis::KinesisClient> client, std::string arn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			Aws::Kinesis::Model::DeregisterStreamConsumerRequest request;

			request.SetConsumerARN(arn);

			Aws::Kinesis::Model::DeregisterStreamConsumerOutcome outcome = client->DeregisterStreamConsumer(request);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, outcome.GetError().GetMessage());

			return true;
		}

		bool kinesis::consumer::_is_attached(std::string arn)
		{
			return (std::find(_arn.begin(), _arn.end(), arn) != _arn.end());
		}

		bool kinesis::consumer::_validate_consumer_arn(std::string arn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			Aws::Kinesis::Model::DescribeStreamConsumerRequest request;

			request.SetConsumerARN(arn);

			Aws::Kinesis::Model::DescribeStreamConsumerOutcome outcome = _client->DescribeStreamConsumer(request);

			if (!outcome.IsSuccess()) return false;

			return true;
		}

		std::string kinesis::consumer::_get_consumer_arn(std::string stream_arn, std::string consumer_name)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
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

		kinesis::consumer::consumer(std::shared_ptr<Aws::Kinesis::KinesisClient> client, std::string name, bool fanout = true):
		_client(client), _name(name), _fanout(fanout), _ready(false), _count(0)
		{
			boost::regex pattern("^[A-Za-z0-9_.-]{1,128}$");

			if (!boost::regex_match(name, pattern)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "invalid consumer name");
			if (client == nullptr) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "shared client not valid");
		}

		std::string kinesis::consumer::attach(std::string arn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			boost::regex c_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/([A-Za-z0-9_.-]{1,128})\\/consumer\\/([A-Za-z0-9_.-]{1,128}):\\d{10,}$", boost::regex::extended);
			boost::regex s_pattern("^arn:aws:kinesis:[A-Za-z0-9-]+:\\d{12}:stream\\/[A-Za-z0-9_.-]{1,128}$");
			boost::smatch what;

			std::string retarn;

			if (boost::regex_match(arn, what, c_pattern))
			{
				if (!_is_attached(arn))
				{
					if (!_validate_consumer_arn(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer arn not valid");

					if (_name != what[1])  throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer name do not match");

					_arn.push_back(arn);
					_count++;
				}

				retarn = arn;
			}
			else if (boost::regex_match(arn, s_pattern))
			{
				retarn = _get_consumer_arn(arn, _name);

				if (retarn.empty()) retarn = attach(_client, arn, _name);

				if (!_is_attached(retarn)) {
					_arn.push_back(retarn);
					_count++;
				}
			}
			else throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream arn not in correct format");

			_ready = true;

			return retarn;
		}

		std::string kinesis::consumer::attach(axon::stream::kinesis::stream &ks)
		{
			if (!ks.ready()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream is not ready");
			if (ks.busy()) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "stream is already busy");

			return attach(ks.arn());
		};

		void kinesis::consumer::detach()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			DBGPRN("consumer count = %d", _count);

			for (int i = 0; i < _count; i++) {
				DBGPRN("removing => %s", _arn[i].c_str());
				if (!_arn[i].empty()) detach(_client, _arn[i]);
			}

			_arn.clear();
			_count = 0;
			_ready = false;
		}

		bool kinesis::consumer::detach(std::string arn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			axon::stream::ARN arntype = axon::stream::resolve(arn);

			if (arntype == axon::stream::ARN::UNKNOWN) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "arn is not in correct format");

			if (!_is_attached(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer not connected");

			if (arntype == axon::stream::ARN::STREAM) arn = _get_consumer_arn(arn, _name);

			if (!_validate_consumer_arn(arn)) throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "consumer arn not valid");

			if (detach(_client, arn))
			{
				_arn.erase(std::remove(_arn.begin(), _arn.end(), arn), _arn.end());
				if (!(_count = _arn.size())) _ready = false;
				return true;
			}

			return false;
		}

		bool kinesis::consumer::detach(axon::stream::kinesis::stream &ks)
		{
			return detach(ks.arn());
		};

		// KINESIS

		void kinesis::_stop()
		{

		}

		std::unique_ptr<axon::recordset> kinesis::get()
		{
			if (_records.empty()) return nullptr;

			std::unique_ptr<axon::recordset> p = std::move(_records.front());
			_records.pop();

			return p;
		}

		kinesis::kinesis(std::string hostname, std::string username, std::string password, uint16_t port):
		interface(hostname, username, password, port), _aws(axon::AwsStack()), _sync(true), _consumer_count(0)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

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

			Aws::Kinesis::Model::ListStreamsRequest listStreamsRequest;

			listStreamsRequest.SetLimit(1);
			Aws::Kinesis::Model::ListStreamsOutcome outcome = _client->ListStreams(listStreamsRequest);

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] " + outcome.GetError().GetMessage());

			_connected = true;
			_streams.reserve(10);
		}

		kinesis::kinesis(std::string hostname, std::string username, std::string password)
		: kinesis(hostname, username, password, 443)
		{
		}

		kinesis::~kinesis()
		{
			axon::timer ctm(__PRETTY_FUNCTION__);

			if (_connected) {
				_consumer->detach();
				_connected = false;
			}

			if (_runner.joinable())
				_runner.join();
		}

		void kinesis::subscribe()
		{
			if (!_consumer) _consumer = std::make_unique<axon::stream::kinesis::consumer>(_client, _name);

			for (auto& t : _topic)
			{
				axon::stream::kinesis::stream ks(_client, t.name, t.callback); // <-- may need some rework

				if (ks) {
					if (!_sync) _consumer->attach(ks);
					_streams.emplace_back(std::move(ks));
				}
				else {
					WRNPRN("%s topic does not exist", t.name.c_str());
				}
			}
		}

		void kinesis::unsubscribe()
		{
			//_client->DisableRequestProcessing();
		}

		bool kinesis::start()
		{
			if (_runnable) return false;

			return start(_callback);
		}

		bool kinesis::start(axon::stream::cbfn callback)
		{
			if (_runnable) return false;

			_daemon = std::thread([this] {

				//std::vector<std::thread> th; <- cannot remember why I put this here

				while (th.size())
				{
					for (std::thread& t : th)
					{
						if (t.joinable()) {
							t.join();
							axon::util::rm_thread(th, t.get_id());
						}
					}
				}

				while (_runnable)
				{

				}
			});

			_runner = std::thread([this, &callback] () {

				while (_runnable)
				{
					std::unique_ptr<axon::recordset> rc = get();

					if (!rc) break;
					if (callback) callback(std::move(rc));

					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}
			});

			return true;
		}

		std::unique_ptr<axon::recordset> kinesis::next()
		{
			return get();
		}

	};
};