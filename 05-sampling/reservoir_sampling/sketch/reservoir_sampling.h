#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "sketch/macro.h"

namespace sketch {

class ReservoirSampling {
 public:
  struct Parameters {
    uint64_t k;
    uint64_t seed;
  };

  explicit ReservoirSampling(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Add(int64_t value) { THROW_NOT_IMPLEMENTED; }

  std::vector<int64_t> Sample() const { THROW_NOT_IMPLEMENTED; }

  std::vector<uint8_t> Serialize() const { THROW_NOT_IMPLEMENTED; }

  static ReservoirSampling Deserialize(const std::vector<uint8_t>& bytes, uint64_t k, uint64_t seed) {
    THROW_NOT_IMPLEMENTED;
  }
};

}  // namespace sketch
