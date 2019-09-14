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

constexpr int size = 100;
static void BM_VirtualVector(benchmark::State& state) {
	std::vector<std::unique_ptr<Base>> objects;
	for (int i = 0; i < size; ++i) {
		objects.push_back(MakeBaseRand());
	}
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		for (auto& o : objects) {
			benchmark::DoNotOptimize(o->draw());
		}
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

static void BM_PolyObjectVector(benchmark::State& state) {
	std::vector<polymorphic::object<int(draw)>> objects;
	for (int i = 0; i < size; ++i) {
		objects.push_back(GetObjectRand());
	}
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		for (auto& o : objects) {
			benchmark::DoNotOptimize(o.call<draw>());
		}
	}
}

static void BM_PolyRefVector(benchmark::State& state) {
	std::vector<polymorphic::object<int(draw)>> objects;
	for (int i = 0; i < size; ++i) {
		objects.push_back(GetObjectRand());
	}

	std::vector<polymorphic::ref<int(draw)>> refs(objects.begin(), objects.end());
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		for (auto& r : refs) {
			benchmark::DoNotOptimize(r.call<draw>());
		}
	}
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

static void BM_FunctionVector(benchmark::State& state) {
	std::vector<std::function<int()>> objects;
	for (int i = 0; i < size; ++i) {
		objects.push_back(GetFunctionRand());
	}
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		for (auto& o : objects) {
			benchmark::DoNotOptimize(o());
		}
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

static void BM_NonVirtualVector(benchmark::State& state) {
	std::vector<NonVirtual> objects;
	for (int i = 0; i < size; ++i) {
		objects.push_back(NonVirtual{});
	}
	// Perform setup here
	for (auto _ : state) {
		// This code gets timed
		for (auto& o : objects) {
			benchmark::DoNotOptimize(o.draw());
		}
	}
}
// Register the function as a benchmark
BENCHMARK(BM_NonVirtual);
BENCHMARK(BM_Virtual);
BENCHMARK(BM_Function);
BENCHMARK(BM_PolyObject);
BENCHMARK(BM_PolyRef);

BENCHMARK(BM_NonVirtualVector);
BENCHMARK(BM_VirtualVector);
BENCHMARK(BM_FunctionVector);
BENCHMARK(BM_PolyObjectVector);
BENCHMARK(BM_PolyRefVector);



// Run the benchmark
BENCHMARK_MAIN();