#ifndef AXON_NOTHING_H_
#define AXON_NOTHING_H_

#define AXON_TRANSFER_NOTHING_PROXY    'A'
#define AXON_TRANSFER_NOTHING_ENDPOINT 'B'

namespace axon
{
	namespace transfer
	{
		class nothing : public connection {

			public:
				nothing(std::string, std::string, std::string, uint16_t);
				nothing(std::string, std::string, std::string);
				nothing(const nothing&);
				~nothing();

				bool set(char, std::string);

				bool connect();
				bool disconnect();

				bool chwd(std::string);
				std::string pwd();
				bool mkdir(std::string);
				size_t list(const axon::transfer::cb &);
				size_t list(std::vector<entry> &);
				off_t copy(std::string, std::string, bool);
				off_t copy(std::string, std::string);
				bool ren(std::string, std::string);
				bool del(std::string);

				int cb(const struct entry *);

				off_t get(std::string, std::string, bool);
				off_t put(std::string, std::string, bool);

				bool open(std::string, std::ios_base::openmode);
				bool close();

				ssize_t read(char*, size_t);
				ssize_t write(const char*, size_t);
		};
	}
}

#endif

