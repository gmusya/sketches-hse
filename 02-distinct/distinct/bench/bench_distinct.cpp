#include "benchmark/benchmark.h"
#include "sketch/distinct.h"

void BenchSimple(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 1});

  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
  }

  sketch.Estimate();
}

BENCHMARK(BenchSimple);

BENCHMARK_MAIN();
