#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/misra_gries.h"

namespace sketch {

MisraGriesSketch CreateSketch(uint64_t max_elements) {
  return MisraGriesSketch(MisraGriesSketch::Parameters{
      .memory_limit_bytes =
          max_elements * (sizeof(MisraGriesSketch::ElementType) + sizeof(MisraGriesSketch::CounterType))});
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

TEST(MisraGries, Simple) {
  constexpr uint64_t kMaxElements = 4;
  MisraGriesSketch sketch = CreateSketch(kMaxElements);

  std::unordered_map<int64_t, uint64_t> counters = {{1, 123}, {2, 15}, {3, 16}, {4, 17}, {5, 150},
                                                    {6, 10},  {7, 8},  {8, 1},  {9, 10}};
  std::vector<int64_t> values = Shuffle(Unpack(counters), 2101);
  const uint64_t total_values = values.size();
  const uint64_t max_allowed_error = total_values / kMaxElements;

  for (int64_t value : values) {
    sketch.Append(value);
  }

  for (const auto& [k, expected] : counters) {
    uint64_t estimation = sketch.Estimate(k);

    uint64_t difference = std::abs(static_cast<int64_t>(estimation) - static_cast<int64_t>(expected));
    EXPECT_LE(difference, max_allowed_error) << "Bad prediction for key " << k;
  }

  auto candidates = sketch.Candidates();
  for (const auto& [k, v] : candidates) {
    ASSERT_EQ(v, sketch.Estimate(k)) << "Inconsistent result for key " << k;
  }

  for (const auto& [k, v] : counters) {
    if (v > max_allowed_error) {
      ASSERT_TRUE(std::find_if(candidates.begin(), candidates.end(),
                               [&](const auto& elem) { return elem.first == k; }) != candidates.end())
          << "Frequent key " << k << " is not found in candidates";
    }
  }
}

TEST(MisraGries, Empty) {
  MisraGriesSketch sketch = CreateSketch(4);
  ASSERT_EQ(sketch.Estimate(42), 0);
  ASSERT_TRUE(sketch.Candidates().empty());
}

TEST(MisraGries, FitsInMemory) {
  MisraGriesSketch sketch = CreateSketch(8);
  std::unordered_map<int64_t, uint64_t> counters = {{-10, 3}, {5, 1}, {12, 8}, {1024, 2}};

  for (int64_t value : Unpack(counters)) {
    sketch.Append(value);
  }

  auto candidates = sketch.Candidates();
  ASSERT_EQ(candidates.size(), counters.size());

  for (const auto& [key, expected] : counters) {
    ASSERT_EQ(sketch.Estimate(key), expected);
    ASSERT_EQ(candidates.at(key), expected);
  }
}

TEST(MisraGries, SerdePreservesEstimates) {
  constexpr uint64_t kMaxElements = 5;
  std::optional<MisraGriesSketch> sketch(CreateSketch(kMaxElements));

  std::unordered_map<int64_t, uint64_t> counters = {{-10, 13}, {1, 123}, {2, 15}, {3, 16}, {4, 17},
                                                    {5, 150},  {6, 10},  {7, 8},  {8, 1},  {9, 10}};
  std::vector<int64_t> values = Shuffle(Unpack(counters), 2101);
  for (int64_t value : values) {
    sketch->Append(value);
  }

  std::unordered_map<int64_t, uint64_t> before_candidates = sketch->Candidates();
  std::unordered_map<int64_t, uint64_t> before_estimates;
  for (const auto& [key, _] : counters) {
    before_estimates[key] = sketch->Estimate(key);
  }

  std::vector<uint8_t> bytes = sketch->ToBytes();
  constexpr uint64_t kBytesLimit =
      kMaxElements * ((sizeof(MisraGriesSketch::CounterType) + sizeof(MisraGriesSketch::ElementType)));

  ASSERT_LE(bytes.size(), kBytesLimit + 100);
  sketch.emplace(MisraGriesSketch::FromBytes(bytes));

  std::unordered_map<int64_t, uint64_t> after_candidates = sketch->Candidates();
  ASSERT_EQ(before_candidates, after_candidates);
  for (const auto& [key, expected] : before_estimates) {
    ASSERT_EQ(sketch->Estimate(key), expected);
  }
}

}  // namespace sketch
