#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/second_moment.h"

namespace sketch {

TEST(SecondMoment, Empty) {
  SecondMomentSketch sketch(SecondMomentSketch::Parameters{.seed = 2101});
  ASSERT_EQ(sketch.Estimate(), 0.0);
}

TEST(SecondMoment, AllSameElements) {
  constexpr uint64_t kElements = 1000;

  for (uint64_t seed = 0; seed < 100; ++seed) {
    SecondMomentSketch sketch(SecondMomentSketch::Parameters{.seed = seed});
    for (uint64_t i = 0; i < kElements; ++i) {
      sketch.Add(42);
    }

    int64_t estimate = sketch.Estimate();
    int64_t true_f2 = static_cast<int64_t>(kElements) * kElements;
    ASSERT_EQ(estimate, true_f2) << "seed = " << seed;
  }
}

TEST(SecondMoment, Errors) {
  struct Configuration {
    uint64_t runs;
    uint64_t n;
    int64_t key_range;
    uint64_t data_seed;
  };

  const std::vector<Configuration> configurations = {
      Configuration{.runs = 1000, .n = 10000, .key_range = 100, .data_seed = 2101},
      Configuration{.runs = 1000, .n = 10000, .key_range = 1000, .data_seed = 2102},
      Configuration{.runs = 1000, .n = 10000, .key_range = 10000, .data_seed = 2103},
      Configuration{.runs = 1000, .n = 50000, .key_range = 500, .data_seed = 2104},
  };

  for (const auto& [runs, n, key_range, data_seed] : configurations) {
    std::cerr << "Test: runs = " << runs << ", n = " << n << ", key_range = " << key_range << std::endl;

    std::mt19937_64 data_rng(data_seed);
    std::uniform_int_distribution<int64_t> key_dist(0, key_range - 1);
    std::vector<int64_t> stream;
    stream.reserve(n);
    for (uint64_t i = 0; i < n; ++i) {
      stream.push_back(key_dist(data_rng));
    }

    std::unordered_map<int64_t, int64_t> freq;
    for (int64_t v : stream) {
      freq[v]++;
    }
    int64_t true_f2 = 0;
    int64_t true_f4 = 0;
    for (const auto& [v, f] : freq) {
      true_f2 += static_cast<int64_t>(f) * f;
      true_f4 += static_cast<int64_t>(f) * f * f * f;
    }

    int64_t estimation_sum = 0;
    int64_t estimation_squared_sum = 0;
    std::vector<double> predictions;

    for (uint64_t run = 0; run < runs; ++run) {
      SecondMomentSketch sketch(SecondMomentSketch::Parameters{.seed = run});
      for (int64_t v : stream) {
        sketch.Add(v);
      }
      int64_t est = sketch.Estimate();
      predictions.push_back(est);
    }

    for (int64_t est : predictions) {
      estimation_sum += est;
      estimation_squared_sum += est * est;
    }

    double estimated_average = static_cast<double>(estimation_sum) / runs;
    double relative_error = estimated_average / true_f2;

    double estimated_variance =
        (static_cast<double>(estimation_squared_sum) - static_cast<double>(estimation_sum) * estimated_average) /
        (runs - 1);
    double rse = sqrt(estimated_variance / (true_f2 * true_f2));

    double true_variance = 2.0 * (static_cast<double>(true_f2) * true_f2 - static_cast<double>(true_f4));
    double true_rse = sqrt(true_variance / (true_f2 * true_f2));

    std::cerr << "average = " << estimated_average << ", variance = " << estimated_variance << ", rse = " << rse
              << std::endl;
    std::cerr << "true_f2 = " << true_f2 << ", true_variance = " << true_variance << ", true_rse = " << true_rse
              << std::endl;

    EXPECT_LT(relative_error, 1 + 3 * true_rse / sqrt(runs));

    EXPECT_LT(rse, true_rse * 1.15);

    std::cerr << std::string(80, '-') << std::endl;
  }
}

TEST(SecondMoment, Serde) {
  constexpr uint64_t kElements = 10000;

  SecondMomentSketch sketch(SecondMomentSketch::Parameters{.seed = 2101});
  for (uint64_t i = 0; i < kElements / 2; ++i) {
    sketch.Add(static_cast<int64_t>(i % 100));
  }

  std::vector<uint8_t> bytes1 = sketch.Serialize();
  ASSERT_EQ(bytes1.size(), sizeof(uint64_t) + sizeof(int64_t));

  SecondMomentSketch sketch2 = SecondMomentSketch::Deserialize(bytes1);

  std::vector<uint8_t> bytes2 = sketch2.Serialize();
  ASSERT_EQ(bytes1, bytes2);

  ASSERT_EQ(sketch.Estimate(), sketch2.Estimate());

  for (uint64_t i = kElements / 2; i < kElements; ++i) {
    sketch.Add(static_cast<int64_t>(i % 100));
    sketch2.Add(static_cast<int64_t>(i % 100));
  }

  ASSERT_EQ(sketch.Estimate(), sketch2.Estimate());
}

TEST(SecondMoment, ErrorsWithSerde) {
  struct Configuration {
    uint64_t runs;
    uint64_t n;
    int64_t key_range;
    uint64_t data_seed;
  };

  const std::vector<Configuration> configurations = {
      Configuration{.runs = 1000, .n = 10000, .key_range = 100, .data_seed = 2101},
      Configuration{.runs = 1000, .n = 10000, .key_range = 1000, .data_seed = 2102},
      Configuration{.runs = 1000, .n = 10000, .key_range = 10000, .data_seed = 2103},
      Configuration{.runs = 1000, .n = 50000, .key_range = 500, .data_seed = 2104},
  };

  constexpr uint64_t kSerdeInterval = 50;

  for (const auto& [runs, n, key_range, data_seed] : configurations) {
    std::cerr << "Test (serde): runs = " << runs << ", n = " << n << ", key_range = " << key_range << std::endl;

    std::mt19937_64 data_rng(data_seed);
    std::uniform_int_distribution<int64_t> key_dist(0, key_range - 1);
    std::vector<int64_t> stream;
    stream.reserve(n);
    for (uint64_t i = 0; i < n; ++i) {
      stream.push_back(key_dist(data_rng));
    }

    std::unordered_map<int64_t, int64_t> freq;
    for (int64_t v : stream) {
      freq[v]++;
    }
    int64_t true_f2 = 0;
    int64_t true_f4 = 0;
    for (const auto& [v, f] : freq) {
      true_f2 += static_cast<int64_t>(f) * f;
      true_f4 += static_cast<int64_t>(f) * f * f * f;
    }

    int64_t estimation_sum = 0;
    int64_t estimation_squared_sum = 0;
    std::vector<double> predictions;

    for (uint64_t run = 0; run < runs; ++run) {
      std::optional<SecondMomentSketch> sketch(SecondMomentSketch::Parameters{.seed = run});
      for (uint64_t i = 0; i < n; ++i) {
        sketch->Add(stream[i]);
        if ((i + 1) % kSerdeInterval == 0) {
          std::vector<uint8_t> bytes = sketch->Serialize();
          sketch.emplace(SecondMomentSketch::Deserialize(bytes));
        }
      }
      int64_t est = sketch->Estimate();
      predictions.push_back(est);
    }

    for (int64_t est : predictions) {
      estimation_sum += est;
      estimation_squared_sum += est * est;
    }

    double estimated_average = static_cast<double>(estimation_sum) / runs;
    double relative_error = estimated_average / true_f2;

    double estimated_variance =
        (static_cast<double>(estimation_squared_sum) - static_cast<double>(estimation_sum) * estimated_average) /
        (runs - 1);
    double rse = sqrt(estimated_variance / (true_f2 * true_f2));

    double true_variance = 2.0 * (static_cast<double>(true_f2) * true_f2 - static_cast<double>(true_f4));
    double true_rse = sqrt(true_variance / (true_f2 * true_f2));

    std::cerr << "average = " << estimated_average << ", variance = " << estimated_variance << ", rse = " << rse
              << std::endl;
    std::cerr << "true_f2 = " << true_f2 << ", true_variance = " << true_variance << ", true_rse = " << true_rse
              << std::endl;

    EXPECT_LT(relative_error, 1 + 3 * true_rse / sqrt(runs));

    EXPECT_LT(rse, true_rse * 1.15);

    std::cerr << std::string(80, '-') << std::endl;
  }
}

}  // namespace sketch
