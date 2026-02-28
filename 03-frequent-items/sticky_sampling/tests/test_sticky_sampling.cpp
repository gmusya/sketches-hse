#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/sticky_sampling.h"

namespace sketch {

StickySamplingSketch CreateSketch(double s, double eps, double delta) {
  return StickySamplingSketch(StickySamplingSketch::Parameters{.s = s, .eps = eps, .delta = delta});
}

std::vector<int64_t> Unpack(const std::unordered_map<int64_t, uint64_t>& counters) {
  std::vector<int64_t> values;
  for (const auto& [k, v] : counters) {
    for (uint64_t i = 0; i < v; ++i) {
      values.emplace_back(k);
    }
  }

  return values;
}

std::vector<int64_t> Shuffle(std::vector<int64_t> values, uint64_t seed) {
  std::mt19937_64 rnd(seed);
  std::shuffle(values.begin(), values.end(), rnd);
  return values;
}

TEST(StickySamplingSketch, Empty) {
  StickySamplingSketch sketch = CreateSketch(0.1, 0.01, 0.01);
  ASSERT_EQ(sketch.Estimate(42), 0);
}

TEST(StickySamplingSketch, Simple) {
  double eps = 0.25;

  StickySamplingSketch sketch = CreateSketch(eps, eps, 0.01);

  std::unordered_map<int64_t, uint64_t> counters = {{1, 123}, {2, 15}, {3, 16}, {4, 17}, {5, 150},
                                                    {6, 10},  {7, 8},  {8, 1},  {9, 10}};
  std::vector<int64_t> values = Shuffle(Unpack(counters), 2101);
  const uint64_t total_values = values.size();
  const uint64_t max_allowed_error = total_values * eps;

  for (int64_t value : values) {
    sketch.Add(value);
  }

  for (const auto& [k, expected] : counters) {
    uint64_t estimation = sketch.Estimate(k);

    uint64_t difference = std::abs(static_cast<int64_t>(estimation) - static_cast<int64_t>(expected));
    EXPECT_LE(difference, max_allowed_error) << "Bad prediction for key " << k;
  }
}

TEST(CountMinSketch, EstimateAtMostRealValue) {
  StickySamplingSketch sketch = CreateSketch(0.01, 0.01, 0.01);

  std::unordered_map<int64_t, int64_t> exact;
  std::mt19937_64 rnd(2101);
  std::uniform_int_distribution<int64_t> key_dist(-1000, 1000);
  std::uniform_int_distribution<int64_t> weight_dist(1, 8);

  for (uint64_t i = 0; i < 10'000; ++i) {
    int64_t key = key_dist(rnd);
    sketch.Add(key);
    exact[key] += 1;
  }

  for (const auto& [key, expected] : exact) {
    ASSERT_LE(sketch.Estimate(key), expected) << "Overestimation for key " << key;
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
      Configuration{.eps = 0.01, .delta = std::exp(-1 + 0.001), .n = 20'000, .key_range = 500, .seed = 2101},
      Configuration{.eps = 0.01, .delta = std::exp(-2 + 0.001), .n = 50'000, .key_range = 20, .seed = 2102},
      Configuration{.eps = 0.01, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 40, .seed = 2103},
      Configuration{.eps = 0.001, .delta = std::exp(-3 + 0.001), .n = 80'000, .key_range = 80, .seed = 2103},
  };

  for (const auto& [eps, delta, n, key_range, seed] : configurations) {
    std::cerr << "Test: eps = " << eps << ", delta = " << delta << ", n = " << n << ", key_range = " << key_range
              << std::endl;

    StickySamplingSketch sketch = CreateSketch(eps, eps, delta);

    std::unordered_map<int64_t, int64_t> exact;
    std::mt19937_64 rnd(seed);
    std::uniform_int_distribution<int64_t> key_dist(0, key_range);
    for (uint64_t i = 0; i < n; ++i) {
      int64_t key = key_dist(rnd);
      sketch.Add(key);
      exact[key] += 1;
    }

    const double error_threshold = eps * static_cast<double>(n);
    uint64_t bad_keys = 0;
    std::vector<double> all_errors;
    for (const auto& [key, true_count] : exact) {
      const int64_t estimate = sketch.Estimate(key);
      const double error = static_cast<double>(true_count - estimate);
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

}  // namespace sketch
