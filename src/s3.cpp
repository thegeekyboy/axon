#include <fstream>
#include <chrono>
#include <thread>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>

#include <axon.h>
#include <axon/connection.h>
#include <axon/s3.h>
#include <axon/util.h>

namespace axon
{
	namespace transfer
	{
		std::atomic<int> s3::_instance = 0;
		std::mutex s3::_mtx;

		s3::s3(std::string hostname, std::string username, std::string password, uint16_t port)
		: connection(hostname, username, password, port)
		{
		}

		s3::~s3()
		{
			disconnect();
			DBGPRN("[%s] connection %s class dying.", _id.c_str(), axon::util::demangle(typeid(*this).name()).c_str());
		}

		bool s3::init()
		{
			std::lock_guard<std::mutex> guard(_mtx);

			if (_instance <= 0)
			{
				//_options = new Aws::SDKOptions;
				Aws::InitAPI(_options);
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}
			_instance++;

			return true;
		}

		bool s3::set(char c, std::string value)
		{
			switch (c)
			{
				case AXON_TRANSFER_S3_PROXY:
					_proxy = value;
					break;

				case AXON_TRANSFER_S3_ENDPOINT:
					_endpoint = value;
					break;
			}

			return true;
		}

		bool s3::connect()
		{
			init();

			Aws::Auth::AWSCredentials auth = Aws::Auth::AWSCredentials(_username, _password);
			Aws::Client::ClientConfiguration cfg;
			bool useVirtualAdressing = true;

			// TODO:
			// need to figure out how to disable IMDS query, specially when non-AWS service
			// provider like minio. for the time export AWS_EC2_METADATA_DISABLED="true" for
			// environment variable as workaround.
			// https://github.com/aws/aws-sdk-ruby/issues/2174

			if (_endpoint.size() > 2)
			{
				cfg.endpointOverride = Aws::String(_endpoint);
				cfg.region = Aws::Region::AWS_GLOBAL;
				useVirtualAdressing = false;
			}
			else
				cfg.region = Aws::Region::AP_SOUTHEAST_1;

			if (_proxy.size() > 3)
			{
				std::vector<std::string> proxy = axon::util::split(_proxy, ':');
				cfg.proxyHost = proxy[0];
				if (proxy.size() > 1) cfg.proxyPort = std::stoi(proxy[1]);
				cfg.proxyScheme = Aws::Http::Scheme::HTTP;
			}

			_client = new Aws::S3::S3Client(auth, cfg, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, useVirtualAdressing);

			auto outcome = _client->ListBuckets();

			if (!outcome.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] " + std::string("there was an error connecting to S3 - ") + outcome.GetError().GetExceptionName().c_str());

			_connected = true;

			return true;
		}

		bool s3::disconnect()
		{
			std::lock_guard<std::mutex> guard(_mtx);

			if (_connected)
			{
				delete _client;

				_instance--;
				if (_instance <= 0)
				{
					Aws::ShutdownAPI(_options);
					// delete _options;
				}

				_connected = false;
			}

			return true;
		}

		bool s3::chwd(std::string path)
		{
			DBGPRN("[%s] requested s3::chwd() = %s", _id.c_str(), path.c_str());
			if (path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid path");

			if (path.substr(0, 1) == "." || path.substr(0, 2) == "..")
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] relative path not supported");

			// TODO: implement relative path

			std::vector<std::string> parts = axon::util::split(path, '/');

			std::string bucket = parts[0];

			Aws::S3::Model::HeadBucketRequest request;
			request.SetBucket(bucket);
			auto result = _client->HeadBucket(request);

			if (!result.IsSuccess())
			{
				auto err = result.GetError();
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] could not change directory - " +bucket + " " + err.GetMessage());
			}

			_path = (path[path.size()] = '/')?path:path+"/";


			return false;
		}

		std::string s3::pwd()
		{
			DBGPRN("[%s] requested s3::pwd()", _id.c_str());
			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			return _path;
		}

		bool s3::mkdir(std::string dir)
		{
			DBGPRN("[%s] requested s3::mkdir() = %s", _id.c_str(), dir.c_str());

			if (dir.size() < 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid directory name");

			return true;
		}

		long long s3::copy(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			DBGPRN("[%s] requested s3::copy() = %s to %s", _id.c_str(), src.c_str(), dest.c_str());
			std::string srcx, destx;
			long long filesize;

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			auto [path, filename] = axon::util::splitpath(srcx);

			if (src == dest || srcx == dest || path == dest || filename == dest)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source and destination object cannot be same for copy operation");

			{
				Aws::S3::Model::HeadObjectRequest request;
				request.WithBucket("").WithKey(destx);
				auto result = _client->HeadObject(request);

				if (result.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] destination object already exist");
			}
			{
				Aws::S3::Model::HeadObjectRequest request;
				request.WithBucket("").WithKey(srcx);
				auto result = _client->HeadObject(request);

				if (!result.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] source object does not exist");

				Aws::S3::Model::HeadObjectResult fileinfo = result.GetResult();
				filesize = fileinfo.GetContentLength(); 
			}

			Aws::S3::Model::CopyObjectRequest request;

			request.WithCopySource(srcx)
				.WithBucket("")
				.WithKey(destx);

			auto response = _client->CopyObject(request);

			if (!response.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] object copy failed");

			return filesize;
		}

		bool s3::ren(std::string src, std::string dest)
		{
			DBGPRN("[%s] requested s3::chwd() = %s to %s", _id.c_str(), src.c_str(), dest.c_str());
			std::string srcx, destx;
			std::string parent, remainder;

			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			copy(srcx, destx, false);
			del(srcx);

			return true;
		}

		bool s3::del(std::string target)
		{
			DBGPRN("[%s] requested s3::del() = %s", _id.c_str(), target.c_str());
			std::string targetx;

			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (target[0] == '/')
				targetx = target;
			else
				targetx = _path + "/" + target;

			{
				Aws::S3::Model::HeadObjectRequest request;
				request.WithBucket("").WithKey(targetx);
				auto result = _client->HeadObject(request);

				if (!result.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] object does not exist");

				Aws::S3::Model::HeadObjectResult fileinfo = result.GetResult();
			}

			Aws::S3::Model::DeleteObjectRequest request;

			request.WithBucket("").WithKey(targetx);
			auto response = _client->DeleteObject(request);

			if (!response.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] object delete error");

			return true;
		}

		int s3::list(const axon::transfer::cb &cbfn)
		{
			axon::timer ctm(__PRETTY_FUNCTION__);
			long count = 0;
			bool done = false;

			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			std::vector<std::string> parts = axon::util::split(_path, '/');

			std::string bucket = parts[0];
			std::string prefix;

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "/";

			DBGPRN("S3: listing %s %s", bucket.c_str(), prefix.c_str());

			Aws::S3::Model::ListObjectsRequest request;
			request.WithBucket(bucket).WithPrefix(prefix);

			while (!done)
			{
				auto response = _client->ListObjects(request);

				if (!response.IsSuccess())
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error generating list");

				Aws::Vector<Aws::S3::Model::Object> objects = response.GetResult().GetContents();

				for (auto const &object : objects)
				{
					struct entry file;

					file.et = axon::protocol::S3;
					file.flag = axon::flags::FILE;
					file.size = object.GetSize();

					file.name = object.GetKey();
					size_t pos = file.name.find(prefix);
					if (pos != std::string::npos)
						file.name.erase(pos, prefix.size());

					if (match(file.name))
					{
						cbfn(file);
						count++;
					}
				}

				done = !response.GetResult().GetIsTruncated();
				if (!done)
					request.SetMarker(response.GetResult().GetContents().back().GetKey());
			}

			return count;
		}

		int s3::list(std::vector<axon::entry> &vec)
		{
			return list([&](const axon::entry &e) mutable {

				vec.push_back(e);
			});
		}

		long long s3::get(std::string src, std::string dest, bool compress)
		{
			DBGPRN("[%s] requested s3::get() = %s to %s", _id.c_str(), src.c_str(), dest.c_str());
			std::string srcx;
			Aws::S3::Model::GetObjectRequest request;

			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (src[0] == '/')
				srcx = src;
			else
				srcx = _path + "/" + src;

			std::vector<std::string> parts = axon::util::split(srcx, '/');

			std::string bucket = parts[0];
			std::string prefix;

			if (parts.size() < 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid path and/or object");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "/";

			if (prefix.size() > 2)
				prefix.pop_back();

			request.WithBucket(bucket).WithKey(prefix);
			Aws::S3::Model::GetObjectOutcome result = _client->GetObject(request);

			if (!result.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error downloading object " + result.GetError().GetMessage());

			Aws::S3::Model::GetObjectResult& details = result.GetResult();

			const long long filesize = details.GetContentLength();

			std::ofstream file;
			file.exceptions(std::ofstream::badbit);

			try {
				file.open(dest, std::ios::out | std::ios::binary);

				if (compress)
				{
					boost::iostreams::filtering_ostream out;

					out.push(boost::iostreams::bzip2_compressor());
					out.push(file);

					out<<result.GetResult().GetBody().rdbuf();
				}
				else 
					file<<result.GetResult().GetBody().rdbuf();
			} catch (const std::ofstream::failure& e) {
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error opening file to writing - " + dest);
			}

			return filesize;
		}

		long long s3::put(std::string src, std::string dest, [[maybe_unused]] bool compress)
		{
			// TODO: Need to implement compress
			DBGPRN("[%s] requested s3::put() = %s to %s", _id.c_str(), src.c_str(), dest.c_str());
			// TODO: https://stackoverflow.com/questions/59526181/multipart-upload-s3-using-aws-c-sdk
			std::string destx;
			Aws::S3::Model::PutObjectRequest request;

			if (_path.size() <= 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] path not initialized");

			if (dest[0] == '/')
				destx = dest;
			else
				destx = _path + "/" + dest;

			std::vector<std::string> parts = axon::util::split(destx, '/');

			std::string bucket = parts[0];
			std::string prefix;

			if (parts.size() < 2)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] invalid path and/or object");

			for (unsigned int i = 1; i < parts.size(); i++)
				prefix += parts[i] + "/";

			if (prefix.size() > 2)
				prefix.pop_back();

			request.WithBucket(bucket).WithKey(prefix);

			auto data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream", src, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
			request.SetBody(data);

			const long long filesize = data->tellg();
			auto result = _client->PutObject(request);

			if (!result.IsSuccess())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] error uploading object");

			return filesize;
		}
	}
}
