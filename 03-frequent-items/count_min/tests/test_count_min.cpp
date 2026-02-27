#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/count_min.h"

namespace sketch {

CountMinSketch CreateSketch(uint64_t buffers_count, uint64_t buffer_size) {
  return CountMinSketch(CountMinSketch::Parameters{
      .buffers_count = buffers_count,
      .buffer_size = buffer_size,
  });
}

TEST(CountMinSketch, Empty) {
  CountMinSketch sketch = CreateSketch(4, 128);
  ASSERT_EQ(sketch.Estimate(42), 0);
}

TEST(CountMinSketch, EstimateAtLeastRealValue) {
  CountMinSketch sketch = CreateSketch(5, 256);

  std::unordered_map<int64_t, int64_t> exact;
  std::mt19937_64 rnd(2101);
  std::uniform_int_distribution<int64_t> key_dist(-1000, 1000);
  std::uniform_int_distribution<int64_t> weight_dist(1, 8);

  for (uint64_t i = 0; i < 10'000; ++i) {
    int64_t key = key_dist(rnd);
    int64_t weight = weight_dist(rnd);
    sketch.Add(key, weight);
    exact[key] += weight;
  }

  for (const auto& [key, expected] : exact) {
    ASSERT_GE(sketch.Estimate(key), expected) << "Underestimation for key " << key;
  }
}

TEST(CountMinSketch, UnitWeights) {
  struct Configuration {
    double eps;
    double delta;
    uint64_t n;
    int64_t key_range;
    uint64_t seed;
  };

  const std::vector<Configuration> configurations = {
      Configuration{.eps = 0.01, .delta = std::exp(-1 + 0.001), .n = 20'000, .key_range = 5'000, .seed = 2101},
      Configuration{.eps = 0.01, .delta = std::exp(-2 + 0.001), .n = 50'000, .key_range = 20'000, .seed = 2102},
      Configuration{.eps = 0.01, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 40'000, .seed = 2103},
      Configuration{.eps = 0.001, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 40'000, .seed = 2103},
  };

  for (const auto& [eps, delta, n, key_range, seed] : configurations) {
    const uint64_t buffer_size = static_cast<uint64_t>(std::ceil(std::exp(1.0) / eps));
    const uint64_t buffers_count = static_cast<uint64_t>(std::ceil(std::log(1.0 / delta)));

    std::cerr << "Test: eps = " << eps << ", delta = " << delta << ", n = " << n << ", key_range = " << key_range
              << ", width = " << buffer_size << ", depth = " << buffers_count << std::endl;

    CountMinSketch sketch = CreateSketch(buffers_count, buffer_size);

    std::unordered_map<int64_t, int64_t> exact;
    std::mt19937_64 rnd(seed);
    std::uniform_int_distribution<int64_t> key_dist(0, key_range);
    for (uint64_t i = 0; i < n; ++i) {
      int64_t key = key_dist(rnd);
      sketch.Add(key, 1);
      exact[key] += 1;
    }

    const double error_threshold = eps * static_cast<double>(n);
    uint64_t bad_keys = 0;
    std::vector<double> all_errors;
    for (const auto& [key, true_count] : exact) {
      const int64_t estimate = sketch.Estimate(key);
      const double error = static_cast<double>(estimate - true_count);
      all_errors.emplace_back(error);
    }

    for (double error : all_errors) {
      if (error > error_threshold) {
        ++bad_keys;
      }
    }

    std::sort(all_errors.begin(), all_errors.end());
    const double max_error = all_errors.back();

    const double actual_eps = all_errors[static_cast<int>(exact.size() - 1 - exact.size() * delta)] / n;

    const double bad_fraction = static_cast<double>(bad_keys) / static_cast<double>(exact.size());
    std::cerr << "bad_fraction = " << bad_fraction << ", max_eps = " << max_error / n << ", actual_eps = " << actual_eps
              << std::endl;

    ASSERT_LE(bad_fraction, delta);
    ASSERT_LE(actual_eps, eps);
    std::cerr << std::string(80, '-') << std::endl;
  }
}

TEST(CountMinSketch, SingleElementDifferentWeights) {
  CountMinSketch sketch = CreateSketch(3, 64);

  sketch.Add(10, 5'000'000'000);
  sketch.Add(10, 7'000'000'000);
  sketch.Add(10, -4'000'000'000);
  sketch.Add(10, 1'000'000'000);

  ASSERT_EQ(sketch.Estimate(10), 9'000'000'000);
}

TEST(CountMinSketch, CustomWeights) {
  struct Configuration {
    double eps;
    double delta;
    uint64_t n;
    int64_t key_range;
    uint64_t seed;
    uint64_t max_w;
  };

  const std::vector<Configuration> configurations = {
      Configuration{
          .eps = 0.01, .delta = std::exp(-1 + 0.001), .n = 20'000, .key_range = 5'000, .seed = 2101, .max_w = 1'000},
      Configuration{
          .eps = 0.01, .delta = std::exp(-2 + 0.001), .n = 50'000, .key_range = 20'000, .seed = 2102, .max_w = 1'000},
      Configuration{
          .eps = 0.01, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 40'000, .seed = 2103, .max_w = 1'000},
      Configuration{
          .eps = 0.001, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 40'000, .seed = 2103, .max_w = 1'000},
  };

  for (const auto& [eps, delta, n, key_range, seed, max_w] : configurations) {
    const uint64_t buffer_size = static_cast<uint64_t>(std::ceil(std::exp(1.0) / eps));
    const uint64_t buffers_count = static_cast<uint64_t>(std::ceil(std::log(1.0 / delta)));

    std::cerr << "Test: eps = " << eps << ", delta = " << delta << ", n = " << n << ", key_range = " << key_range
              << ", width = " << buffer_size << ", depth = " << buffers_count << std::endl;

    CountMinSketch sketch = CreateSketch(buffers_count, buffer_size);

    std::unordered_map<int64_t, int64_t> exact;
    std::mt19937_64 rnd(seed);
    std::uniform_int_distribution<int64_t> key_dist(0, key_range);

    for (uint64_t i = 0; i < n; ++i) {
      int64_t key = key_dist(rnd);
      int64_t w = rnd() % (2 * max_w + 1) - max_w;

      sketch.Add(key, w);
      exact[key] += w;
    }

    int64_t abs_w = 0;
    for (const auto& [k, v] : exact) {
      abs_w += abs(v);
    }

    const double error_threshold = eps * static_cast<double>(abs_w);
    uint64_t bad_keys = 0;
    std::vector<double> all_errors;
    for (const auto& [key, true_count] : exact) {
      const int64_t estimate = sketch.Estimate(key);
      const double error = static_cast<double>(estimate - true_count);
      all_errors.emplace_back(error);
    }

    for (double error : all_errors) {
      if (error > error_threshold) {
        ++bad_keys;
      }
    }

    std::sort(all_errors.begin(), all_errors.end());
    const double max_error = all_errors.back();

    const double actual_eps = all_errors[static_cast<int>(exact.size() - 1 - exact.size() * delta)] / abs_w;

    const double bad_fraction = static_cast<double>(bad_keys) / static_cast<double>(exact.size());
    std::cerr << "bad_fraction = " << bad_fraction << ", max_eps = " << max_error / abs_w
              << ", actual_eps = " << actual_eps << std::endl;

    ASSERT_LE(bad_fraction, delta);
    ASSERT_LE(actual_eps, eps);
    std::cerr << std::string(80, '-') << std::endl;
  }
}

}  // namespace sketch
