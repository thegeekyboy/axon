#include <axon.h>
#include <axon/message.h>

int main()
{

	axon::message msg;

	try {

		axon::msg blah;
		axon::msg blah2;

		blah.id = 1;
		blah.type = 0;
		sprintf(blah.buffer, "This is a test msg");

		msg.open();
		//for (int x = 0; x < 11; x++)
		msg<<blah;
		msg>>blah2;
		//if (msg.get(blah))
			std::cout<<blah2.buffer;

	} catch (axon::exception &e) {

		std::cout<<"Internal Error: "<<e.what()<<std::endl;
		sleep(10);
	}

	msg.close();
}