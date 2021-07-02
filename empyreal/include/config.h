#ifndef BLAH
#define BLAH

#define DEBUG(fmt, ...) print(fmt, ##__VA_ARGS__)

#define CFG_E_FILEMISSING 0x0001
#define CFG_E_FILEPARSE 0x0002

#define CFG_FILE_PATH 'a'
#define CFG_FILE_NAME 'b'

#define CFG_LOG_PATH 'b'
#define CFG_LOG_FILENAME 'c'
#define CFG_LOG_LEVEL 0x0001

#define CFG_DB_FILENAME 'd'
#define CFG_DB_USERNAME 'e'
#define CFG_DB_PASSWORD 'f'
#define CFG_DB_GTT 'g'
#define CFG_DB_FILELIST 'h'
#define CFG_DB_ERROR 'i'
#define CFG_DB_TYPE 0x0002

class config {

	std::string _path, _filename, _errstr;
	int _errno;

	std::string _version, _key, _validity, _basedir;
	int _mode;

	std::string _log_path, _log_filename;
	int _log_level;

	std::string _db_path, _db_name, _db_username, _db_password;
	int _type;

	config_t _cfg;
	config_setting_t *_settings;

public:

	config();
	config(std::string);
	~config();

	int load(std::string);
	int reload();
	std::string path();

	int errno();
	std::string errstr();

	config_setting_t *settings();

	// bool populate(tcn::database::sqlite *);
	// bool populate(logger *);
	// bool populate(nodes *);

	std::string operator[] (char);
	int operator[] (int);
};

#endif