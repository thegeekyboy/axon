#ifndef AXON_S3_H_
#define AXON_S3_H_

#define AXON_TRANSFER_S3_PROXY    'A'
#define AXON_TRANSFER_S3_ENDPOINT 'B'

#include <sstream>

#include <aws/core/Aws.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/GetBucketPolicyRequest.h>

#include <aws/s3/model/Object.h>
#include <aws/s3/model/ListObjectsRequest.h>

#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/clouddirectory/model/GetObjectAttributesResult.h>

#include <aws/s3/model/PutObjectRequest.h>

namespace axon
{
	namespace transfer
	{
		class s3 : public connection {

			Aws::SDKOptions _options;
			Aws::S3::S3Client *_client;

			std::unique_ptr<Aws::S3::Model::GetObjectOutcome> _getobject;
			std::unique_ptr<Aws::S3::Model::PutObjectRequest> _putobject;

			std::shared_ptr<std::stringstream> _buffer;

			std::string _endpoint, _proxy;

			static std::atomic<int> _instance;
			static std::mutex _lock;

			bool init();
			bool push(axon::transfer::connection&);

		public:
			s3(std::string, std::string, std::string, uint16_t);
			s3(std::string, std::string, std::string);
			s3(const s3& rhs) : connection(rhs) {  };
			~s3();

			bool set(char, std::string);

			bool connect();
			bool disconnect();

			bool chwd(std::string);
			std::string pwd();
			bool mkdir(std::string);
			int list(const axon::transfer::cb &);
			int list(std::vector<entry> &);

			long long copy(std::string, std::string, bool);
			long long copy(std::string src, std::string dest) { return copy(src, dest, false); };
			bool ren(std::string, std::string);
			bool del(std::string);

			int cb(const struct entry *);

			long long get(std::string, std::string, bool);
			long long put(std::string, std::string, bool);

			bool open(std::string, std::ios_base::openmode);
			bool close();

			ssize_t read(char*, size_t);
			ssize_t write(const char*, size_t);
		};
	}
}

#endif
