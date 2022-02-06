#include <main.h>

#include <signal.h>

#include <axon/config.h>
#include <axon/log.h>
#include <axon/database.h>
#include <axon/oracle.h>

axon::log logger;
cluster overmind;

void sigman(int, siginfo_t*, void*);
int install_signal_manager();

int main(int argc, char *argv[])
{
	axon::config cfg;
	dbconf dbc;
	
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
				logger.print("FATAL", e.what());
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

	axon::database::oracle oradb;
	axon::database::interface *db = &oradb;
	
	db->connect("MDUATDB", "AMIRUL_0950", "Amirul095O");
	db->ping();
	db->version();

	db->close();

	if (!hcfg)
	{
		logger.print("FATAL", "Overmind - Configuration file name missing, consider using %s -c filename.conf", argv[0]);
		return 127;
	}

	try {

		char *s;

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

		overmind.set(logger);
		overmind.set(dbc);

		cfg.open("nodes");
		overmind.load(cfg);
		cfg.close();

		install_signal_manager();

		overmind.print();
		overmind.init();

	} catch (axon::exception &e) {
		
		logger.print("FATAL", e.what());
	} catch (...) {
		
		std::cout<<__FILE__<<": some kind of error happened!"<<std::endl;
	}

	return 0;
}

void sigman(int signum, siginfo_t *siginfo, void *context)
{
	logger.print("WARNING", "overlord - received signal, disabling further signal processing.");

	signal (SIGTERM, SIG_IGN);
	signal (SIGHUP, SIG_IGN);

	switch (signum)
	{
		case SIGTERM:
			logger.print("WARNING", "overlord - SIGTERM received, shutting down processes...");
			break;

		case SIGHUP:
			logger.print("WARNING", "overlord - SIGHUP received, reloading configuration...");
			break;
	}

	overmind.killall();
}

int install_signal_manager()
{
	struct sigaction action;
	int retval;

	logger.print("INFO", "overmind - installing signal manager.");

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_sigaction = &sigman;
	action.sa_flags = SA_SIGINFO;

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);

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