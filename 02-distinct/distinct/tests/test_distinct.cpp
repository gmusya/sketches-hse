#include <optional>
#include <random>
#include <unordered_set>

#include "gtest/gtest.h"
#include "sketch/distinct.h"

namespace sketch {

TEST(Distinct, Empty) {
  DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = 100 * sizeof(uint64_t)});

  ASSERT_EQ(sketch.Estimate(), 0);
}

// this test is correct only for kmv sketch
#if 0
TEST(Distinct, SmallNumberOfElements) {
  constexpr uint64_t kDistinctElements = 100;
  constexpr uint64_t kAllowedElements = kDistinctElements + 1;

  DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = kAllowedElements * sizeof(uint64_t)});

  for (uint64_t i = 1; i <= kDistinctElements; ++i) {
    sketch.Append(i);
    ASSERT_EQ(sketch.Estimate(), i);
  }
}
#endif

TEST(Distinct, LargeNumberOfElements) {
  constexpr uint64_t kDistinctElements = 100'000;
  constexpr uint64_t kAllowedElements = 40;

  DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = kAllowedElements * sizeof(uint64_t)});

  for (uint64_t i = 1; i <= kDistinctElements; ++i) {
    sketch.Append(i);
  }

  double estimate = sketch.Estimate();
  ASSERT_LE(kDistinctElements / 10, estimate);
  ASSERT_LE(estimate, kDistinctElements * 10);
}

TEST(Distinct, Serde) {
  constexpr uint64_t kDistinctElements = 100'000;
  constexpr uint64_t kAllowedElements = 40;

  constexpr uint64_t kBytesLimit = kAllowedElements * sizeof(uint64_t);

  std::optional<DistinctSketch> sketch(
      DistinctSketch::Parameters{.memory_limit_bytes = kBytesLimit});

  for (uint64_t i = 1; i <= kDistinctElements; ++i) {
    sketch->Append(i);
    double old_estimate = sketch->Estimate();

    std::vector<uint8_t> bytes = sketch->ToBytes();
    ASSERT_LE(bytes.size(), kBytesLimit + 100);

    sketch.emplace(DistinctSketch::FromBytes(bytes));

    ASSERT_EQ(bytes, sketch->ToBytes());

    double new_estimate = sketch->Estimate();
    ASSERT_EQ(old_estimate, new_estimate);
  }

  double estimate = sketch->Estimate();
  ASSERT_LE(kDistinctElements / 10, estimate);
  ASSERT_LE(estimate, kDistinctElements * 10);
}

TEST(Distinct, EstimateIsMonotonic) {
  constexpr uint64_t kDistinctElements = 100'000;
  constexpr uint64_t kAllowedElements = 4;

  DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = kAllowedElements * sizeof(uint64_t)});

  std::vector<double> estimate;

  for (uint64_t i = 1; i <= kDistinctElements; ++i) {
    sketch.Append(i);
    estimate.emplace_back(sketch.Estimate());
  }

  for (uint64_t i = 0; i + 1 < estimate.size(); ++i) {
    ASSERT_LE(estimate[i], estimate[i + 1]);
  }
}

TEST(Distinct, DuplicatesDoNotAffectEstimate) {
  constexpr uint64_t kDistinctElements = 100'000;
  constexpr uint64_t kAllowedElements = 4;

  DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = kAllowedElements * sizeof(uint64_t)});

  std::mt19937 rnd(2101);

  for (uint64_t i = 1; i <= kDistinctElements; ++i) {
    sketch.Append(i);
    while (rnd() & 1) {
      double old_estimate = sketch.Estimate();
      sketch.Append((rnd() % i) + 1);
      double new_estimate = sketch.Estimate();
      ASSERT_EQ(old_estimate, new_estimate);
    }
  }
}

TEST(Distinct, Errors) {
  struct Configuration {
    uint64_t runs;
    uint64_t allowed_elements;
    uint64_t distinct_elements;
  };

  std::vector<Configuration> configurations{
      Configuration{.runs = 100, .allowed_elements = 10, .distinct_elements = 1000},
      Configuration{.runs = 100, .allowed_elements = 10, .distinct_elements = 10000},
      Configuration{.runs = 100, .allowed_elements = 10, .distinct_elements = 100000},
      Configuration{.runs = 100, .allowed_elements = 100, .distinct_elements = 1000},
      Configuration{.runs = 100, .allowed_elements = 100, .distinct_elements = 10000},
      Configuration{.runs = 100, .allowed_elements = 100, .distinct_elements = 100000},
  };

  for (const auto& [runs, allowed_elements, distinct_elements] : configurations) {
    std::cerr << "Test: runs = " << runs << ", k = " << allowed_elements << ", d = " << distinct_elements << std::endl;
    double estimation_sum = 0;
    double estimation_squared_sum = 0;

    std::vector<double> predictions;

    for (uint32_t run = 0; run < runs; ++run) {
      DistinctSketch sketch(DistinctSketch::Parameters{.memory_limit_bytes = allowed_elements * sizeof(uint64_t)});

      std::vector<uint64_t> unique_elements_as_vec;
      std::unordered_set<uint64_t> unique_elements;
      std::mt19937_64 random_value(2101 + run);
      for (uint32_t i = 1; i <= distinct_elements; ++i) {
        uint64_t val = random_value();
        while (unique_elements.contains(val)) {
          val = random_value();
        }
        unique_elements.insert(val);
        sketch.Append(val);
      }

      double estimation = sketch.Estimate();
      predictions.emplace_back(estimation);
    }

    for (double estimation : predictions) {
      estimation_sum += estimation;
      estimation_squared_sum += estimation * estimation;
    }

    double estimated_average = estimation_sum / runs;
    double estimated_variance = (estimation_squared_sum - estimation_sum * estimated_average) / (runs - 1);

    double rse = sqrt(estimated_variance / (distinct_elements * distinct_elements));

    std::cerr << "average = " << estimated_average << ", variance = " << estimated_variance << ", rse = " << rse
              << std::endl;
    std::cerr << std::string(80, '-') << std::endl;

    double relative_error = std::max(estimated_average / distinct_elements, distinct_elements / estimated_variance);
    EXPECT_LT(relative_error, 1 + 1 / sqrt(allowed_elements - 2) * 1.15);
    EXPECT_LT(rse, 1 / sqrt(allowed_elements - 2) * 1.15);
  }
}

TEST(Distinct, ErrorsWithSerde) {
  struct Configuration {
    uint64_t allowed_elements;
    uint64_t distinct_elements;
  };

  std::vector<Configuration> configurations{
      Configuration{.allowed_elements = 10, .distinct_elements = 1000},
      Configuration{.allowed_elements = 10, .distinct_elements = 10000},
      Configuration{.allowed_elements = 10, .distinct_elements = 100000},
      Configuration{.allowed_elements = 100, .distinct_elements = 1000},
      Configuration{.allowed_elements = 100, .distinct_elements = 10000},
      Configuration{.allowed_elements = 100, .distinct_elements = 100000},
  };

  for (const auto& [allowed_elements, distinct_elements] : configurations) {
    std::cerr << "Test: k = " << allowed_elements << ", d = " << distinct_elements << std::endl;

    auto run = [&](bool with_serde) -> std::vector<double> {
      std::vector<double> predictions;

      std::optional<DistinctSketch> sketch(
          DistinctSketch::Parameters{.memory_limit_bytes = allowed_elements * sizeof(uint64_t)});

      std::unordered_set<uint64_t> unique_elements;
      std::mt19937_64 random_value(2101);
      for (uint32_t i = 1; i <= distinct_elements; ++i) {
        uint64_t val = random_value();
        while (unique_elements.contains(val)) {
          val = random_value();
        }
        unique_elements.insert(val);
        sketch->Append(val);

        if (with_serde) {
          std::vector<uint8_t> serialized_sketch = sketch->ToBytes();

          sketch.reset();
          sketch.emplace(DistinctSketch::FromBytes(serialized_sketch));
        }
      }

      double estimation = sketch->Estimate();
      predictions.emplace_back(estimation);

      return predictions;
    };

    std::vector<double> predictions_default = run(false);
    std::vector<double> predictions_with_serde = run(true);

    EXPECT_EQ(predictions_default, predictions_with_serde);
  }
}

}  // namespace sketch
