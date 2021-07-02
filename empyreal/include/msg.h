#ifndef MSGQUE_H_
#define MSGQUE_H_

#define MSGQUESIZE 2048

struct mq {

	int msgid;
	char message[MSGQUESIZE];
};

class msgque {

};

#endif
