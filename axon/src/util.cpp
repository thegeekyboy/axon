#include <chrono>
#include <tuple>

#include <boost/filesystem.hpp>

#include <axon.h>
#include <axon/util.h>
#include <axon/md5.h>
#include <axon/aes.h>

namespace axon
{
	static const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	unsigned int toInt(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return 10 + c - 'A';
		if (c >= 'a' && c <= 'f') return 10 + c - 'a';

		return c;
	}

	unsigned long getdate()
	{
		time_t rawtime;
		struct tm *timeinfo;

		rawtime = time (NULL);
		timeinfo = localtime(&rawtime);

		return ((timeinfo->tm_year+1900)*10000) + ((timeinfo->tm_mon+1)*100) + timeinfo->tm_mday;
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
		size_t found = path.find_last_of("/\\");
		std::string parent = (path.substr(0, found).size() == 0 && path[0] == '/')?"/":path.substr(0, found);
		return std::make_tuple(parent, path.substr(found + 1));
	}

	int mkdir(const std::string& path, mode_t mode = 0700)
	{
		std::string parent, remainder;

		if (path == "/")
		{

			return 0;
		}

		std::tie(parent, remainder) = axon::splitpath(path);
		
		std::cout<<">> parent: "<<parent<<", remainder: "<<remainder<<std::endl;
		std::cin.get();

		return mkdir(parent, mode);
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

	int license(struct licensekey *master)
	{
		md5_state_t state;
		md5_byte_t digest[16];
		uint8_t hash[16], in[16], out[16];
		char hex_output[16*2 + 1], output[1024], seed[2048];
		int cnt, cdn;
		FILE *fp;

		char s1[1024] = {0x2f, 0x75, 0x73, 0x72, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x68, 0x61, 0x6c, 0x2d, 0x67, 0x65, 0x74, 0x2d, 0x70, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x79, 0x20, 0x2d, 0x2d, 0x75, 0x64, 0x69, 0x20, 0x2f, 0x6f, 0x72, 0x67, 0x2f, 0x66, 0x72, 0x65, 0x65, 0x64, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2f, 0x48, 0x61, 0x6c, 0x2f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x73, 0x2f, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x72, 0x20, 0x2d, 0x2d, 0x6b, 0x65, 0x79, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x2e, 0x68, 0x61, 0x72, 0x64, 0x77, 0x61, 0x72, 0x65, 0x2e, 0x75, 0x75, 0x69, 0x64, 0x20, 0x32, 0x3e, 0x2f, 0x64, 0x65, 0x76, 0x2f, 0x6e, 0x75, 0x6c, 0x6c, 0x00};
		char s2[1024] = {0x2f, 0x73, 0x62, 0x69, 0x6e, 0x2f, 0x69, 0x66, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x20, 0x2d, 0x61, 0x20, 0x7c, 0x20, 0x67, 0x72, 0x65, 0x70, 0x20, 0x2d, 0x45, 0x20, 0x22, 0x28, 0x65, 0x6d, 0x31, 0x7c, 0x65, 0x74, 0x68, 0x30, 0x29, 0x22, 0x20, 0x7c, 0x20, 0x74, 0x72, 0x20, 0x2d, 0x73, 0x20, 0x27, 0x20, 0x27, 0x20, 0x7c, 0x20, 0x63, 0x75, 0x74, 0x20, 0x2d, 0x64, 0x27, 0x20, 0x27, 0x20, 0x2d, 0x66, 0x35, 0x00};
		//char s2[1024] = {0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x63, 0x61, 0x74, 0x20, 0x2f, 0x73, 0x79, 0x73, 0x2f, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x2f, 0x6e, 0x65, 0x74, 0x2f, 0x65, 0x6d, 0x31, 0x2f, 0x61, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x20, 0x32, 0x3e, 0x2f, 0x64, 0x65, 0x76, 0x2f, 0x6e, 0x75, 0x6c, 0x6c, 0x00}; //em0
		//char s2[1024] = {0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x63, 0x61, 0x74, 0x20, 0x2f, 0x73, 0x79, 0x73, 0x2f, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x2f, 0x6e, 0x65, 0x74, 0x2f, 0x65, 0x74, 0x68, 0x30, 0x2f, 0x61, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x20, 0x32, 0x3e, 0x2f, 0x64, 0x65, 0x76, 0x2f, 0x6e, 0x75, 0x6c, 0x6c, 0x00}; //eth0

		if ((fp = popen(s1, "r")) == NULL)
			return false;

		bzero(output, 1024);

		if (fgets(output, sizeof(output)-1, fp) == NULL)
			return false;

		pclose(fp);

		strcpy(seed, output);
		seed[strlen(seed)-1] = 0;

		if ((fp = popen(s2, "r")) == NULL)
			return false;

		bzero(output, 1024);

		if (fgets(output, sizeof(output)-1, fp) == NULL)
			return false;

		pclose(fp);

		strcat(seed, output);
		seed[strlen(seed)-1] = 0;

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *) seed, strlen(seed));
		md5_finish(&state, digest);

		for (cnt = 0, cdn = 15; cnt < 16; cnt++, cdn--)
		{
			sprintf(hex_output + cnt * 2, "%02x", digest[cnt]);
			hash[cnt] = digest[cdn];
		}
	#ifdef SHOWHASH

		printf("HashKey>%s\n", hex_output);

	#endif

		if (strcmp(master->key, hex_output) != 0)
		{
			//printlog("FATL", "Overmind - The license key is wrong, please collect correct licence key.");
			return false;
		}

		for (cnt = 0; cnt < 16; cnt++)
			in[cnt] = 16 * toInt(master->validity[cnt*2]) + toInt(master->validity[(cnt*2)+1]);

	#ifdef SHOWHASH

		printf("NewVali>");

		uint8_t test[16] = {0x20, 0x17, 0x06, 0x30, 0X46, 0x49, 0x4c, 0x45, 0x42, 0x4f, 0x54, 0x56, 0x30, 0x2e, 0x31, 0x30};
		AES128_ECB_encrypt(test, hash, out);
		for (cnt = 0; cnt < 16; cnt++)
			printf("%02x", out[cnt]);
			printf("\n");

	#endif

		AES128_ECB_decrypt(in, hash, out);

		int chksum = false;

		if (out[4] == 0x46 && out[5] == 0x49 && out[6] == 0x4c && out[7] == 0x45 && out[8] == 0x42 && out[9] == 0x4f && out[10] == 0x54 && out[11] == 0x56 && out[12] == 0x30 && out[13] == 0x2e && out[14] == 0x31 && out[15] == 0x30)
			chksum = true;

		if (!chksum)
		{
			//printlog("FATL", "Overmind - Data is corrupted. Please contact sisscons@yahoo.com");
			return false;
		}

		unsigned long expiredate = ((((out[0]/16)*10)+(out[0]%16))*1000000) + ((((out[1]/16)*10)+(out[1]%16))*10000) + ((((out[2]/16)*10)+(out[2]%16))*100) + (((out[3]/16)*10)+(out[3]%16));
		
		//printf("%d > %d\n", getdate(), expiredate);
		if (getdate() > expiredate)
		{
			//printlog("FATL", "Overmind - Your licence has expired, kindly get or renew your licence");
			return false;
		} else {
			//printlog("INFO", "Overmind - Your license will expire on: %02d-%02d-20%02d", (((out[3]/16)*10)+(out[3]%16)), (((out[2]/16)*10)+(out[2]%16)), (((out[1]/16)*10)+(out[1]%16)));
		}

		master->expiredate = expiredate;
		return true;
	}

	bool execmd(const char *cmd, const char *name)
	{
		FILE *fp;
		char final[PATH_MAX], ch;
		// char output[1024];
		// unsigned int index = 0;

		sprintf(final, "%s 2>&1", cmd);

		if((fp = popen(final, "w")) == NULL)
			return false;
		
		while((ch = fgetc(fp)) != EOF)
		{
			if (ch == '\n')
			{
				// printlog("STDO", "%s - %s", name, output);
				// index = 0;
			}
			// else
			// 	output[index++] = ch;
		}

		// if (index != 0)
		// 	printlog("STDO", "%s - %s", name, output);

		pclose(fp);

		return true;
	}

	std::string demangle(const char* mangled)
	{
		int status;

		std::unique_ptr<char[], void (*)(void*)> result(abi::__cxa_demangle(mangled, 0, 0, &status), std::free);
		
		return result.get() ? std::string(result.get()) : "error occurred";
	}


	static inline bool is_base64(BYTE c) {
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
}