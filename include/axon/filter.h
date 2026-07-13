#ifndef AXON_FILTER_H_
#define AXON_FILTER_H_

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <axon/connection.h>

namespace axon
{
	namespace transfer {

		// axon::transfer::filter
		//
		// A pass-through connector meant to sit *between* two real endpoints in a
		// pipeline, e.g. `sftp >> bz2 >> file`. It is not itself a storage backend:
		// connect()/chwd()/list()/get()/put()/etc. are not meaningful for it and
		// simply throw if called. Concrete filters (bz2, ...) drive their transform
		// out of write(), forwarding the transformed bytes to `_downstream`.
		//
		// NOTE ON CHAINING: axon::transfer::operator>>(src, dest) evaluates as
		// `src.push(dest); return src;` -- it returns the *source*, not the
		// destination. That means a literal `sftp >> bz2 >> file` left-to-right
		// chain will NOT wire file in as bz2's downstream automatically; the
		// second `>>` would re-read sftp and push straight into file, bypassing
		// bz2 entirely. Until operator>> is made filter-aware, attach the chain
		// explicitly before pushing, e.g.:
		//
		//     axon::transfer::bz2 bz2c;
		//     bz2c.attach(file);
		//     bz2c.open("", std::ios::out);
		//     sftp >> bz2c;
		//     bz2c.close();
		//
		class filter : public connection {

			protected:
				connection *_downstream { nullptr };
				bool _open { false };

				[[noreturn]] void _unsupported(const char *fn) const {
					throw axon::exception(__FILENAME__, __LINE__, fn, "[" + _id + "] operation not supported on a filter connector");
				}

			public:
				filter(): connection("localhost", "filter", "", 0) { }
				filter(const filter &rhs): connection(rhs), _downstream(rhs._downstream) { }
				virtual ~filter() { }

				// wire the next stage of the pipeline; must be called before open()
				void attach(connection &next) { _downstream = &next; }
				connection *downstream() const { return _downstream; }

				bool connect() override { return true; }
				bool disconnect() override { return true; }

				bool chwd(std::string) override { _unsupported(__PRETTY_FUNCTION__); }
				std::string pwd() override { _unsupported(__PRETTY_FUNCTION__); }
				bool mkdir(std::string) override { _unsupported(__PRETTY_FUNCTION__); }
				size_t list(const cb &) override { _unsupported(__PRETTY_FUNCTION__); }
				size_t list(std::vector<axon::entry> &) override { _unsupported(__PRETTY_FUNCTION__); }
				off_t copy(std::string, std::string, bool) override { _unsupported(__PRETTY_FUNCTION__); }
				off_t copy(std::string, std::string) override { _unsupported(__PRETTY_FUNCTION__); }
				bool ren(std::string, std::string) override { _unsupported(__PRETTY_FUNCTION__); }
				bool del(std::string) override { _unsupported(__PRETTY_FUNCTION__); }

				off_t get(std::string, std::string, bool) override { _unsupported(__PRETTY_FUNCTION__); }
				off_t put(std::string, std::string, bool) override { _unsupported(__PRETTY_FUNCTION__); }

				// open()/close(), read()/write() are left to concrete filters (e.g. bz2)
		};

		// axon::transfer::bz2
		//
		// Streaming bzip2 filter. In COMPRESS mode (the default), bytes written
		// into this connector via write() are bzip2-compressed and forwarded to
		// the attached downstream connection. In DECOMPRESS mode, bytes written
		// in are treated as a bzip2 stream and the decompressed bytes are
		// forwarded downstream instead.
		//
		// This connector is a sink, not a source: read() is not supported since
		// nothing upstream ever pulls from it -- data always arrives via write()
		// from whatever is on the left-hand side of the pipeline.
		class bz2 : public filter {

			public:
				enum class mode { COMPRESS, DECOMPRESS };

			private:
				mode _mode { mode::COMPRESS };
				boost::iostreams::filtering_ostream _stream;

				// Minimal Boost.Iostreams Sink adapter that forwards compressed/
				// decompressed bytes to the downstream axon connector's write().
				struct sink {

					typedef char char_type;
					typedef boost::iostreams::sink_tag category;

					connection *conn;

					std::streamsize write(const char *s, std::streamsize n) {

						ssize_t rc = conn->write(s, static_cast<size_t>(n));

						if (rc < 0)
							throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "downstream write() failed");

						return static_cast<std::streamsize>(rc);
					}
				};

			public:
				bz2(mode m = mode::COMPRESS): filter(), _mode(m) { }
				bz2(const bz2 &rhs): filter(rhs), _mode(rhs._mode) { }
				~bz2() { if (_open) close(); }

				void mode(enum mode m) { _mode = m; }
				enum mode mode() const { return _mode; }

				bool open(std::string, std::ios_base::openmode) override {

					if (_open)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] filter is already open");

					if (_downstream == nullptr)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] no downstream connector attached, call attach() first");

					_stream.reset();

					if (_mode == mode::COMPRESS)
						_stream.push(boost::iostreams::bzip2_compressor());
					else
						_stream.push(boost::iostreams::bzip2_decompressor());

					_stream.push(sink{ _downstream });

					_open = _fileopen = true;

					return _open;
				}

				bool close() override {

					if (!_open)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] filter is not open");

					// popping/reset()-ing the filtering_ostream flushes the
					// bzip2 codec's internal buffers (and, for compression,
					// writes the final block/trailer) through to the sink.
					_stream.reset();

					_open = _fileopen = false;

					return !_open;
				}

				ssize_t read(char *, size_t) override {
					_unsupported(__PRETTY_FUNCTION__);
				}

				ssize_t write(const char *buffer, size_t size) override {

					if (!_open)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] filter is not open");

					_stream.write(buffer, size);

					if (!_stream)
						throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "[" + _id + "] bzip2 stream error");

					return static_cast<ssize_t>(size);
				}
		};
	}
}

#endif