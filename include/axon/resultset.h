#ifndef AXON_RESULTSET_H_
#define AXON_RESULTSET_H_

#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <deque>
#include <ostream>
#include <cstdint>
#include <cstring>

#define AXON_RESULTSET_BATCH_SIZE 256

namespace axon {

	namespace database { class connector; }
	namespace stream { class connector; }

	enum class column_type : uint8_t {
		null_t = 0,
		bool_t = 1,
		int64_t = 2,
		double_t = 3,
		string_t = 4,
		bytes_t = 5
	};

	struct column_meta {
		std::string name;
		column_type type { column_type::null_t };
		bool nullable { true };
	};

	using field_value = std::variant<
		std::monostate,           // NULL
		bool,
		int64_t,
		double,
		std::string,
		std::vector<uint8_t>      // binary / BLOB
	>;

	struct field {
		field_value value;
		column_type type { column_type::null_t };
		bool is_null() const noexcept { return std::holds_alternative<std::monostate>(value); }
	};

	using record = std::vector<axon::field>;

	class resultset {

		std::string _id;
		std::string _name;
		axon::database::connector *_database { nullptr };
		axon::stream::connector *_stream { nullptr };


		std::vector<column_meta> _schema;
		bool _schema_ready { false };

		std::deque<axon::record> _rows;

		axon::record _staging;
		axon::record _current;

		bool _bof { true };
		bool _eof { false };
		size_t _col_cursor { 0 };   // for operator>>
		size_t _row_count { 0 };    // total rows consumed

		int _batch_size  { AXON_RESULTSET_BATCH_SIZE };

		void _assert_row() const;
		size_t _column_index(std::string_view) const;
		const axon::field& _field_at(size_t) const;

		template <typename T>
		bool _extract(const axon::field &, T &) const;

	public:

		explicit resultset(std::string name = {});
		explicit resultset(axon::database::connector&, int = AXON_RESULTSET_BATCH_SIZE);
		explicit resultset(axon::stream::connector&, int = 1);

		resultset(const axon::resultset&) = delete;
		resultset& operator=(const axon::resultset&) = delete;
		~resultset() = default;

		void add_column(std::string, column_type, bool = true);

		void begin_row();
		void end_row();

		void push_null();
		void push_bool (bool);
		void push_int (int64_t);
		void push_double(double);
		void push_string(std::string);
		void push_bytes (std::vector<uint8_t>);
		void push_bytes (const void *buf, size_t);

		void from_json(std::string_view);
		void set_eof() { _eof = true; }

		bool next();
		axon::resultset& operator++();

		void done();

		size_t count() const noexcept { return _schema.size(); }
		size_t rows() const noexcept { return _row_count; }
		size_t buffered() const noexcept { return _rows.size(); }
		bool empty() const noexcept { return _bof && _rows.empty(); }
		bool is_empty() const noexcept { return empty(); }
		bool eof() const noexcept { return _eof && _rows.empty(); }

		column_type type (size_t) const;
		std::string_view name (size_t) const;
		size_t index (std::string_view) const;

		std::string_view source() const noexcept { return _name; }
		void set_source(std::string val) { _name = std::move(val); }

		template <typename T>
		bool get(size_t n, T &out) const {

			_assert_row();
			const axon::field &f = _field_at(n);
			if (f.is_null()) return false;
			return _extract(f, out);
		}

		template <typename T>
		bool get(std::string_view col, T &out) const { return get(index(col), out); }

		size_t get(size_t, void *, size_t ) const;
		size_t get(std::string_view, void *, size_t) const;

		template <typename T>
		axon::resultset& operator>>(T &out) {

			if (_col_cursor >= _schema.size())
				throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "operator>>: all columns consumed for this row. _col_cursor: %d, _schema.size(): %d", _col_cursor, _schema.size());
			get(_col_cursor++, out);
			return *this;
		}

		std::string to_json() const;

		void print() const;
		std::ostream& print(std::ostream &) const;

		friend std::ostream& operator<<(std::ostream &os, const axon::resultset &rc) { return rc.print(os); }
		friend std::ostream& operator<<(std::ostream &os, axon::resultset &rc) { return rc.print(os); }
	};

}

#endif