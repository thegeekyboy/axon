#ifndef AXON_CONFIG_H_
#define AXON_CONFIG_H_

#include <stack>
#include <queue>

#include <string.h>
#include <unistd.h>

#include <libconfig.h>

namespace axon
{

	enum conftype
	{
		MISSING = -1,
		UNKNOWN = 0,
		GROUP = 1,
		INTEGER = 2,
		INT64 = 3,
		FLOAT = 4,
		STRING = 5,
		BOOL = 6,
		ARRAY = 7,
		LIST = 8,
	};

	class config
	{
		std::string _filename, _error;

		config_t _cfg;
		config_setting_t *_root, *_master;

		bool _open;

		std::stack<std::string> _path;
		std::queue<std::string> _crumb;

		void deepcopy(const config &);
		std::string _name(const config_setting_t *);

	public:
		class proxy
		{
			int intval;
			char strval[1024];

		public:
			proxy(int intv, std::string strv)
			{
				intval = intv;
				strcpy(strval, strv.c_str());
			};
			operator int() const { return intval; }
			operator char *() const { return const_cast<char *>(strval); }
		};

		config();
		~config();

		config(const config &);
		config &operator=(const config &);
		const config_setting_t *raw();

		bool load(std::string);
		bool load();
		bool reload();
		bool rewind();

		bool save();
		bool save(std::string);

		std::string name();
		std::string name(int);
		bool exists(std::string path);
		bool open(std::string path);
		bool close();
		int size();
		int size(std::string);
		bool add(const config_setting_t *);
		bool add(const std::string&, axon::conftype);
		bool remove(std::string&);

		axon::conftype type(std::string);
		axon::conftype type(int);

		proxy get(std::string);
		proxy get(int);

		// via name
		bool get(std::string, int &);
		bool get(std::string, long long &);
		bool get(std::string, double &);
		bool get(std::string, std::string &);

		bool get(std::string, int, int &); // int array
		bool get(std::string, int, std::string &); // string array

		// via index
		bool get(int, int &);
		bool get(int, std::string &);

		void print(const config_setting_t *, int);
		void print();
	};

} // namespace axon
#endif