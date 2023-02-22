#ifndef AXON_MSGQUE_H_
#define AXON_MSGQUE_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MSGMAX 10
#define MSGQUESIZE 4096
#define MSGSIZE 2048
#define QUEUE_NAME "/axonmsg"

#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __PRETTY_FUNCTION__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \

namespace axon {

	struct msg {

		int id;
		int type;
		char buffer[MSGSIZE];
	};

	class message {

		mqd_t _mq;
		struct mq_attr _attr;

	public:

		message();
		~message();

		bool open(); //throw (axon::exception);
		bool close();
		bool remove();

		bool send(msg &);
		bool get(msg &);

		message& operator>>(msg &);
		message& operator<<(msg &);
	};
}

#endif
