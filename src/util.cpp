#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <tuple>
#include <random>
#include <filesystem>
#include <bitset>

#include <cxxabi.h>

#include <magic.h>
#include <boost/filesystem.hpp>

#include <axon.h>
#include <version.h>
#include <axon/util.h>
#include <axon/md5.h>
#include <axon/aes.h>

std::mutex printer_safety;

void RAWDBG(int code, const char *format, ...)
{
	std::lock_guard<std::mutex> lock(printer_safety);
	va_list argptr;
	char refmt[2048];

	snprintf(refmt, 2048, "\033[0;%dm[%s] %s\033[0m\n", code, axon::timer::iso8601().c_str(), format);
	va_start(argptr, format);
	vfprintf(stderr, refmt, argptr);
	fflush(stderr);
	va_end(argptr);
}

namespace axon
{
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

		static inline void ltrim(std::string &s)
		{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
		}

		static inline void rtrim(std::string &s)
		{
			s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch);	}).base(), s.end());
		}

		static inline void trim(std::string &s)
		{
			rtrim(s);
			ltrim(s);
		}

		unsigned long long bytestoull(const char *bcd, const size_t size)
		{
			std::stringstream ss;

			for (size_t i = 0; i < size; i++)
			{
				std::bitset<8> x(bcd[i]);
				ss<<x;
			}
			std::bitset<64> bits(ss.str());

			return bits.to_ullong();
		}

		std::string bytestodecstring(const char *bcd, const size_t size)
		{
			std::stringstream ss;

			for (size_t i = 0; i < size; i++)
			{
				std::bitset<8> x(bcd[i]);
				ss<<x;
			}

			return ss.str();
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

		std::tuple<std::string, std::string> splitpath(std::string path)
		{
			// size_t found = path.find_last_of("/\\");
			// std::string parent = (path.substr(0, found).size() == 0 && path[0] == '/')?"/":path.substr(0, found);
			// return std::make_tuple(parent, path.substr(found + 1));
			return std::make_tuple(std::filesystem::path(path).parent_path(), std::filesystem::path(path).filename());
		}

		int _mkdir(const std::string& path, mode_t mode = 0700)
		{
			std::string parent, remainder;

			if (path == "/")
			{

				return 0;
			}

			std::tie(parent, remainder) = axon::util::splitpath(path);

			std::cin.get();

			return _mkdir(parent, mode);
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
				else
					return false;
			}
			else
			{
				if (exists(path))
				{
					if (access(path.c_str(), W_OK) == 0)
						return true;
					else
						return false;
				}
				else
				{
					std::string folder, file;
					std::tie (folder, file) = splitpath(path);

					if (access(folder.c_str(), W_OK) == 0)
						return true;
					else
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

			return true;
		}

		bool exists(const std::string& filename, const std::string& path)
		{
			char fname[PATH_MAX];

			if (path.size())
			{
				sprintf(fname, "%s/%s", path.c_str(), filename.c_str());
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

		std::tuple<std::string, std::string> magic(std::string& filename)
		{
			struct magic_set *mcookie;
			std::string mimetype, encoding;

			if ((mcookie = magic_open(MAGIC_MIME|MAGIC_CHECK)) != NULL)
			{
				if (magic_load(mcookie, NULL) == 0)
				{
					mimetype = magic_file(mcookie, filename.c_str());
					std::vector<std::string> bits = axon::util::split(mimetype, ';');
					if (bits.size()>1)
					{
						trim(bits[0]);
						trim(bits[1]);
					}
					mimetype = bits[0];
					encoding = bits[1];
				}
				magic_close(mcookie);
			}
			return { mimetype, encoding };
		}

		bool execmd(const char *cmd)
		{
			FILE *fp;
			char final[PATH_MAX], ch;
			std::stringstream ss;

			sprintf(final, "%s 2>&1", cmd);

			if((fp = popen(final, "w")) == NULL)
				return false;

			while((ch = fgetc(fp)) != EOF)
				ss<<ch;

			pclose(fp);

			return true;
		}

		std::string demangle(const char* mangled)
		{
			int status;

			std::unique_ptr<char[], void (*)(void*)> result(abi::__cxa_demangle(mangled, 0, 0, &status), std::free);

			return result.get() ? std::string(result.get()) : "error occurred";
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
			static std::random_device dev;
			static std::mt19937 rng(dev());

			std::uniform_int_distribution<int> dist(0, 15);

			const char *v = "0123456789abcdef";
			const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

			std::string res;
			for (int i = 0; i < 16; i++)
			{
				if (dash[i]) res += "-";
					res += v[dist(rng)];
				res += v[dist(rng)];
			}

			return res;
		}

		double random(double lower, double upper)
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution<double> dist(lower, upper);

			return dist(mt);
		}

		void debugprint(const char *format, ...)
		{
			va_list argptr;
			char refmt[2048];

			snprintf(refmt, 2048, "\033[0;33m%s\033[0m\n", format);
			va_start(argptr, format);
			vfprintf(stderr, refmt, argptr);
			va_end(argptr);
		}

		std::string protoname(axon::proto_t i)
		{
			switch (i)
			{
				case axon::protocol::UNKNOWN:
					return "axon::protocol::UNKNOWN";
					break;

				case axon::protocol::NOTHING:
					return "axon::protocol::NOTHING";
					break;

				case axon::protocol::FILE:
					return "axon::protocol::FILE";
					break;

				case axon::protocol::SFTP:
					return "axon::protocol::SFTP";
					break;

				case axon::protocol::FTP:
					return "axon::protocol::FTP";
					break;

				case axon::protocol::S3:
					return "axon::protocol::S3";
					break;

				case axon::protocol::SAMBA:
					return "axon::protocol::SAMBA";
					break;

				case axon::protocol::SCP:
					return "axon::protocol::SCP";
					break;

				case axon::protocol::HDFS:
					return "axon::protocol::HDFS";
					break;

				case axon::protocol::DATABASE:
					return "axon::protocol::DATABASE";
					break;

				case axon::protocol::KAFKA:
					return "axon::protocol::KAFKA";
					break;

				default:
					return "axon::protocol::UNKNOWN";
					break;
			}

			return "axon::protocol::UNKNOWN";
		}

		axon::proto_t protoid(std::string& name)
		{
			if (name == "NOTHING" || name == "AXON::PROTOCOL::NOTHING")
				return axon::protocol::NOTHING;
			else if (name == "FILE" || name == "AXON::PROTOCOL::FILE")
				return axon::protocol::FILE;
			else if (name == "SFTP" || name == "AXON::PROTOCOL::SFTP")
				return axon::protocol::SFTP;
			else if (name == "FTP" || name == "AXON::PROTOCOL::FTP")
				return axon::protocol::FTP;
			else if (name == "S3" || name == "AXON::PROTOCOL::S3")
				return axon::protocol::S3;
			else if (name == "SAMBA" || name == "AXON::PROTOCOL::SAMBA")
				return axon::protocol::SAMBA;
			else if (name == "SCP" || name == "AXON::PROTOCOL::SCP")
				return axon::protocol::SCP;
			else if (name == "HDFS" || name == "AXON::PROTOCOL::HDFS")
				return axon::protocol::HDFS;
			else if (name == "DATABASE" || name == "AXON::PROTOCOL::DATABASE")
				return axon::protocol::DATABASE;
			else if (name == "KAFKA" || name == "AXON::PROTOCOL::KAFKA")
				return axon::protocol::KAFKA;

			return axon::protocol::UNKNOWN;
		}

		std::string authname(axon::auth_t i)
		{
			switch (i)
			{
				case axon::authtype::UNKNOWN:
					return "axon::authtype::UNKNOWN";
					break;

				case axon::authtype::PASSWORD:
					return "axon::authtype::PASSWORD";
					break;

				case axon::authtype::PRIVATEKEY:
					return "axon::authtype::PRIVATEKEY";
					break;

				case axon::authtype::KERBEROS:
					return "axon::authtype::KERBEROS";
					break;

				case axon::authtype::NTLM:
					return "axon::authtype::NTLM";
					break;

				default:
					return "axon::authtype::UNKNOWN";
					break;
			}

			return "axon::protocol::UNKNOWN";
		}

		axon::auth_t authid(std::string& name)
		{
			if (name == "PASSWORD" || name == "AXON::AUTHTYPE::PASSWORD")
				return axon::authtype::PASSWORD;
			else if (name == "PRIVATEKEY" || name == "AXON::AUTHTYPE::PRIVATEKEY")
				return axon::authtype::PRIVATEKEY;
			else if (name == "KERBEROS" || name == "AXON::AUTHTYPE::KERBEROS")
				return axon::authtype::KERBEROS;
			else if (name == "NTLM" || name == "AXON::AUTHTYPE::NTLM")
				return axon::authtype::NTLM;

			return axon::protocol::UNKNOWN;
		}
	}
}
