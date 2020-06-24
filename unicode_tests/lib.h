#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <array>
#include <iostream>
#include <bit>

#include <immintrin.h>
#include <numeric>

using std::vector;
using std::span;
using std::pair;

struct codepoint {
	constexpr codepoint() = default;

	constexpr codepoint(uint32_t value)
		: _value(value) {
	}

	[[nodiscard]] constexpr uint32_t value() const {
		return _value;
	}

	[[nodiscard]] bool is_valid() const {
		return _value != 0;
	}

	bool operator==(const codepoint& other) const {
		return _value == other._value;
	}

private:
	uint32_t _value;
};

inline std::ostream& operator<<(std::ostream& out, const codepoint& cp) {
	out << cp.value();
	return out;
}

template <class T>
concept encoding = requires {
	typename T::code_unit_type;
};

struct utf8_encoding {
	using code_unit_type = char8_t;

	template <class Iterator>
	class decode_iterator {
		Iterator _inner;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = codepoint;
		using difference_type = size_t;
		using pointer = codepoint*;
		using reference = codepoint&;

		template <class _Iterator>
		constexpr decode_iterator(_Iterator inner) :
			_inner(inner) {
		}

		[[nodiscard]] constexpr Iterator inner() const {
			return _inner;
		}

		constexpr codepoint operator*() const {
			auto it = _inner;
			uint32_t cp = *it;

			switch (sequence_length(*_inner)) {
			case 1: break;
			case 2:
				++it;
				cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
				break;
			case 3:
				++it;
				cp = ((cp << 12) & 0xffff) + (((*it) << 6) & 0xfff);
				++it;
				cp += (*it) & 0x3f;
				break;
			case 4:
				++it;
				cp = ((cp << 18) & 0x1fffff) + (((*it) << 12) & 0x3ffff);
				++it;
				cp += ((*it) << 6) & 0xfff;
				++it;
				cp += (*it) & 0x3f;
				break;
			}

			return cp;
		}

		constexpr decode_iterator& operator++() {
			_inner += sequence_length(*_inner);
			return *this;
		}

		constexpr bool operator==(const decode_iterator& rhs) const {
			return _inner == rhs._inner;
		}

		constexpr bool operator!=(const decode_iterator& rhs) const {
			return _inner != rhs._inner;
		}
	};

	template <class Iterator>
	decode_iterator(Iterator&&) -> decode_iterator<Iterator>;

private:

	static constexpr code_unit_type lengths[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
	};

	static constexpr int sequence_length(code_unit_type lead) {
		return lengths[lead >> 3];
	}
};

namespace util
{
	template <class C>
	constexpr size_t strlen(const C* str) {
		size_t count = 0;
		while (*(str++) != 0) {
			count++;
		}
		return count;
	}

	template <class Iterator>
	constexpr Iterator safe_next(Iterator it, Iterator end, std::iter_difference_t<Iterator> offset) {
#if _HAS_ITERATOR_DEBUGGING
		return std::distance(it, end) >= offset
			? std::next(it, offset) : end;

#else
		return std::next(it, offset);
#endif

	}
}

template <class C, size_t N>
struct static_string {
	constexpr static_string(const C (&foo)[N + 1]) {
		std::copy_n(foo, N, string);
	}

	using char_type = C;

	bool operator==(const static_string& rhs) const = default;

	C string[N];
};

template <class C, std::size_t N>
static_string(const C (&str)[N]) -> static_string<C, N - 1>;

namespace segmentation
{
	namespace detail
	{
		template <size_t Index>
		static constexpr uint32_t bit = 0x1 << Index;

		enum class grapheme_property : uint32_t {
			OTHER = bit<0>,
			PREPEND = bit<1>,
			CR = bit<2>,
			LF = bit<3>,
			CONTROL = bit<4>,
			EXTEND = bit<5>,
			REGIONAL_INDICATOR = bit<6>,
			SPACINGMARK = bit<7>,
			L = bit<8>,
			V = bit<9>,
			T = bit<10>,
			LV = bit<11>,
			LVT = bit<12>,
			E_BASE = bit<13>,
			E_MODIFIER = bit<14>,
			ZWJ = bit<15>,
			GLUE_AFTER_ZWJ = bit<16>,
			E_BASE_GAZ = bit<17>,

			ANY = 0xFFFFF,
			NONE = 0x0
		};

		constexpr grapheme_property operator|(grapheme_property a, grapheme_property b) {
			return static_cast<grapheme_property>(
				static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
		}

		constexpr grapheme_property operator&(grapheme_property a, grapheme_property b) {
			return static_cast<grapheme_property>(
				static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
		}
		using enum grapheme_property;

		struct grapheme_property_range {
			codepoint min;
			codepoint max;
			grapheme_property property;
		};

		//template <class It, class K, class C>
		//constexpr It branchfree_lower_bound(It begin, It end, const K& key, const C& comp) {
		//	auto n = std::distance(begin, end);
		//	while (n > 1) {
		//		auto half = n / 2;
		//		auto bh = begin + half;
		//		begin = comp(*bh, key) ? bh : begin;
		//		n -= half;
		//	}
		//	
		//	return begin + static_cast<int>(comp(*begin, key));
		//}

		template <class Input, class Output>
		constexpr int eytzinger(Input beg, Input end, Output out, int i = 0, int k = 1) {
			if (k <= std::distance(beg, end)) {
				i = eytzinger(beg, end, out, i, 2 * k);
				*(out + k) = *(beg + i++);
				i = eytzinger(beg, end, out, i, 2 * k + 1);
			}
			return i;
		}

		template <class It, class K, class C>
		constexpr It eytzinger_lower_bound(It begin, It end, const K& key, const C& comp) {
			auto k = 1u;
			while (k <= std::distance(begin, end)) {
				//__builtin_prefetch(b + k * block_size);
				auto r = comp(*(begin + k), key);
				k = 2 * k + static_cast<unsigned int>(r);
			}
			//k >>= __builtin_ffs(~k);
			k >>= std::countr_zero(~k) + 1;
			return begin + k;
		}

		constexpr auto make_range_data() {
			auto input = std::to_array<grapheme_property_range>({
#include "grapheme_break_data.txt"
			});

			std::array<grapheme_property_range, input.size() + 1> output{};
			eytzinger(input.begin(), input.end(), output.begin());

			return output;
		}

		static constexpr auto grapheme_property_ranges = make_range_data();

		struct grapheme_property_range_comparator {
			using is_transparent = void;

			constexpr bool operator()(const grapheme_property_range& range, const codepoint& cp) const {
				return range.max.value() < cp.value();
			}
		};

		constexpr grapheme_property lookup_grapheme_property(codepoint cp) {
			auto beg = std::begin(grapheme_property_ranges);
			auto end = std::end(grapheme_property_ranges);

			auto it = eytzinger_lower_bound(
				beg,
				end,
				cp,
				grapheme_property_range_comparator());

			assert(it != end);

			if (cp.value() < it->min.value() || cp.value() > it->max.value()) {
				return OTHER;
			}

			return it->property;
		}

		struct break_rule {
			grapheme_property left;
			grapheme_property right;

			[[nodiscard]] constexpr bool match(grapheme_property a, grapheme_property b) const {
				return (left & a) == a && (right & b) == b;
			}
		};

		//https://unicode.org/reports/tr29/
		static constexpr break_rule break_rules[] = {
			// GB3
			{CR, LF},
			// GB6
			{L, L | V | LV | LVT},
			// GB7
			{LV | V, V | T},
			// GB8
			{LVT | T, T},
			// GB9
			{ANY, EXTEND | ZWJ},
			// GB9a
			{ANY, SPACINGMARK},
			// GB9b
			{PREPEND, ANY},
			// GB10
			{E_BASE | E_BASE_GAZ, E_MODIFIER},
			{EXTEND, E_MODIFIER},
			// GB11
			{ZWJ, E_BASE_GAZ | GLUE_AFTER_ZWJ},
			// GB12, GB13
			{REGIONAL_INDICATOR, REGIONAL_INDICATOR},

			//PADDING 
			{NONE, NONE},
		};

		inline bool match_break_rules_simd(grapheme_property ap, grapheme_property bp) {

			auto beg = std::begin(break_rules);
			auto end = std::end(break_rules);

			auto lmask = _mm_set1_epi32(static_cast<int>(ap));
			auto rmask = _mm_set1_epi32(static_cast<int>(bp));

			for (auto it = beg; it != end;) {

				auto r0 = *it++;
				auto r1 = *it++;
				auto r2 = *it++;
				auto r3 = *it++;

				auto left = _mm_set_epi32(
					static_cast<int>(r3.left),
					static_cast<int>(r2.left),
					static_cast<int>(r1.left),
					static_cast<int>(r0.left)
				);

				auto right = _mm_set_epi32(
					static_cast<int>(r3.right),
					static_cast<int>(r2.right),
					static_cast<int>(r1.right),
					static_cast<int>(r0.right)
				);

				auto lresult = _mm_and_si128(left, lmask);
				auto rresult = _mm_and_si128(right, rmask);
				auto result = _mm_mullo_epi32(lresult, rresult);

				if (_mm_testz_si128(result, result) == 0) {
					return true;
				}
			}

			// GB4, GB5, GB999
			return false;
		}

		constexpr bool match_break_rules(grapheme_property ap, grapheme_property bp) {

			for (auto rule : break_rules) {
				if (rule.match(ap, bp)) {
					return true;
				}
			}

			// GB4, GB5, GB999
			return false;
		}

		constexpr bool is_grapheme_break(grapheme_property ap, grapheme_property bp) {
			if (std::is_constant_evaluated()) {
				return match_break_rules(ap, bp);
			}

			return match_break_rules_simd(ap, bp);
		}
	}

	constexpr bool is_grapheme(codepoint a, codepoint b) {
		using namespace detail;
		auto ap = lookup_grapheme_property(a);
		auto bp = lookup_grapheme_property(b);
		return is_grapheme_break(ap, bp);
	}

	template <class Iterator>
	constexpr Iterator next_grapheme_cluster(Iterator beg, Iterator end) {

		using namespace detail;

		grapheme_property ap = lookup_grapheme_property(*beg);
		grapheme_property bp;

		while (++beg != end) {
			bp = lookup_grapheme_property(*beg);

			if (!is_grapheme_break(ap, bp)) [[likely]]
			{
				break;
			}

			ap = bp;
		}

		return beg;
	}
}

template <encoding Encoding>
class basic_character {
public:
	using encoding_type = Encoding;
	using code_unit_type = typename encoding_type::code_unit_type;
	using iterator = typename encoding_type::template decode_iterator<
		typename span<const code_unit_type>::iterator>;

	template <static_string Expression>
	static constexpr basic_character<Encoding> make() {
		static_assert(std::is_same_v<decltype(Expression)::char_type, code_unit_type>,
			"The given character expression does not have the correct char type. Make sure to use proper literal operator (u8, u16, u32)."
		);
		static_assert(validate_sequence(Expression.string),
			"The given character expression is invalid. Only single code points or graphemes are allowed.");

		return basic_character<Encoding>{Expression.string};
	}

	[[nodiscard]] bool operator==(const basic_character<Encoding>& other) const {
		return std::memcmp(_units.data(), other._units.data(), other._units.size_bytes()) == 0;
	}

	[[nodiscard]] iterator begin() const {
		return iterator(_units.begin());
	}

	[[nodiscard]] iterator end() const {
		return iterator(_units.end());
	}

private:
	span<const code_unit_type> _units;

	friend std::ostream& operator<<(std::ostream& out, const basic_character<Encoding>& character) {
		out.write(reinterpret_cast<const char*>(character._units.data()), character._units.size());
		return out;
	}

	explicit basic_character(span<const code_unit_type> units)
		: _units(units) {
	}

	static constexpr bool validate_sequence(span<const code_unit_type> units) {
		if (units.empty()) {
			return false;
		}

		auto beg = iterator(units.begin());
		auto end = iterator(units.end());
		auto cle = segmentation::next_grapheme_cluster(beg, end);

		return cle == end;
	}

	template <encoding, class>
	friend class basic_string;

	template <encoding, class>
	friend class character_iterator;
};

template <class Unit>
class basic_character_map {
	vector<Unit> _units;

	static_assert(std::is_unsigned_v<Unit>);
	static constexpr auto unit_size = sizeof(Unit) * 8;

	static constexpr size_t next_bit(Unit unit, size_t current) {
		auto p2 = Unit(1) << (current + 1);
		auto mask = unit & ~(p2 - 1);
		return std::countr_zero(mask);
	}

public:
	template <class Iterator>
	explicit basic_character_map(Iterator beg, Iterator end) {

		auto length = std::distance(beg.inner(), end.inner());
		_units.resize(length / unit_size + 1);

		for (auto it = beg; it != end; it = segmentation::next_grapheme_cluster(it, end)) {
			auto pos = std::distance(beg.inner(), it.inner());
			auto v = pos % unit_size;
			_units[pos / unit_size] |= Unit(0x1) << v;
		}
	}

	[[nodiscard]] constexpr size_t count() const {
		size_t total = 0;
		for (auto unit : _units) {
			total += std::popcount(unit);
		}
		return total;
	}

	template <class Iterator>
	[[nodiscard]] Iterator next_character(Iterator current, Iterator begin, Iterator end) const {

		while (current < end) {

			auto offset = std::distance(begin, current);
			auto unit = _units[offset / unit_size];
			auto bit_offset = offset % unit_size;
			auto next_bit_offset = next_bit(unit, bit_offset);

			current = util::safe_next(begin, end, next_bit_offset);

			//Another bit was found in this unit
			if (next_bit_offset != unit_size) {
				break;
			}
		}

		return current;
	}
};

using character_map64 = basic_character_map<uint64_t>;

template <encoding Encoding, class Iterator>
class character_iterator {
	const character_map64* _map;
	Iterator _begin;
	Iterator _end;
	Iterator _range0;
	Iterator _range1;

public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = basic_character<Encoding>;
	using difference_type = size_t;
	using pointer = basic_character<Encoding>*;
	using reference = basic_character<Encoding>&;

	using encoding_type = Encoding;
	using code_unit_type = typename encoding_type::code_unit_type;

	character_iterator(const character_map64* map, Iterator current, Iterator begin, Iterator end)
		: _map(map),
		  _begin(begin),
		  _end(end),
		  _range0(current),
		  _range1(_map->next_character(_range0, _begin, _end)) {
	}

	basic_character<Encoding> operator*() const {
		return basic_character<Encoding>({
			_range0,
			_range1
		});
	}

	constexpr character_iterator& operator++() {
		_range0 = _range1;
		_range1 = _map->next_character(_range0, _begin, _end);
		return *this;
	}

	constexpr bool operator==(const character_iterator& rhs) const {
		return _range0 == rhs._range0;
	}

	constexpr bool operator!=(const character_iterator& rhs) const {
		return _range0 != rhs._range0;
	}
};

template <encoding Encoding, class Allocator>
class basic_string {
public:
	using encoding_type = Encoding;
	using code_unit_type = typename encoding_type::code_unit_type;
	using iterator = character_iterator<
		Encoding, typename vector<code_unit_type>::const_iterator>;

	basic_string(const code_unit_type* literal) :
		_storage(literal, literal + util::strlen(literal)),
		_map(codepoint_iterator(_storage.begin()), codepoint_iterator(_storage.end())) {
	}

	[[nodiscard]] size_t size() const noexcept {
		return _storage.size();
	}

	[[nodiscard]] size_t length() const noexcept {
		return _map.count();
	}

	[[nodiscard]] basic_character<Encoding> operator[](size_t index) const {
		return *std::next(begin(), index);
	}

	[[nodiscard]] iterator begin() const {
		return iterator(
			&_map,
			_storage.begin(),
			_storage.begin(),
			_storage.end()
		);
	}

	[[nodiscard]] iterator end() const {
		return iterator(
			&_map,
			_storage.end(),
			_storage.begin(),
			_storage.end()
		);
	}

private:
	using codepoint_iterator = typename
	encoding_type::template decode_iterator<
		typename vector<code_unit_type>::const_iterator>;

	vector<code_unit_type, Allocator> _storage;
	character_map64 _map;
};

#define U8_CHAR(e) basic_character<utf8_encoding>::make<u8##e>()

using utf8string = basic_string<utf8_encoding, std::allocator<utf8_encoding::code_unit_type>>;
using utf8character = basic_character<utf8_encoding>;
