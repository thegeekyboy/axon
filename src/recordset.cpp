#include <axon.h>
#include <axon/util.h>
#include <axon/recordset.h>

#include <iostream>

namespace axon {

	bool recordset::get_bool(std::string name)
	{
		bool value;

		if (_empty || _data.at(name).kind() == boost::json::kind::null)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "recordset is either empty or value is null");

		if (_data.at(name).kind() != boost::json::kind::bool_)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value requested is not of type: bool");

		value = _data.at(name).get_bool();

		return value;
	}

	int recordset::get_int(std::string name)
	{
		return get_long(name);
	}

	long recordset::get_long(std::string name)
	{
		long value;

		if (_empty || _data.at(name).kind() == boost::json::kind::null)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "recordset is either empty or value is null");

		if (_data.at(name).kind() != boost::json::kind::int64)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value requested is not of type: int/long");

		value = _data.at(name).get_int64();

		return value;
	}

	float recordset::get_float(std::string name)
	{
		return (float) get_double(name);
	}

	double recordset::get_double(std::string name)
	{
		double value;

		if (_empty || _data.at(name).kind() == boost::json::kind::null)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "recordset is either empty or value is null");

		if (_data.at(name).kind() != boost::json::kind::double_)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value requested is not of type: float/double");

		value = _data.at(name).get_double();

		return value;
	}

	std::string recordset::get_string(std::string name)
	{
		std::string value;

		if (_empty || _data.at(name).kind() == boost::json::kind::null)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "recordset is either empty or value is null");

		if (_data.at(name).kind() != boost::json::kind::string)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "value requested is not of type: string");

		value = _data.at(name).get_string().c_str();

		return value;
	}

	recordset::~recordset()
	{
		// DBGPRN("%s is terminating", _id.c_str());
	}

	recordset::recordset(std::string data):_empty(true), _id(axon::util::uuid()), _sdata(data)
	{
		_data = boost::json::parse(_sdata);
		_empty = false;
	}

	recordset::recordset(const unsigned char *data): recordset(reinterpret_cast<char*>(const_cast<unsigned char*>(data)))
	{
	}

	size_t recordset::get(std::string, void *, size_t)
	{
		return 0;
	}

	void recordset::print()
	{
		print(std::cout);
	}

	std::ostream& recordset::print(std::ostream &os)
	{
		os<<_sdata;
		return os;
	}
};