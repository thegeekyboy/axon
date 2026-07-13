#include <iostream>
#include <cstring>

#include <boost/json.hpp>

#include <axon.h>
#include <axon/util.h>
#include <axon/stream.h>

#include <axon/database.h>
#include <axon/resultset.h>

namespace axon {

	resultset::resultset(std::string name)
	: _id(axon::util::uuid()), _name(std::move(name))
	{
	}

	resultset::resultset(axon::database::connector &db, int batch_size)
	: _id(axon::util::uuid()), _database(&db), _batch_size(batch_size)
	{
	}

	resultset::resultset(axon::stream::connector &strm, int batch_size)
	: _id(axon::util::uuid()), _stream(&strm), _batch_size(batch_size)
	{
	}

	void resultset::add_column(std::string name, column_type type, bool nullable)
	{
		_schema.push_back({ std::move(name), type, nullable });
	}

	void resultset::begin_row()
	{
		_staging.clear();
		_staging.reserve(_schema.size());
	}

	void resultset::push_null()
	{
		_staging.push_back({ std::monostate{}, column_type::null_t });
	}

	void resultset::push_bool(bool v)
	{
		_staging.push_back({ v, column_type::bool_t });
	}

	void resultset::push_int(int64_t v)
	{
		_staging.push_back({ v, column_type::int64_t });
	}

	void resultset::push_double(double v)
	{
		_staging.push_back({ v, column_type::double_t });
	}

	void resultset::push_string(std::string v)
	{
		_staging.push_back({ std::move(v), column_type::string_t });
	}

	void resultset::push_bytes(std::vector<uint8_t> v)
	{
		_staging.push_back({ std::move(v), column_type::bytes_t });
	}

	void resultset::push_bytes(const void *buf, size_t len)
	{
		const uint8_t *p = static_cast<const uint8_t*>(buf);
		push_bytes(std::vector<uint8_t>(p, p + len));
	}

	void resultset::end_row()
	{
		while (_staging.size() < _schema.size())
			push_null();

		_rows.push_back(std::move(_staging));
		_staging.clear();
	}

	void resultset::from_json(std::string_view json)
	{
		boost::json::value jv = boost::json::parse(std::string(json));

		if (!jv.is_object())
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "from_json: top-level value must be a JSON object");

		const boost::json::object &obj = jv.get_object();

		begin_row();

		for (const auto &col : _schema)
		{
			auto it = obj.find(col.name);

			if (it == obj.end() || it->value().is_null())
			{
				push_null();
				continue;
			}

			const boost::json::value &v = it->value();

			switch (col.type)
			{
				case column_type::bool_t: push_bool(v.as_bool()); break;
				case column_type::int64_t: push_int(v.as_int64()); break;
				case column_type::double_t: push_double(v.as_double()); break;
				case column_type::string_t: push_string(std::string(v.as_string().c_str())); break;
				case column_type::bytes_t: {
					std::string b64(v.as_string().c_str());
					auto decoded = axon::util::base64_decode(b64);
					push_bytes(std::vector<uint8_t>(decoded.begin(), decoded.end()));
					break;
				}
				default: push_null(); break;
			}
		}

		end_row();
	}

	bool resultset::next()
	{
		BENCHMARK;

		if (!_rows.empty())
		{
			_current = std::move(_rows.front());
			_rows.pop_front();
			_bof = false;
			_col_cursor = 0;
			_row_count++;
			return true;
		}

		if (_eof) return false;

		if (_database)
		{
			_database->fetch(*this, _batch_size);
		}
		else if (_stream)
		{
			// _stream->fetch(*this, _batch_size); // TODO: this will come when stream class is adjusted
		}
		else
		{
			_eof = true;
			return false;
		}

		if (_rows.empty())
		{
			_eof = true;
			return false;
		}

		_current = std::move(_rows.front());
		_rows.pop_front();
		_bof = false;
		_col_cursor = 0;
		_row_count++;
		return true;
	}

	resultset& resultset::operator++()
	{
		next();
		return *this;
	}

	void resultset::done()
	{
		if (_database) _database->done();
		// if (_stream) _stream->done();

		_rows.clear();
		_current.clear();
		_staging.clear();
		_schema.clear();

		_schema_ready = false;
		_bof = true;
		_eof = false;
		_row_count = 0;
		_col_cursor = 0;
	}

	column_type resultset::type(size_t n) const
	{
		if (n >= _schema.size())
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");
		return _schema[n].type;
	}

	std::string_view resultset::name(size_t n) const
	{
		if (n >= _schema.size())
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");
		return _schema[n].name;
	}

	size_t resultset::index(std::string_view col) const
	{
		for (size_t i = 0; i < _schema.size(); i++)
			if (_schema[i].name == col) return i;
		throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column not found: " + std::string(col));
	}

	size_t resultset::get(std::string_view col, void *buf, size_t len) const
	{
		return get(index(col), buf, len);
	}

	size_t resultset::get(size_t n, void *buf, size_t len) const
	{
		_assert_row();
		const axon::field &f = _field_at(n);
		if (f.is_null()) return 0;
		if (f.type != column_type::bytes_t)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "get(bytes): column is not of type bytes");
		const auto &v = std::get<std::vector<uint8_t>>(f.value);
		size_t copy   = std::min(len, v.size());
		std::memcpy(buf, v.data(), copy);
		return copy;
	}

	std::string resultset::to_json() const
	{
		_assert_row();

		boost::json::object obj;

		for (size_t i = 0; i < _schema.size() && i < _current.size(); ++i)
		{
			const axon::field &f = _current[i];

			if (f.is_null()) { obj[_schema[i].name] = nullptr; continue; }

			switch (f.type)
			{
				case column_type::bool_t: obj[_schema[i].name] = std::get<bool>(f.value); break;
				case column_type::int64_t: obj[_schema[i].name] = std::get<int64_t>(f.value); break;
				case column_type::double_t: obj[_schema[i].name] = std::get<double>(f.value); break;
				case column_type::string_t: obj[_schema[i].name] = std::get<std::string>(f.value); break;
				case column_type::bytes_t: {
					const auto &v = std::get<std::vector<uint8_t>>(f.value);
					obj[_schema[i].name] = axon::util::base64_encode(std::string(v.begin(), v.end()));
					break;
				}
				default: obj[_schema[i].name] = nullptr; break;
			}
		}

		return boost::json::serialize(obj);
	}

	void resultset::print() const { print(std::cout); }

	std::ostream& resultset::print(std::ostream &os) const
	{
		if (_bof || _current.empty())
		{
			os << "(no current row)";
			return os;
		}

		for (size_t i = 0; i < _schema.size() && i < _current.size(); ++i)
		{
			if (i > 0) os << " | ";
			os << _schema[i].name << "=";

			const axon::field &f = _current[i];

			if (f.is_null()) { os << "NULL"; continue; }

			switch (f.type)
			{
				case column_type::bool_t: os << (std::get<bool>(f.value) ? "true" : "false"); break;
				case column_type::int64_t: os << std::get<int64_t>(f.value); break;
				case column_type::double_t: os << std::get<double>(f.value); break;
				case column_type::string_t: os << std::get<std::string>(f.value); break;
				case column_type::bytes_t: {
					const auto &v = std::get<std::vector<uint8_t>>(f.value);
					os << "[" << v.size() << " bytes]";
					break;
				}
				default: os << "?"; break;
			}
		}

		return os;
	}

	void resultset::_assert_row() const
	{
		if (_bof)
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "no current row — call next() first");
	}

	size_t resultset::_column_index(std::string_view col) const { return index(col); }

	const axon::field& resultset::_field_at(size_t n) const
	{
		if (n >= _current.size())
			throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "column index out of bounds");
		return _current[n];
	}

	template <typename T>
	bool resultset::_extract(const axon::field &f, T &out) const
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			if (f.type != column_type::bool_t)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch: expected bool");
			out = std::get<bool>(f.value);
		}
		else if constexpr (std::is_integral_v<T>)
		{
			if (f.type != column_type::int64_t)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch: expected integer got %d", f.type);
			out = static_cast<T>(std::get<int64_t>(f.value));
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if (f.type != column_type::double_t)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch: expected float/double got %d", f.type);
			out = static_cast<T>(std::get<double>(f.value));
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			if (f.type != column_type::string_t)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch: expected string");
			out = std::get<std::string>(f.value);
		}
		else if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
		{
			if (f.type != column_type::bytes_t)
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "type mismatch: expected bytes");
			out = std::get<std::vector<uint8_t>>(f.value);
		}
		else
		{
			return false;
		}

		return true;
	}

	template bool resultset::_extract(const axon::field&, bool&) const;
	template bool resultset::_extract(const axon::field&, int&) const;
	template bool resultset::_extract(const axon::field&, long&) const;
	template bool resultset::_extract(const axon::field&, long long&) const;
	template bool resultset::_extract(const axon::field&, float&) const;
	template bool resultset::_extract(const axon::field&, double&) const;
	template bool resultset::_extract(const axon::field&, std::string&) const;
	template bool resultset::_extract(const axon::field&, std::vector<uint8_t>&) const;
}