#include "polymorphic.hpp"
#include <benchmark/benchmark.h>

#ifdef _MSC_VER
#pragma comment(lib,"shlwapi.lib")
#endif

#include "benchmark_imp.h"


void SomeFunction() {}

static void BM_Virtual(benchmark::State& state) {
	auto b = MakeBase();
	auto pb = b.get();
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		benchmark::DoNotOptimize(pb->draw());
	}
}

static void BM_PolyRef(benchmark::State& state) {
	Dummy d;
	polymorphic::ref<int(draw)> ref(d);
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		benchmark::DoNotOptimize(ref.call<draw>());
	}
	static_assert(sizeof(ref) == 2 * sizeof(void*));
}

static void BM_PolyObject(benchmark::State& state) {
	Dummy d;
	polymorphic::object<int(draw)> ref(d);
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		benchmark::DoNotOptimize(ref.call<draw>());
	}
	static_assert(sizeof(ref) == 3 * sizeof(void*));
}

static void BM_Function(benchmark::State& state) {
	auto f = GetFunction();
	Dummy d;
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		benchmark::DoNotOptimize(f(d));
	}
}

static void BM_NonVirtual(benchmark::State& state) {
	NonVirtual n;
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		benchmark::DoNotOptimize(n.draw());
	}
}
// Register the function as a benchmark
BENCHMARK(BM_NonVirtual);
BENCHMARK(BM_Virtual);
BENCHMARK(BM_Function);
BENCHMARK(BM_PolyObject);
BENCHMARK(BM_PolyRef);

// Run the benchmark
BENCHMARK_MAIN();