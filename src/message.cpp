#include <axon.h>
#include <axon/message.h>

namespace axon
{
	message::message()
	{
		_attr.mq_flags = 0;
		_attr.mq_maxmsg = MSGMAX;
		_attr.mq_msgsize = MSGQUESIZE;
		_attr.mq_curmsgs = 0;
	}

	message::~message()
	{
	}

	bool message::open() //throw(axon::exception)
	{
		if ((_mq = mq_open(QUEUE_NAME, O_RDWR | O_CREAT | O_NONBLOCK, 0644, &_attr)) == (mqd_t) -1)
		{
			std::string _errstr = strerror(errno);
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot open message queue, " + _errstr);
		}

		return true;
	}

	bool message::close()
	{
		if (_mq != (mqd_t) -1)
		{
			mq_close(_mq);
			return true;
		}

		return false;
	}

	bool message::remove()
	{
		if (_mq != (mqd_t) -1)
		{
			mq_unlink(QUEUE_NAME);
			return true;
		}

		return false;
	}

	bool message::send(msg &buf)
	{
		if (mq_send(_mq, (char *) &buf, sizeof(buf), 0) < 0)
		{
			std::string _errstr = strerror(errno);
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot send message, " + _errstr);
		}

		return true;
	}

	bool message::get(msg &buf)
	{
		if (mq_receive(_mq, (char *) &buf, MSGQUESIZE + 1, NULL) < 0)
			if (errno != EAGAIN)
				return false;

		return true;
	}

	message& message::operator>>(msg &buf)
	{
		int retval;

		if ((retval = mq_receive(_mq, (char *) &buf, MSGQUESIZE + 1, NULL)) < 0)
		{
			if (errno != EAGAIN)
			{
				std::string _errstr = strerror(errno);
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot read message, " + _errstr);
			}
		}

		return *this;
	}

	message& message::operator<<(msg &buf)
	{
		if (mq_send(_mq, (char *) &buf, sizeof(buf), 0) < 0)
		{
			std::string _errstr = strerror(errno);
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "Cannot send message, " + _errstr);
		}

		return *this;
	}
}