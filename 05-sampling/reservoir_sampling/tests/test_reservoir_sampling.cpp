#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "gtest/gtest.h"
#include "sketch/reservoir_sampling.h"

namespace sketch {

TEST(ReservoirSampling, SampleSizeMatchesK) {
  constexpr uint64_t kSampleSize = 50;

  ReservoirSampling rs(ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101});
  for (int64_t i = 0; i < 1000; ++i) {
    rs.Add(i);
  }

  std::vector<int64_t> sample = rs.Sample();
  ASSERT_EQ(sample.size(), kSampleSize);
}

TEST(ReservoirSampling, SmallStream) {
  constexpr uint64_t kSampleSize = 50;

  ReservoirSampling rs(ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101});
  for (int64_t i = 0; i < 10; ++i) {
    rs.Add(i);
  }

  std::vector<int64_t> sample = rs.Sample();
  ASSERT_EQ(sample.size(), 10u);

  std::sort(sample.begin(), sample.end());
  for (int64_t i = 0; i < 10; ++i) {
    EXPECT_EQ(sample[i], i);
  }
}

// Test (a): 1000 distinct values, sample of size 50.
// Run many times and check that each element is selected with probability k/n.
TEST(ReservoirSampling, ElementSelectionProbability) {
  constexpr int64_t kTotalElements = 1000;
  constexpr uint64_t kSampleSize = 50;
  constexpr int64_t kRuns = 10000;

  std::vector<int64_t> count(kTotalElements, 0);

  for (uint64_t run = 0; run < kRuns; ++run) {
    ReservoirSampling rs(ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101 + run});
    for (int64_t i = 0; i < kTotalElements; ++i) {
      rs.Add(i);
    }

    std::vector<int64_t> sample = rs.Sample();
    ASSERT_EQ(sample.size(), kSampleSize);

    for (int64_t val : sample) {
      ++count[val];
    }
  }

  // Each element should be selected with probability p = k/n.
  // Across `runs` independent runs the count follows Binomial(runs, p).
  const double p = static_cast<double>(kSampleSize) / kTotalElements;
  const double expected = static_cast<double>(kRuns) * p;
  const double stddev = std::sqrt(static_cast<double>(kRuns) * p * (1.0 - p));

  for (int64_t i = 0; i < kTotalElements; ++i) {
    ASSERT_GT(count[i], expected - 5 * stddev)
        << "Element " << i << " selected too rarely: " << count[i] << " times (expected ~" << expected << ")";
    ASSERT_LT(count[i], expected + 5 * stddev)
        << "Element " << i << " selected too often: " << count[i] << " times (expected ~" << expected << ")";
  }
}

// Test (b): 1000 values with 4 unique values (different frequencies).
// Sample of size 3. Run many times and check that each multiset
// appears with the expected multivariate hypergeometric probability:
//   P(multiset) = C(n1,a1) * C(n2,a2) * C(n3,a3) * C(n4,a4) / C(n,k)
TEST(ReservoirSampling, MultisetDistribution) {
  constexpr uint64_t kSampleSize = 3;
  constexpr int64_t kRuns = 100000;

  // 4 unique values with different frequencies:
  //   value 1 appears 400 times
  //   value 2 appears 300 times
  //   value 3 appears 200 times
  //   value 4 appears 100 times
  std::vector<int64_t> values;
  values.reserve(1000);
  for (int i = 0; i < 400; ++i) {
    values.push_back(1);
  }
  for (int i = 0; i < 300; ++i) {
    values.push_back(2);
  }
  for (int i = 0; i < 200; ++i) {
    values.push_back(3);
  }
  for (int i = 0; i < 100; ++i) {
    values.push_back(4);
  }

  const int64_t total_elements = static_cast<int64_t>(values.size());

  std::map<std::vector<int64_t>, int64_t> multiset_count;

  for (uint64_t run = 0; run < kRuns; ++run) {
    ReservoirSampling rs(ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101 + run});
    for (int64_t val : values) {
      rs.Add(val);
    }

    std::vector<int64_t> sample = rs.Sample();
    ASSERT_EQ(sample.size(), kSampleSize);

    std::sort(sample.begin(), sample.end());
    multiset_count[sample]++;
  }

  // C(n, r) - binomial coefficient
  auto comb = [](int64_t nn, int64_t rr) -> double {
    if (rr < 0 || rr > nn) {
      return 0.0;
    }
    if (rr == 0) {
      return 1.0;
    }
    double result = 1.0;
    for (int64_t i = 0; i < rr; ++i) {
      result *= static_cast<double>(nn - i);
      result /= static_cast<double>(i + 1);
    }
    return result;
  };

  const std::vector<int64_t> group_sizes = {400, 300, 200, 100};
  const double total_comb = comb(total_elements, kSampleSize);

  int64_t total_observed = 0;
  for (const auto& [multiset, observed] : multiset_count) {
    total_observed += observed;

    // Count how many times each value appears in this multiset
    std::map<int64_t, int64_t> val_counts;
    for (int64_t v : multiset) {
      ++val_counts[v];
    }

    // Compute expected probability using multivariate hypergeometric distribution
    double prob = 1.0;
    for (int val = 1; val <= 4; ++val) {
      int64_t a = 0;
      if (auto it = val_counts.find(val); it != val_counts.end()) {
        a = it->second;
      }
      prob *= comb(group_sizes[val - 1], a);
    }
    prob /= total_comb;

    double expected_count = kRuns * prob;
    double stddev = std::sqrt(kRuns * prob * (1.0 - prob));

    ASSERT_GT(static_cast<double>(observed), expected_count - 5 * stddev)
        << "Multiset observed too rarely: " << observed << " vs expected " << expected_count;
    ASSERT_LT(static_cast<double>(observed), expected_count + 5 * stddev)
        << "Multiset observed too often: " << observed << " vs expected " << expected_count;
  }

  EXPECT_EQ(total_observed, kRuns);
}

TEST(ReservoirSampling, Serde) {
  constexpr uint64_t kSampleSize = 50;
  constexpr int64_t kTotalElements = 1000;

  ReservoirSampling rs(ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101});
  for (int64_t i = 0; i < 500; ++i) {
    rs.Add(i);
  }

  std::vector<uint8_t> bytes1 = rs.Serialize();
  ASSERT_LE(bytes1.size(), sizeof(int64_t) * kSampleSize + sizeof(int64_t));

  ReservoirSampling rs2 = ReservoirSampling::Deserialize(bytes1, kSampleSize, /*seed=*/42);

  std::vector<uint8_t> bytes2 = rs2.Serialize();
  ASSERT_EQ(bytes1, bytes2);

  for (int64_t i = 500; i < kTotalElements; ++i) {
    rs2.Add(i);
  }
  ASSERT_EQ(rs2.Sample().size(), kSampleSize);
}

TEST(ReservoirSampling, ElementSelectionProbabilityWithSerde) {
  constexpr int64_t kTotalElements = 1000;
  constexpr uint64_t kSampleSize = 50;
  constexpr int64_t kRuns = 10000;
  constexpr int64_t kSerdeInterval = 250;

  std::vector<int64_t> count(kTotalElements, 0);

  for (uint64_t run = 0; run < kRuns; ++run) {
    std::optional<ReservoirSampling> rs(
        ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101 + run});
    uint64_t serde_seed = 1000000 + run * 1000;
    for (int64_t i = 0; i < kTotalElements; ++i) {
      rs->Add(i);
      if ((i + 1) % kSerdeInterval == 0) {
        std::vector<uint8_t> bytes = rs->Serialize();
        rs.emplace(ReservoirSampling::Deserialize(bytes, kSampleSize, serde_seed++));
      }
    }

    std::vector<int64_t> sample = rs->Sample();
    ASSERT_EQ(sample.size(), kSampleSize);

    for (int64_t val : sample) {
      ++count[val];
    }
  }

  const double p = static_cast<double>(kSampleSize) / kTotalElements;
  const double expected = static_cast<double>(kRuns) * p;
  const double stddev = std::sqrt(static_cast<double>(kRuns) * p * (1.0 - p));

  for (int64_t i = 0; i < kTotalElements; ++i) {
    ASSERT_GT(count[i], expected - 5 * stddev)
        << "Element " << i << " selected too rarely: " << count[i] << " times (expected ~" << expected << ")";
    ASSERT_LT(count[i], expected + 5 * stddev)
        << "Element " << i << " selected too often: " << count[i] << " times (expected ~" << expected << ")";
  }
}

TEST(ReservoirSampling, MultisetDistributionWithSerde) {
  constexpr uint64_t kSampleSize = 3;
  constexpr int64_t kRuns = 100000;
  constexpr int64_t kSerdeInterval = 250;

  std::vector<int64_t> values;
  values.reserve(1000);
  for (int i = 0; i < 400; ++i) {
    values.push_back(1);
  }
  for (int i = 0; i < 300; ++i) {
    values.push_back(2);
  }
  for (int i = 0; i < 200; ++i) {
    values.push_back(3);
  }
  for (int i = 0; i < 100; ++i) {
    values.push_back(4);
  }

  const int64_t total_elements = static_cast<int64_t>(values.size());

  std::map<std::vector<int64_t>, int64_t> multiset_count;

  for (uint64_t run = 0; run < kRuns; ++run) {
    std::optional<ReservoirSampling> rs(
        ReservoirSampling::Parameters{.k = kSampleSize, .seed = 2101 + run});
    uint64_t serde_seed = 1000000 + run * 1000;
    for (int64_t idx = 0; idx < total_elements; ++idx) {
      rs->Add(values[idx]);
      if ((idx + 1) % kSerdeInterval == 0) {
        std::vector<uint8_t> bytes = rs->Serialize();
        rs.emplace(ReservoirSampling::Deserialize(bytes, kSampleSize, serde_seed++));
      }
    }

    std::vector<int64_t> sample = rs->Sample();
    ASSERT_EQ(sample.size(), kSampleSize);

    std::sort(sample.begin(), sample.end());
    multiset_count[sample]++;
  }

  // C(n, r) - binomial coefficient
  auto comb = [](int64_t nn, int64_t rr) -> double {
    if (rr < 0 || rr > nn) {
      return 0.0;
    }
    if (rr == 0) {
      return 1.0;
    }
    double result = 1.0;
    for (int64_t i = 0; i < rr; ++i) {
      result *= static_cast<double>(nn - i);
      result /= static_cast<double>(i + 1);
    }
    return result;
  };

  const std::vector<int64_t> group_sizes = {400, 300, 200, 100};
  const double total_comb = comb(total_elements, kSampleSize);

  int64_t total_observed = 0;
  for (const auto& [multiset, observed] : multiset_count) {
    total_observed += observed;

    std::map<int64_t, int64_t> val_counts;
    for (int64_t v : multiset) {
      ++val_counts[v];
    }

    double prob = 1.0;
    for (int val = 1; val <= 4; ++val) {
      int64_t a = 0;
      if (auto it = val_counts.find(val); it != val_counts.end()) {
        a = it->second;
      }
      prob *= comb(group_sizes[val - 1], a);
    }
    prob /= total_comb;

    double expected_count = kRuns * prob;
    double stddev = std::sqrt(kRuns * prob * (1.0 - prob));

    ASSERT_GT(static_cast<double>(observed), expected_count - 5 * stddev)
        << "Multiset observed too rarely: " << observed << " vs expected " << expected_count;
    ASSERT_LT(static_cast<double>(observed), expected_count + 5 * stddev)
        << "Multiset observed too often: " << observed << " vs expected " << expected_count;
  }

  EXPECT_EQ(total_observed, kRuns);
}

}  // namespace sketch
