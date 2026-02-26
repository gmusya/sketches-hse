#include <optional>
#include <random>
#include <unordered_set>

#include "gtest/gtest.h"
#include "sketch/distinct.h"

namespace sketch {

TEST(Distinct, Simple) {
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
      sketch::DistinctSketch sketch(
          sketch::DistinctSketch::Parameters{.memory_limit_bytes = allowed_elements * sizeof(uint64_t)});

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

TEST(Distinct, SerDe) {
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

      std::optional<sketch::DistinctSketch> sketch(
          sketch::DistinctSketch::Parameters{.memory_limit_bytes = allowed_elements * sizeof(uint64_t)});

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
