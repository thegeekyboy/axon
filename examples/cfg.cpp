#include <axon.h>
#include <axon/config.h>

int printcfg(axon::config& cfg, int level)
{
	for (int i = 0; i < cfg.size(); i++)
	{
		axon::conftype ct = cfg.type(i);
		std::cout<<i<<"*"<<ct<<">>";

		if (ct == axon::STRING)
		{
			char *cval = cfg.get(i);
			std::string sname = cfg.name(i);
			std::cout<<std::string(level, ' ')<<sname<<" = "<<cval<<std::endl;
		}
		else if (ct == axon::INTEGER)
		{
			int x = cfg.get(i);
			std::string sname = cfg.name(i);
			std::cout<<std::string(level, ' ')<<sname<<" = "<<x<<std::endl;
		}
		else if (ct == axon::GROUP) {
			axon::config tcfg(cfg);
			tcfg.open(cfg.name(i));
			std::cout<<std::string(level, ' ')<<cfg.name(i)<<std::endl;
			printcfg(tcfg, level+1);
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	axon::config cfg, cfg2;

	try {

		// char *temp;
		// int mode;

		// cfg.load("saved.cfg");
		// printcfg(cfg, 0); return 0;

		if (argc > 1)
			cfg.load(argv[1]);
		else
			return 0;
		// cfg.load("ship.cfg");
		// cfg.save("saved_before.cfg");

		// cfg.open("nodes");
		// cfg.rewind();
		cfg.print(NULL, 0);
		return 0;

		// printcfg(cfg, 0);
		// std::cout<<"$$ "<<cfg.name(0)<<std::endl;
		// std::cout<<"1. size: "<<cfg.size()<<std::endl;
		cfg.open("nodes");
		// std::cout<<"$$ "<<cfg.name(0)<<std::endl;
		cfg2.load("add.cfg");
		cfg2.open("doggers");
		
		// std::cout<<"2. size: "<<cfg.size()<<std::endl;
		cfg2.open(cfg2.name(0));
		cfg.add(cfg2.raw());
		cfg2.close();
		// std::cout<<"3. size: "<<cfg.size()<<std::endl;
		cfg2.open(cfg2.name(1));
		cfg.add(cfg2.raw());
		cfg2.close();

		// std::cout<<"4. size: "<<cfg.size()<<std::endl;
		cfg.close();
		// std::cout<<"5. size: "<<cfg.size()<<std::endl;
		// cfg.rewind();
		// std::cout<<"$$ "<<cfg.name(0)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(1)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(2)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(3)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(4)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(5)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(6)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(7)<<std::endl;
		// std::cout<<"$$ "<<cfg.name(8)<<std::endl;
		// std::cout<<"$$ "<<cfg.name()<<std::endl;
		std::cout<<"------------------------"<<std::endl;
		cfg.print(NULL, 0);

		// cfg.save("saved_after.cfg");
		// axon::config cfgX = cfg;
		printcfg(cfg, 0);
		
		// cfg.open("nodes");
		// std::cout<<"step 1"<<std::endl;
		// cfg.open("ramdisk");
		// std::cout<<"step 2"<<std::endl;
		// axon::config tcfg0(cfg);
		// std::cout<<"step 3"<<std::endl;
		// tcfg0.open("mark");
		// std::cout<<"step 4"<<std::endl;
		// axon::config tcfg1(tcfg0);
		// std::cout<<"step 5"<<std::endl;
		// cfg.open("huamsc");
		// axon::config cfg2(cfg);
		// cfg.back();
		// cfg.back();
		// cfg.back();

		// for (int i = 0; i < cfg2.size(); ++i)
		// {
		// 	if (cfg2.type(i) == axon::STRING)
		// 	{
		// 		char *value;
		// 		std::string name;

		// 		temp = cfg2.get(i);
		// 		name = cfg2.name();

		// 		std::cout<<name<<": "<<temp<<std::endl;
		// 	}
		// }
		// std::cout<<cfg2.size()<<std::endl;

	} catch (axon::exception& e) {

		std::cout<<e.what()<<std::endl;
	}

	return 0;
}
