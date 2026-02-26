#include <benchmark/benchmark.h>

#include "benchmark/benchmark.h"
#include "sketch/distinct.h"

void BenchTinyOneEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 3 * sizeof(uint64_t)});

  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
  }

  benchmark::DoNotOptimize(sketch.Estimate());
}

void BenchTinyMultiEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 3 * sizeof(uint64_t)});

  double estimation = 0;
  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
    estimation += sketch.Estimate();
  }

  benchmark::DoNotOptimize(estimation);
}

void BenchMediumOneEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 100 * sizeof(uint64_t)});

  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
  }

  benchmark::DoNotOptimize(sketch.Estimate());
}

void BenchMediumMultiEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 100 * sizeof(uint64_t)});

  double estimation = 0;
  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
    estimation += sketch.Estimate();
  }

  benchmark::DoNotOptimize(estimation);
}

void BenchLargeOneEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 10000 * sizeof(uint64_t)});

  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
  }

  benchmark::DoNotOptimize(sketch.Estimate());
}

void BenchLargeMultiEstimation(benchmark::State& state) {
  sketch::DistinctSketch sketch(sketch::DistinctSketch::Parameters{.memory_limit_bytes = 10000 * sizeof(uint64_t)});

  double estimation = 0;
  uint64_t i = 0;
  while (state.KeepRunning()) {
    ++i;
    sketch.Append(i);
    estimation += sketch.Estimate();
  }

  benchmark::DoNotOptimize(estimation);
}

BENCHMARK(BenchTinyOneEstimation);
BENCHMARK(BenchTinyMultiEstimation);
BENCHMARK(BenchMediumOneEstimation);
BENCHMARK(BenchMediumMultiEstimation);
BENCHMARK(BenchLargeOneEstimation);
BENCHMARK(BenchLargeMultiEstimation);

BENCHMARK_MAIN();
