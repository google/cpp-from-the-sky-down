#include "polymorphic.hpp"
#include <benchmark/benchmark.h>

#pragma comment(lib,"shlwapi.lib")


#include "benchmark_imp.h"



void SomeFunction() {}

static void BM_Virtual(benchmark::State& state) {
	auto b = MakeBase();
	auto pb = b.get();
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
	  benchmark::DoNotOptimize(pb);
	  benchmark::DoNotOptimize(pb->draw());
  }
}

static void BM_Poly(benchmark::State& state) {
	Dummy d;
	polymorphic::ref<int(draw)> ref(d);
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
	  benchmark::DoNotOptimize(ref);
	  benchmark::DoNotOptimize(ref.call<draw>());
  }
}

static void BM_Function(benchmark::State& state) {
	auto f = GetFunction();
	Dummy d;
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
	  benchmark::DoNotOptimize(f);
	  benchmark::DoNotOptimize(f(d));
  }
}

static void BM_NonVirtual(benchmark::State& state) {
	NonVirtual n;
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
	  benchmark::DoNotOptimize(n);
	  benchmark::DoNotOptimize(n.draw());
  }
}
// Register the function as a benchmark
BENCHMARK(BM_Poly);
BENCHMARK(BM_NonVirtual);
BENCHMARK(BM_Function);
BENCHMARK(BM_Virtual);

// Run the benchmark
BENCHMARK_MAIN();