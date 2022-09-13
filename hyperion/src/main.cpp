#include <main.h>

#include <signal.h>
#include <mntent.h>
#include <pwd.h>
#include <sys/mount.h>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include <axon/config.h>
#include <axon/log.h>
#include <axon/database.h>
#include <axon/oracle.h>
#include <axon/util.h>

#include <node.h>
#include <cluster.h>
#include <sendmail.h>

static std::string _base;
static std::string _log_path;

axon::log logger;
axon::config cfg;

dbconf dbc;
mailconf mc;

cluster overlord;
hyperion::sendmail sm;

uid_t uid;

void sigman(int, siginfo_t*, void*);
int install_signal_manager();

bool create_ramdisk(size_t mb, std::string location)
{return true; /// <--------------------------- remove this
	uid_t ruid, euid, suid;

	if (mb > 16384)
	{
		logger.print("FATAL", "tmpfs cannot be more than 16GB. aborting!");
		return false;
	}

	getresuid(&ruid, &euid, &suid);

	logger.print("INFO", "size: %d, location: %s, euid: %d, ruid: %d, suid: %d", mb, location.c_str(), euid, ruid, suid);

	if (euid == 0)
	{
		FILE *fp;
		bool exists = false;

		if (!axon::helper::exists(location))
		{
			logger.print("FATAL", "mount point does not exist. aborting!");
			return false;
		}

		if (!(fp = setmntent("/proc/mounts", "r")))
		{
			logger.print("FATAL", "cannot open /proc/mount. aborting!");
			return false;
		}

		while (true)
		{
			struct mntent mnt;
			char buf[PATH_MAX * 3];

			if (getmntent_r(fp, &mnt, buf, sizeof(buf)) == NULL)
				break;
			
			if (location.compare(mnt.mnt_dir) == 0 && strcmp(mnt.mnt_type, "tmpfs") == 0)
				exists = true;

			// std::cout<<"location: "<<mnt.mnt_dir<<", type:"<<mnt.mnt_type<<std::endl;
		}

		endmntent(fp);
		
		if (!exists)
		{
			char options[128];

			sprintf(options, "size=%luM,uid=%d,gid=%d,mode=700", mb, uid, uid);
			logger.print("INFO", "options => %s", options);
			if (mount("tmpfs", location.c_str(), "tmpfs", MS_NOSUID|MS_NOATIME|MS_NODEV|MS_NODIRATIME|MS_NOEXEC, &options) != 0)
			{
				logger.print("FATAL", "mount() failed, errCode=%d, reason=%s", errno, strerror(errno));
				return false;
			}
		}
	}
	else
	{
		logger.print("FATAL", "effective user do not have super-user previlage! aborting");
		return false;
	}

	return true;
}

bool setup()
{
	char *s;
	int x;

	cfg.reload();

	s = cfg.get("runuser");
	struct passwd *pw = getpwnam(s);
	
	if (pw == NULL)
	{
		logger.print("FATAL", "cannot determine runtime userid from name! aborting");
		return false;
	}

	uid = pw->pw_uid;

	s = cfg.get("base");
	if (!axon::helper::makedir(s))
	{
		logger.print("FATAL", "cannot access or create base directory");
		return false;
	}
	_base = s;

	cfg.open("log");
	s = cfg.get("location");
	_log_path = (s[0]=='/')?s:_base+"/"+s;
	if (!axon::helper::makedir(_log_path.c_str()))
	{
		logger.print("FATAL", "cannot access or create log directory");
		return false;
	}
	logger[AXON_LOG_PATH] = _log_path;
	s = cfg.get("file");
	logger[AXON_LOG_FILENAME] = s;
	logger.open();
	cfg.close();

	cfg.open("ramdisk");
	if (cfg.get("enabled"))
	{
		x = cfg.get("size");
		s = cfg.get("location");
		if (!create_ramdisk(x, s))
			return false;
		overlord.set(CLUSTER_CFG_BUFFER, s);
	}
	cfg.close();

	cfg.open("database");
	dbc.load(cfg);
	cfg.close();

	cfg.open("mail");
	mc.load(cfg);
	sm[SENTINEL_SENDMAIL_SERVER] = mc.server;
	sm[SENTINEL_SENDMAIL_FROM] = mc.username;
	sm[SENTINEL_SENDMAIL_TEMPLATE] = mc.body;
	sm[SENTINEL_SENDMAIL_LOGO] = mc.logo;
	cfg.close();

	overlord.set(logger);
	overlord.set(dbc);

	cfg.open("nodes");
	// s = cfg.get("path");
	// std::string workpath = (s[0]=='/')?s:_base+"/"+s;
	// if (!axon::helper::makedir(workpath.c_str()))
	// {
	// 	logger.print("FATAL", "cannot access or create node base directory");
	// 	return false;
	// }
	// overlord.set(CLUSTER_CFG_WORKPATH, workpath);
	overlord.load(cfg);
	cfg.close();
#ifdef DEBUG
	overlord.print();
#endif

	return true;
}

int install_signal_manager()
{
	struct sigaction action;
	int retval;

	logger.print("INFO", "overlord - installing signal manager.");

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_sigaction = &sigman;
	action.sa_flags = SA_SIGINFO;

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);

#ifndef DEBUG
	signal (SIGSEGV, SIG_IGN);
	if ((retval = sigaction(SIGSEGV, &action, NULL)) == -1)
	{
		logger.print("ERROR", "overlord - cannot attach signal manager for SIGSEGV (%d), cannot continue.", retval);
		return false;
	}
#endif

	if ((retval = sigaction(SIGHUP, &action, NULL)) == -1)
	{
		logger.print("ERROR", "overlord - cannot attach signal manager for SIGHUP (%d), cannot continue.", retval);
		return false;
	}

	if ((retval = sigaction(SIGTERM, &action, NULL)) == -1)
	{
		logger.print("ERROR", "overlord - cannot attach signal manager for SIGTERM (%d), cannot continue.", retval);
		return false;
	}

	if ((retval = sigaction(SIGINT, &action, NULL)) == -1)
	{
		logger.print("ERROR", "overlord - cannot attach signal manager for SIGINT (%d), cannot continue.", retval);
		return false;
	}

	return true;
}

void sigman(int signum, siginfo_t *siginfo, void *context)
{
	logger.print("WARNING", "overlord - received signal, disabling further signal processing.");

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);
#ifndef DEBUG
	signal (SIGSEGV, SIG_IGN);
#endif
	switch (signum)
	{
#ifndef DEBUG
		case SIGSEGV:
			logger.print("FATAL", "overlord - Something caused SEGFAULT please investigate, shutting down processes...");
			overlord.killall();
			break;
#endif
		case SIGTERM:
		case SIGINT:
			logger.print("WARNING", "overlord - SIGTERM received, shutting down processes...");
			overlord.killall();
			break;

		case SIGHUP:
			logger.print("WARNING", "overlord - SIGHUP received, reloading configuration...");
			overlord.reload();
			setup();
			overlord.start();
			install_signal_manager();
			break;
	}
}

int main(int argc, char *argv[])
{
	int hcfg = false;
	char logfile[PATH_MAX] = "sentine.log";

	while (--argc > 0 && (*++argv)[0] == '-')
	{
		char *p = argv[0] + 1;
		int flag = *p++;

		if (flag == '\0' || *p == '\0')
		{
			if (argc > 1 && argv[1][0] != '-')
			{
				p = *++argv;
				argc -= 1;
			}
			else
				p = (char *) "";
		}

		switch (flag)
		{
			case 'c':
				try {
					cfg.load(p);
				} catch (axon::exception &e) {
					logger.print("FATAL", "%s", e.what());
					return -128;
				}
				hcfg = true;
				break;

			case 'l':
				strcpy(logfile, p);
				break;

			default:
				std::cerr << "Invalid option: -" << flag << std::endl;
				return 100;
				break;
		}
	}

	if (!hcfg)
	{
		logger.print("FATAL", "overlord - Configuration file name missing, consider using %s -c filename.conf", argv[0]);
		return 127;
	}

	int pid;

	//if ((pid = fork()) == 0) // detaching from terminal and going to background
	{
		try {
#if DEBUG == 0
			freopen("/dev/null", "w", stderr); // disabling stderr. this is to prevent pre-post script errors to bleed into terminal
#endif
			if (!setup())
			{
				logger.print("FATAL", "setup process failed. aborting!");
				return -1;
			}

			overlord.start(uid);

			if (!install_signal_manager())
			{
				logger.print("FATAL", "cannot install signal manager. aborting!");
				return -2;
			}

			std::thread th_main(&cluster::pool, std::ref(overlord)); // 
			th_main.join();

		} catch (axon::exception &e) {
			logger.print("FATAL", "Exception: %s", e.what());
		}
	}

	return 0;
}