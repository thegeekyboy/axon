#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include <charconv>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include <axon/util.h>

/* ── Parse an 18-digit numeric string into a uint64_t key.
   std::from_chars is used — no locale, no heap, branch-predictor friendly. */
inline uint64_t parse_identity(std::string_view id)
{
	if (id.size() != 18) throw std::invalid_argument("identity id must be exactly 18 digits");

	return axon::util::str_to_num<uint64_t>(id);
}

/* entity cost
	uint64_t identity_id  =  8 bytes
	uint64_t × 5 (sums)   =  40 bytes
	uint16_t × 5 (cnts)   =  10 bytes
	[padding at end]      =   6 bytes
	──────────────────────────────────
	Total                 =  64 bytes
*/
struct Stats {
	uint64_t p2p_sum  {};
	uint64_t bill_sum {};
	uint64_t co_sum   {};
	uint64_t ci_sum   {};
	uint64_t pay_sum  {};

	uint16_t p2p      {};
	uint16_t bill     {};
	uint16_t co       {};
	uint16_t ci       {};
	uint16_t pay      {};
};

struct entity {
	uint64_t _id {};

	struct Stats stats;

	explicit entity(uint64_t id): _id(id) { };
};

class cache
{
	std::vector<entity>	_entities;
	std::unordered_map<uint64_t, std::size_t> _index;
	
	public:
		explicit cache(std::size_t expected_capacity = 0) {
			if (expected_capacity > 0) {
				_entities.reserve(expected_capacity);
				_index.reserve(expected_capacity);
			}
		}

		/* Push by pre-parsed uint64_t key */
		entity* push(uint64_t _id) {
			if (_index.count(_id))
				throw std::runtime_error("duplicate _id");

			_entities.emplace_back(_id);
			_index.emplace(_id, _entities.size() - 1);
			return &_entities.back();
		}

		/* Push by 18-digit string — parsed once at insert time */
		entity* push(std::string_view _id) {
			return push(parse_identity(_id));
		}

		/* O(1) find by uint64_t — fastest path */
		entity* find(uint64_t _id) noexcept {
			auto it = _index.find(_id);
			return it == _index.end() ? nullptr : &_entities[it->second];
		}

		/* O(1) find by 18-digit string */
		entity* find(std::string_view _id) {
			return find(parse_identity(_id));
		}

		const entity* find(uint64_t _id) const noexcept {
			auto it = _index.find(_id);
			return it == _index.end() ? nullptr : &_entities[it->second];
		}

		const entity* find(std::string_view _id) const {
			return find(parse_identity(_id));
		}

		std::size_t size() const noexcept { return _entities.size(); }
		bool empty() const noexcept { return _entities.empty(); }

		auto begin() noexcept { return _entities.begin(); }
		auto end() noexcept { return _entities.end(); }
		auto begin() const noexcept { return _entities.begin(); }
		auto end() const noexcept { return _entities.end(); }

		void print() {

			for (const entity& en : _entities) std::cout<<en._id<<"=> co: "<<en.stats.co<<":"<<en.stats.co_sum<<"=> ci: "<<en.stats.ci<<":"<<en.stats.ci_sum<<"=> p2p: "<<en.stats.p2p<<":"<<en.stats.p2p_sum<<"=> pay: "<<en.stats.pay<<":"<<en.stats.pay_sum<<std::endl;
		}

		std::size_t memory() {
			static constexpr std::size_t NODE_OVERHEAD = sizeof(uint64_t) + sizeof(std::size_t) + sizeof(void*) + sizeof(void*);
													   //^^ key             ^^ mapped value (array index) ^^ next-pointer (chained node)

			std::size_t capacity = 
				_entities.capacity() * sizeof(entity)
				+ (_index.bucket_count() * sizeof(void*))   // bucket array
				+ (_index.size() * NODE_OVERHEAD);

			return capacity / (1024*1024);
		}
};

