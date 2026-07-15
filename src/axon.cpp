#include <axon.h>
#include <axon/version.h>

#include <csignal>

void global_signal_trap (int sig)
{
	switch (sig)
	{
		case SIGPIPE:
			ERRPRN("SIGPIPE signal was raised. ignoring for the time.");
			break;

		default:
			ERRPRN("Unknown signal %d was raised!", sig);
	}
	
	fflush(stderr);
}

__attribute__((constructor)) void axon_init_library()
{
#if DEBUG >= 1
	fprintf(stdout, "\033[33maxon %s - loading...\033[0m\n", VERSION);
#endif
}

__attribute__((destructor)) void axon_fini_library()
{
#if DEBUG >= 1
	fprintf(stdout, "\033[33maxon %s - unloading...\033[0m\n", VERSION);
#endif
}