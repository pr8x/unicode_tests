#include <benchmark/benchmark.h>

#include "lib.h"
#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#ifdef _DEBUG
#pragma comment ( lib, "benchmarkd.lib" )
#pragma comment ( lib, "benchmark_maind.lib" )
#else
#pragma comment ( lib, "benchmark.lib" )
#pragma comment ( lib, "benchmark_main.lib" )
#endif
#endif


static void BM_MakeCharacter(benchmark::State& state) {

	for (auto _ : state) {

		auto a = utf8character::make<u8"ä">();

		benchmark::DoNotOptimize(a);

	}
}

BENCHMARK(BM_MakeCharacter);

static void BM_StringConstruction(benchmark::State& state) {

	for (auto _ : state) {

		auto a = utf8string(u8"123456789123456789123456789");

		benchmark::DoNotOptimize(a);

	}
}


BENCHMARK(BM_StringConstruction);


static void BM_StdStringConstruction(benchmark::State& state) {

	for (auto _ : state) {

		auto a = std::u8string(u8"123456789123456789123456789");

		benchmark::DoNotOptimize(a);

	}
}


BENCHMARK(BM_StdStringConstruction);


static void BM_StringLength(benchmark::State& state) {

	auto a = utf8string(u8"123456789123456789123456789");

	benchmark::DoNotOptimize(a);

	for (auto _ : state) {
		auto len = a.length();

		benchmark::DoNotOptimize(len);
	}
}


BENCHMARK(BM_StringLength);


static void BM_Decode(benchmark::State& state) {

	auto& a = u8"äüöüdwefdefwef❤";

	for (auto _ : state) {

		utf8_encoding::decode_iterator beg(std::begin(a));
		utf8_encoding::decode_iterator end(std::end(a));

		for (; beg != end; ++beg) {
			benchmark::DoNotOptimize(*beg);
		}

	}
}

BENCHMARK(BM_Decode);


static void BM_DecodeLong(benchmark::State& state) {

	auto& a = u8"äüöüdwefdefwef❤😂😂😂😂😂😂😂😂😂😂😂😂343434343434343434343434434343434343434";

	for (auto _ : state) {

		utf8_encoding::decode_iterator beg(std::begin(a));
		utf8_encoding::decode_iterator end(std::end(a));

		for (; beg != end; ++beg) {
			benchmark::DoNotOptimize(*beg);
		}

	}
}

BENCHMARK(BM_DecodeLong);


static void BM_GraphemeSplitting(benchmark::State& state) {

	auto a = codepoint(2344);
	auto b = codepoint(2368);

	for (auto _ : state) {
		benchmark::DoNotOptimize(segmentation::is_grapheme(a, b));
	}
}

BENCHMARK(BM_GraphemeSplitting);


static void BM_IndexString(benchmark::State& state) {

	auto str = utf8string(u8"ABC💋💋💋");

	for (auto _ : state) {
		benchmark::DoNotOptimize(str[3]);
	}
}

BENCHMARK(BM_IndexString);


BENCHMARK_MAIN();
