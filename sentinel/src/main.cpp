#include <main.h>

#include <signal.h>

#include <axon/config.h>
#include <axon/log.h>
#include <axon/database.h>
#include <axon/oracle.h>

#include <sendmail.h>

axon::log logger;
axon::config cfg;

dbconf dbc;
mailconf mc;

cluster overlord;
sentinel::sendmail sm;

void sigman(int, siginfo_t*, void*);
int install_signal_manager();

void setup()
{
	char *s;

	cfg.reload();

	cfg.open("log");
	s = cfg.get("location");
	logger[AXON_LOG_PATH] = s;
	s = cfg.get("file");
	logger[AXON_LOG_FILENAME] = s;
	logger.open();
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
	overlord.load(cfg);
	cfg.close();
#ifdef DEBUG
	overlord.print();
#endif
}

int main(int argc, char *argv[])
{

	// sm[SENTINEL_SENDMAIL_SERVER] = "smtp://exedge-st-2.bkash.com";
	// sm[SENTINEL_SENDMAIL_FROM] = "data@bkash.com";
	// sm[SENTINEL_SENDMAIL_TEMPLATE] = "/home/amirul.islam/development/medutils/sentinel/test/email.html";
	// sm[SENTINEL_SENDMAIL_LOGO] = "/home/amirul.islam/development/medutils/sentinel/test/bkash-orange.png";
	// sm[SENTINEL_SENDMAIL_TO] = "amirul.islam@bkash.com";
	// sm[SENTINEL_SENDMAIL_CC] = "alam.sarker@bkash.com";
	// sm[SENTINEL_SENDMAIL_SUBJECT] = "Test email";
	// sm[SENTINEL_SENDMAIL_BODY] = "This is a mail body";
	// sm.send();
	// return 0;

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

	if ((pid = fork()) == 0) // detaching from terminal and going to background
	{
		try {

			freopen("/dev/null", "w", stderr); // disabling stderr. this is to prevend pre-post script errors to bleed into terminal

			setup();
			overlord.start();
			install_signal_manager();

			std::thread th_main(&cluster::pool, std::ref(overlord)); // 
			th_main.join();

		} catch (axon::exception &e) {
			
			logger.print("FATAL", "Exception: %s", e.what());
		}
	}

	return 0;
}

void sigman(int signum, siginfo_t *siginfo, void *context)
{
	logger.print("WARNING", "overlord - received signal, disabling further signal processing.");

	signal (SIGSEGV, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);

	switch (signum)
	{
		case SIGSEGV:
			logger.print("FATAL", "overlord - Something caused SEGFAULT please investigate, shutting down processes...");
			overlord.killall();
			break;

		case SIGTERM:
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

int install_signal_manager()
{
	struct sigaction action;
	int retval;

	logger.print("INFO", "overlord - installing signal manager.");

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_sigaction = &sigman;
	action.sa_flags = SA_SIGINFO;

	signal (SIGSEGV, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);

	if ((retval = sigaction(SIGSEGV, &action, NULL)) == -1)
	{
		logger.print("ERROR", "overlord - cannot attach signal manager for SIGSEGV (%d), cannot continue.", retval);
		return false;
	}

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

	return true;
}