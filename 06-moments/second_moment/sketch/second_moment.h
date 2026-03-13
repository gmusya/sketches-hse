#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "sketch/macro.h"

namespace sketch {

class SecondMomentSketch {
 public:
  struct Parameters {
    uint64_t seed;
  };

  explicit SecondMomentSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Add(int64_t value) { THROW_NOT_IMPLEMENTED; }

  int64_t Estimate() const { THROW_NOT_IMPLEMENTED; }

  std::vector<uint8_t> Serialize() const { THROW_NOT_IMPLEMENTED; }

  static SecondMomentSketch Deserialize(const std::vector<uint8_t>& bytes) { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
