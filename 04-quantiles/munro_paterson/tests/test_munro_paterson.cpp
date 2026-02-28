#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/munro_paterson.h"

namespace sketch {

MunroPatersonSketch CreateSketch(uint64_t k) {
  return MunroPatersonSketch(MunroPatersonSketch::Parameters{
      .k = k,
  });
}

class KthFinder {
 private:
  struct MinMax {
    int64_t min;
    int64_t max;
  };

 public:
  explicit KthFinder(const std::vector<int64_t>& values, uint64_t buffer_size, int64_t pass_hardlimit)
      : values_(values), buffer_size_(buffer_size), pass_hardlimit_(pass_hardlimit) {}

  struct Result {
    int64_t value;
    int64_t passes;
    int64_t max_memory_consumption;
  };

  Result Find(uint64_t kth) {
    int64_t passes = 0;
    int64_t max_memory_consumption = 0;

    std::optional<MinMax> old_min_max;
    std::optional<MinMax> min_max;

    while (passes != pass_hardlimit_ && (!min_max.has_value() || min_max->min != min_max->max)) {
      int64_t elements_added = 0;

      ++passes;
      std::optional<MunroPatersonSketch> sketch = CreateSketch(buffer_size_);

      for (int64_t elem : values_) {
        if (IsNewLess(old_min_max, min_max, elem)) {
          --kth;
        }
        if (Matches(min_max, elem)) {
          ++elements_added;
          sketch->Add(elem);

          if (elements_added % buffer_size_ == buffer_size_ - 2) {
            std::vector<uint8_t> bytes = sketch->ToBytes();
            max_memory_consumption = std::max(max_memory_consumption, static_cast<int64_t>(bytes.size()));

            sketch.emplace(MunroPatersonSketch::FromBytes(bytes));
          }
        }
      }

      sketch->Finalize();
      auto [min, max] = sketch->KthRange(kth);

      old_min_max = min_max;
      min_max = MinMax{.min = min, .max = max};
    }

    return Result{.value = min_max->min, .passes = passes, .max_memory_consumption = max_memory_consumption};
  }

 private:
  static bool Matches(const std::optional<MinMax>& filter, int64_t element) {
    return !filter.has_value() || (filter->min <= element && element <= filter->max);
  }

  static bool IsGreater(const std::optional<MinMax>& old, int64_t element) {
    if (!old.has_value()) {
      return false;
    }
    return element < old->min;
  }

  static bool IsNewLess(const std::optional<MinMax>& old_filter, const std::optional<MinMax>& new_filter,
                        int64_t value) {
    return !IsGreater(old_filter, value) && IsGreater(new_filter, value);
  }

  const std::vector<int64_t>& values_;
  const uint64_t buffer_size_;
  const int64_t pass_hardlimit_;
};

TEST(MunroPatersonSketch, FindKth) {
  struct Configuration {
    uint64_t buffer_size;
    uint64_t total_elements;
    uint64_t k;

    uint64_t passes;
  };

  const std::vector<Configuration> configurations = {
      Configuration{.buffer_size = 64, .total_elements = 50'000, .k = 25'000, .passes = 6},
      Configuration{.buffer_size = 64, .total_elements = 50'000, .k = 1, .passes = 6},
      Configuration{.buffer_size = 64, .total_elements = 50'000, .k = 50'000, .passes = 6},
      Configuration{.buffer_size = 32, .total_elements = 50'000, .k = 25'000, .passes = 8},
      Configuration{.buffer_size = 128, .total_elements = 50'000, .k = 25'000, .passes = 4},
      Configuration{.buffer_size = 1024, .total_elements = 50'000, .k = 25'000, .passes = 2},
  };

  for (const auto& [buffer_size, total_elements, k, passes_limit] : configurations) {
    const uint64_t expected_height = std::log2(total_elements / buffer_size) + 1;
    const uint64_t memory_limit = (buffer_size * sizeof(int64_t) + 100) * (expected_height + 1);

    std::cerr << "Test: buffer_size = " << buffer_size << ", total_elements = " << total_elements << ", k = " << k
              << ", passes_limit = " << passes_limit << ", memory_limit = " << memory_limit << std::endl;

    std::mt19937_64 rnd(2101);
    std::uniform_int_distribution<int64_t> value_dist(static_cast<int64_t>(-1e18), static_cast<int64_t>(1e18));

    std::vector<int64_t> values;
    values.reserve(total_elements);
    for (uint64_t i = 0; i < total_elements; ++i) {
      values.emplace_back(value_dist(rnd));
    }

    std::vector<int64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const int64_t exact_value = sorted[k - 1];

    KthFinder finder(values, buffer_size, /* some pass hardlimit */ 16);
    KthFinder::Result result = finder.Find(k);

    std::cerr << "passes = " << result.passes << ", memory = " << result.max_memory_consumption << std::endl;

    EXPECT_EQ(result.value, exact_value);
    EXPECT_LE(result.passes, passes_limit);
    EXPECT_LE(result.max_memory_consumption, memory_limit);

    std::cerr << std::string(80, '-') << std::endl;
  }
}

}  // namespace sketch
