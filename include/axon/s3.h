#ifndef AXON_S3_H_
#define AXON_S3_H_

#define AXON_TRANSFER_S3_PROXY    'A'
#define AXON_TRANSFER_S3_ENDPOINT 'B'

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
	namespace transport
	{
		namespace transfer
		{
			class s3 : public connection {

				Aws::SDKOptions _options;
				Aws::S3::S3Client *_client;
				std::string _endpoint;
				std::string _proxy;

				static std::atomic<int> _instance;
				static std::mutex _mtx;

				bool init();

			public:
				s3(std::string hostname, std::string username, std::string password, uint16_t port);
				s3(const s3& rhs) : connection(rhs) {  };
				~s3();

				bool set(char, std::string);

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				bool mkdir(std::string);
				int list(const axon::transport::transfer::cb &);
				int list(std::vector<entry> &);
				long long copy(std::string, std::string, bool = false);
				bool ren(std::string, std::string);
				bool del(std::string);

				int cb(const struct entry *);

				long long get(std::string, std::string, bool = false);
				long long put(std::string, std::string, bool = false);
			};
		}
	}
}

#endif