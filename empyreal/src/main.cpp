#include <main.h>

int main(int argc, char *argv[])
{
	config cfg;
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
			if (!cfg.load(p))
			{
				std::cerr << "Some error: " << cfg.errstr() << std::endl;
				return 126;
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

	std::mutex mu;
	
	logger log(&mu, cfg[CFG_LOG_FILENAME], cfg[CFG_LOG_PATH], cfg[CFG_LOG_LEVEL]);
	// tcn::database::sqlite db(cfg);
	// nodes tree(cfg);

	//tree.init();
	//sleep(5);
	return 1;

	try
	{

		std::vector<tcn::entry> x;

		// ftp conn("10.151.51.84", "raagent", "rapasswd");
		tcn::transport::transfer::ftp conn("10.10.40.239", "amirul.islam", "PASSWORD_HERE");

		conn.connect();

		try
		{
			conn.chwd("/bakmed/data/srcbak/HRT-MSC");
			// conn.chwd("/bakmed/temp");
		}
		catch (cException &e)
		{
			std::cout << "Path not found error" << std::endl;
		}

		conn.filter(".*123.*");
		conn.list(&x);

		for (int i = 0; i < x.size(); i++)
			std::cout << x[i].name << std::endl;

		// std::cout<<"Count: "<<x.size()<<std::endl;

		// conn.del("test.bin");
		// conn.put("test.bin", "test.2");
		// conn.ren("test.2", "test.bin");
		// conn.get("test.bin", "test.bin", true);

		//conn.list(blah);

		// for (int z = 0; z < x.size(); z++)
		// 	conn.get(x[z].name, x[z].name, true);

		conn.disconnect();
	}
	catch (cException &e)
	{

		std::cout << "Internal Error: " << e.what() << std::endl;
	}

	return 0;
}