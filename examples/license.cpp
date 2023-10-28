#include <axon.h>
#include <sys/utsname.h>
#include <mntent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <blkid/blkid.h>

#include <axon/dmi.h>
#include <axon/md5.h>

namespace axon
{
	class license
	{
		std::string _bios, _mac, _volume, _hostname;
		std::string _uuid, _key;
		uint8_t _hash[16];

		bool finddev(char *, unsigned int);

	public:
		license();
		~license();

		std::string hostname();
		bool volid();
		bool bios();
		bool keygen();
	};

	license::license()
	{

	}

	license::~license()
	{

	}

	bool license::finddev(char *pathname, unsigned int devnum)
	{
		DIR *dir;
		struct dirent *elem;
		struct stat buf;
		int pathlen = 0;

		if ((dir = opendir(pathname)) == NULL)
			return false;

		pathlen = strlen(pathname);

		while ((elem = readdir(dir)) != NULL)
		{
			if (!strcmp(elem->d_name, ".") || !strcmp(elem->d_name, ".."))
				continue;
			
			if (pathlen + 1 + strlen(elem->d_name) > PATH_MAX)
				continue;
			
			pathname[pathlen] = '/';
			
			strcpy(pathname + pathlen + 1, elem->d_name);
			
			if (lstat(pathname, &buf) < 0)
				continue;

			if ((buf.st_mode & S_IFMT) == S_IFBLK && buf.st_rdev == devnum)
			{
				closedir(dir);
				return true;
			}
			
			if ((buf.st_mode & S_IFMT) == S_IFDIR && finddev(pathname, devnum))
			{
				pathname[pathlen] = 0;
				return true;
			}
		}

		pathname[pathlen] = 0;
		closedir(dir);

		return false;
	}

	std::string license::hostname()
	{
		static struct utsname u;

		if (uname(&u) < 0)  
			return "unknown";

		return u.nodename;
	}

	bool license::volid()
	{
		struct stat buf;
		int i;
		blkid_probe probe;
		const char *uuid;

		if ((i = stat("/", &buf)) == 0)
		{

			char devname[2048];
			bzero(devname, 2048);
			strcpy(devname, "/dev");

			if (finddev(devname, buf.st_dev))
			{
				if ((probe = blkid_new_probe_from_filename(devname)))
				{
					blkid_do_probe(probe);
					blkid_probe_lookup_value(probe, "UUID", &uuid, NULL);
					blkid_free_probe(probe);
					_volume = uuid;
				}
				else
					throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Could not open device");
			}
		}
		else
			throw axon::exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Could not extract root mount point status");

		return true;
	}

	bool license::bios()
	{
		axon::identity::dmi d;
		_bios = d.getuuid();

		return true;
	}

	bool license::keygen()
	{
		md5_state_t state;
		md5_byte_t digest[16];
		//uint8_t in[16], out[16];
		char hexbyte[3], seed[2048];
		int cnt, cdn;

		bzero(seed, 2048);
		strncpy(seed, _uuid.c_str(), _uuid.size());

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *) seed, strlen(seed));
		md5_finish(&state, digest);

		for (cnt = 0, cdn = 15; cnt < 16; cnt++, cdn--)
		{
			sprintf(hexbyte, "%02X", digest[cnt]);
			_key += hexbyte;
			_hash[cnt] = digest[cdn];
		}

		return true;
	}
}

int main()
{
	axon::license lcs;

	try {

		//std::cout<<"Return: "<<lcs.volid()<<std::endl;
		if (lcs.bios())
		{
			std::cout<<"DMI Query successful!"<<std::endl;
		}
		else if (lcs.volid())
		{
			std::cout<<"VolumeID Query successful!"<<std::endl;
		}
		else
			std::cout<<"All Failed!!"<<std::endl;

	} catch (axon::exception& e) {

		std::cout<<e.what()<<std::endl;
	}

	return 0;
}