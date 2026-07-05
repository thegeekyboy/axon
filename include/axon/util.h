#ifndef AXON_UTIL_H_
#define AXON_UTIL_H_

#include <vector>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <thread>
#include <algorithm>

#include <unistd.h>

#include <cstdint>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <type_traits>

#include <boost/regex.hpp>
#include <magic.h>

#include <axon.h>

namespace axon
{
	namespace util
	{
		typedef unsigned char BYTE;

		unsigned long long bytes_to_ull(const char*, size_t);
		std::string bytes_to_binarystring(const char *, const size_t);

		char *trim(char *);
		static inline void ltrim(std::string &s)
		{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
		}

		static inline void rtrim(std::string &s)
		{
			s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch);	}).base(), s.end());
		}

		static inline void trim(std::string &s)
		{
			rtrim(s);
			ltrim(s);
		}

		std::vector<std::string> split(const std::string&, const char);
		std::string merge(std::vector<std::string>, const char);

		std::tuple<std::string, std::string> splitpath(std::string);
		std::tuple<std::string, std::string> splitbucket(std::string);

		std::string hash(const std::string&);
		std::string md5(std::string&);

		// int mkdir(const std::string&, mode_t);
		bool makedir(const char *);
		bool isdir(const std::string&);
		bool isfile(const std::string&);
		bool exists(const std::string&);
		bool exists(const std::string&, const std::string&);
		bool iswritable(const std::string&);

		bool execmd(const char *cmd);

		std::string base64_encode(BYTE const* buf, unsigned int bufLen);
		std::string base64_encode(const std::string &);
		std::vector<BYTE> base64_decode(std::string const&);

		std::string uuid();
		double random(double, double);

		bool set_thread_name(pthread_t, std::string);
		void rm_thread(std::vector<std::thread>&, std::thread::id);

		std::string demangle(const char*);

		template <typename T>
		uint16_t count(std::va_list list, const T first)
		{
			uint16_t cnt = 0;
			T bv = first;

			while (bv != nullptr)
			{
				cnt++;
				bv = va_arg(list, T);
			}

			return cnt;
		}

		template <typename T>
		inline T str_to_num(std::string_view sv)
		{
			static_assert(std::is_integral_v<T>, "T must be an integral type");

			T result{};
			auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

			if (ec == std::errc::invalid_argument)
				throw std::invalid_argument(std::string("non-numeric input '") + std::string(sv) + "'");

			if (ec == std::errc::result_out_of_range)
				throw std::overflow_error(std::string("value '") + std::string(sv) + "' out of range for type (max=" + std::to_string(std::numeric_limits<T>::max()) + ")");

			if (ptr != sv.data() + sv.size())
				throw std::invalid_argument(std::string("trailing non-numeric characters in '") + std::string(sv) + "'");

			return result;
		}

		struct magic {

			static struct magic_set *cookie;

			magic() {

				if (cookie) return;

				if ((cookie = magic_open(MAGIC_MIME|MAGIC_CHECK)) == NULL)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot openmagic database");

				if (magic_load(cookie, NULL) != 0)
					throw axon::exception(__FILENAME__, __LINE__, __PRETTY_FUNCTION__, "cannot load magic database");
			}

			~magic() {
				if (cookie) magic_close(cookie);
			}

			static std::tuple<std::string, std::string> resolve(std::string_view filename)
			{
				std::string mimetype = {}, encoding = {};

				mimetype = magic_file(cookie, filename.data());

				std::vector<std::string> bits = axon::util::split(mimetype, ';');

				if (bits.size()>1)
				{
					trim(bits[0]);
					trim(bits[1]);
				}

				mimetype = bits[0];
				encoding = bits[1];

				return { mimetype, encoding };
			}
		};

		struct validator {

			static const boost::regex regex_ipaddr;
			static const boost::regex regex_fqdn;
			static const boost::regex regex_tns;
			static const boost::regex regex_username;

			static bool hostname(const std::string &hn) {

				if (!boost::regex_match(hn, regex_ipaddr) && !boost::regex_match(hn, regex_fqdn))
					return false;

				return true;
			};

			static bool username(const std::string &un) {

				if (un.size() <= 0 || !boost::regex_match(un, regex_username))
					return false;

				return true;
			};

			static bool tns(const std::string &un) {

				if (un.size() <= 0 || !boost::regex_match(un, regex_tns))
					return false;

				return true;
			};
		};

		struct procinfo {

			char tcomm[16], state;
			int pid, ppid, pgid, sid, tty_nr, tty_pgrp, exit_signal, cpu;
			unsigned int flags, rt_priority, policy;
			long cutime, cstime, priority, nicev, num_threads, it_real_value, rss, tickspersec;
			unsigned long min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stimev, vsize, rsslim, start_code, end_code, start_stack, esp, eip, pending, blocked, sigign, sigcatch, wchan, dummy;
			unsigned long long start_time;

			procinfo() = delete;
			procinfo(const procinfo&) = delete;
			procinfo& operator=(const procinfo&) = delete;

			explicit procinfo(int xpid) {
				char filename[64];
				snprintf(filename, sizeof(filename), "/proc/%d/stat", xpid);

				std::unique_ptr<FILE, decltype(&fclose)> f(fopen(filename, "r"), &fclose);
				if (!f) throw std::runtime_error("could not open " + std::string(filename));

				FILE* fp = f.get();
				fscanf(fp, "%d %15s %c %d %d %d %d %d %u "
						"%lu %lu %lu %lu %lu %lu %ld %ld "
						"%ld %ld %ld %ld %llu "
						"%lu %ld %lu %lu %lu %lu %lu %lu "
						"%lu %lu %lu %lu %lu "
						"%lu %lu "        // skip nswap, cnswap
						"%d %d %u %u",
					&pid, tcomm, &state, &ppid, &pgid, &sid, &tty_nr, &tty_pgrp, &flags,
					&min_flt, &cmin_flt, &maj_flt, &cmaj_flt, &utime, &stimev, &cutime, &cstime,
					&priority, &nicev, &num_threads, &it_real_value, &start_time,
					&vsize, &rss, &rsslim, &start_code, &end_code, &start_stack, &esp, &eip,
					&pending, &blocked, &sigign, &sigcatch, &wchan,
					&dummy, &dummy,
					&exit_signal, &cpu, &rt_priority, &policy);

				tickspersec = sysconf(_SC_CLK_TCK);
			}
		};
	}
}

#endif

