#include <main.h>
#include <config.h>

int main(int argc, char *argv[])
{
	axon::config cfg;
	int hcfg = false;

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
				p = "";
		}

		switch (flag)
		{
		case 'c':
			try {
				cfg.load(p);
			} catch (axon::exception &e) {
				std::cerr<<e.what()<<std::endl;
				return -128;
			}
			hcfg = true;
			break;

		default:
			std::cerr << "Invalid option: -" << flag << std::endl;

			return 100;
			break;
		}
	}

	if (!hcfg)
	{
		std::cerr << "Overmind - Configuration file name missing, consider using " << argv[0] << " -c filename.conf" << std::endl;
		return 127;
	}

	try
	{
	}
	catch (axon::exception &e)
	{
		std::cout << "Internal Error: " << e.what() << std::endl;
	}

	return 0;
}