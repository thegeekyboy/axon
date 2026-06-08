#include <random>
#include <bitset>
#include <filesystem>
#include <algorithm>

#include <cxxabi.h>
#include <unistd.h>

#include <axon.h>
#include <axon/version.h>
#include <axon/util.h>
#include <axon/md5.h>
#include <axon/aes.h>

namespace axon
{
	std::mutex _debug_mtx;

	std::string version()
	{
		return VERSION;
	}

	namespace util
	{
		static const std::string base64_chars =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";

		struct magic_set *magic::cookie = NULL;
		static std::mutex thmtx;

		const boost::regex validator::regex_ipaddr{"(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)){3}"};
		const boost::regex validator::regex_fqdn{"^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$"};
		const boost::regex validator::regex_username{"^[A-Za-z0-9]+(?:[._-][A-Za-z0-9]+)*$"};

		unsigned long long bytes_to_ull(const char* bytes, size_t size)
		{
			unsigned long long result = 0;
			std::memcpy(&result, bytes, size);

			return __builtin_bswap64(result);
		}

		std::string bytes_to_binarystring(const char *bcd, const size_t size)
		{
			std::string result;
			result.reserve(size * 8);

			for (size_t i = 0; i < size; i++)
				for (int bit = 7; bit >= 0; bit--)
					result += ((static_cast<unsigned char>(bcd[i]) >> bit) & 1) ? '1' : '0';

			return result;
		}

		char *trim(char *str)
		{
			char *end;

			while(((unsigned char)*str) == ' ')
				str++;

			if(*str == 0)
				return str;

			end = str + strlen(str) - 1;

			while(end > str && isspace((unsigned char)*end))
				end--;

			*(end+1) = 0;

			return str;
		}

		std::vector<std::string> split(const std::string& str, const char delim)
		{
			std::vector<std::string> tokens;
			size_t prev = 0, pos = 0;

			do {

				pos = str.find(delim, prev);

				if (pos == std::string::npos)
					pos = str.length();

				std::string token = str.substr(prev, pos - prev);

				if (!token.empty())
					tokens.push_back(token);

				prev = pos + 1;

			} while (pos < str.length() && prev < str.length());

			return tokens;
		}

		std::string merge(std::vector<std::string> list, const char delim)
		{
			std::string merged = list.empty()?"":std::accumulate(++list.begin(), list.end(), *list.begin(), [delim](auto& a, auto& b) { return a + delim + b; });
			return merged;
		}

		std::tuple<std::string, std::string> splitpath(std::string path)
		{
			return std::make_tuple(std::filesystem::path(path).parent_path(), std::filesystem::path(path).filename());
		}

		std::tuple<std::string, std::string> splitbucket(std::string path)
		{
			std::string bucket, key = path;

			std::vector<std::string> parts = axon::util::split(path, '/');
			if (parts.size() >= 2) {
				bucket = parts[0];
				parts.erase(parts.begin());
				key = axon::util::merge(parts, '/');
			}

			return std::make_tuple(bucket, key);
		}

		std::string hash(const std::string& word)
		{
			std::string password;
			int i;
			unsigned long seed[2];
			char salt[] = "$1$........";
			const char *const seedchars = 
				"./0123456789ABCDEFGHIJKLMNOPQRST"
				"UVWXYZabcdefghijklmnopqrstuvwxyz";

			seed[0] = time(NULL);
			seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);


			for (i = 0; i < 8; i++)
				salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

			password = crypt(word.c_str(), salt);

			return password;
		}

		std::string md5(std::string& word)
		{
			md5_state_t state;
			md5_byte_t digest[16];

			std::stringstream ss;
			ss<<std::hex;

			md5_init(&state);
			md5_append(&state, (const md5_byte_t *) word.c_str(), word.size());
			md5_finish(&state, digest);

			for( int i(0); i < 16; ++i )
				ss<<std::setw(2)<<std::setfill('0')<<(int)digest[i];

			return ss.str();
		}

		bool iswritable(const std::string &path)
		{
			if (isdir(path))
			{
				if (access(path.c_str(), W_OK) == 0)
					return true;
				return false;
			}
			else
			{
				if (exists(path))
				{
					if (access(path.c_str(), W_OK) == 0)
						return true;
					return false;
				}
				else
				{
					std::string folder, file;
					std::tie (folder, file) = splitpath(path);

					if (access(folder.c_str(), W_OK) == 0)
						return true;
					return false;
				}
			}

			return false;
		}

		bool isdir(const std::string &path)
		{
			struct stat info;

			if(stat(path.c_str(), &info ) != 0)
				return false;
			else if(info.st_mode & S_IFDIR)
				return true;
			else
				return false;
		}

		bool isfile(const std::string &path)
		{
			struct stat info;

			if(stat(path.c_str(), &info ) != 0)
				return false;
			else if(info.st_mode & S_IFREG)
				return true;
			else
				return false;
		}

		bool exists(const std::string &path)
		{
			struct stat info;

			if(stat(path.c_str(), &info ) != 0)
				return false;

			if (access(path.c_str(), R_OK) != 0)
				return false;

			return true;
		}

		bool exists(const std::string& filename, const std::string& path)
		{
			char fname[PATH_MAX];

			if (path.size())
			{
				snprintf(fname, PATH_MAX - 1, "%s/%s", path.c_str(), filename.c_str());
				if (access(fname, F_OK) != -1)
					return true;
				else
					return false;
			} 
			else
			{
				if (access(filename.c_str(), F_OK) != -1)
					return true;
				else
					return false;
			}

			return true;
		}

		bool makedir(const char *dir)
		{
			char tmp[PATH_MAX];
			char *p = NULL;
			size_t len;

			if ((len = snprintf(tmp, sizeof(tmp), "%s", dir)) >= sizeof(tmp))
				return false;

			if (tmp[len - 1] == '/')
				tmp[len - 1] = 0;

			for (p = tmp + 1; *p; p++)
			{
				if (*p == '/')
				{
					*p = 0;
					if (mkdir(tmp, S_IRWXU) == -1)
						if (errno != EEXIST)
							return false;
					*p = '/';
				}
			}

			if (mkdir(tmp, S_IRWXU) == -1)
				if (errno != EEXIST)
					return false;

			return true;
		}

		bool execmd(const char *cmd)
		{
			FILE *fp;
			char final[PATH_MAX];

			snprintf(final, PATH_MAX, "%s 2>&1", cmd);

			if((fp = popen(final, "r")) == NULL)
				return false;

			pclose(fp);

			return true;
		}

		std::string demangle(const char* mangled)
		{
			int status;

			std::unique_ptr<char[], void (*)(void*)> result(abi::__cxa_demangle(mangled, 0, 0, &status), std::free);

			return result.get() ? std::string(result.get()) : "error occurred";
		}

		bool set_thread_name(pthread_t th, std::string name)
		{
			return !pthread_setname_np(th, name.c_str());
		}

		void rm_thread(std::vector<std::thread> &tl, std::thread::id id)
		{
			std::lock_guard<std::mutex> lock(thmtx);

			auto iter = std::find_if(tl.begin(), tl.end(), [=](std::thread &t) { return (t.get_id() == id); });
			if (iter != tl.end())
			{
				iter->detach();
				tl.erase(iter);
			}
		}

		static inline bool is_base64(BYTE c)
		{
			return (isalnum(c) || (c == '+') || (c == '/'));
		}

		std::string base64_encode(BYTE const* buf, unsigned int bufLen)
		{
			std::string ret;
			int i = 0;
			int j = 0;
			BYTE char_array_3[3];
			BYTE char_array_4[4];

			while (bufLen--)
			{
				char_array_3[i++] = *(buf++);

				if (i == 3)
				{
					char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
					char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
					char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
					char_array_4[3] = char_array_3[2] & 0x3f;

					for(i = 0; (i <4) ; i++)
						ret += base64_chars[char_array_4[i]];
					i = 0;
				}
			}

			if (i)
			{
				for(j = i; j < 3; j++)
					char_array_3[j] = '\0';

				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for (j = 0; (j < i + 1); j++)
					ret += base64_chars[char_array_4[j]];

				while((i++ < 3))
					ret += '=';
			}

			return ret;
		}

		std::string base64_encode(const std::string &in)
		{
			std::string out;

			int val = 0, valb = -6;

			for (u_char c : in)
			{
				val = (val << 8) + c;
				valb += 8;
				while (valb >= 0) {
					out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
					valb -= 6;
				}
			}

			if (valb>-6)
				out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);

			while (out.size()%4)
				out.push_back('=');

			return out;
		}

		std::vector<BYTE> base64_decode(std::string const& encoded_string)
		{
			int in_len = encoded_string.size();
			int i = 0;
			int j = 0;
			int in_ = 0;
			BYTE char_array_4[4], char_array_3[3];
			std::vector<BYTE> ret;

			while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
			{
				char_array_4[i++] = encoded_string[in_]; in_++;
				if (i ==4) 
				{
					for (i = 0; i <4; i++)
						char_array_4[i] = base64_chars.find(char_array_4[i]);

					char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
					char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
					char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

					for (i = 0; (i < 3); i++)
						ret.push_back(char_array_3[i]);
					i = 0;
				}
			}

			if (i)
			{
				for (j = i; j <4; j++)
					char_array_4[j] = 0;

				for (j = 0; j <4; j++)
					char_array_4[j] = base64_chars.find(char_array_4[j]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (j = 0; (j < i - 1); j++)
					ret.push_back(char_array_3[j]);
			}

			return ret;
		}

		std::string uuid()
		{
			thread_local std::mt19937 rng(std::random_device{}());

			std::uniform_int_distribution<int> dist(0, 15);

			static constexpr char v[] = "0123456789abcdef";
			static constexpr bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

			std::string res;
			res.reserve(36);

			for (int i = 0; i < 16; i++)
			{
				if (dash[i]) res += '-';

				res += v[dist(rng)];
				res += v[dist(rng)];
			}

			return res;
		}

		double random(double lower, double upper)
		{
			thread_local std::mt19937 rng(std::random_device{}());
			std::uniform_real_distribution<double> dist(lower, upper);

			return dist(rng);
		}
	}
}

