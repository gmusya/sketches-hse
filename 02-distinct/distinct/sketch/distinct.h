#pragma once

#include <cstdint>
#include <vector>

#include "sketch/macro.h"

namespace sketch {

class DistinctSketch {
 public:
  struct Parameters {
    uint64_t memory_limit_bytes;
  };

  DistinctSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Append(int64_t value) { THROW_NOT_IMPLEMENTED; }

  double Estimate() const { THROW_NOT_IMPLEMENTED; }

  std::vector<uint8_t> ToBytes() const { THROW_NOT_IMPLEMENTED; }

  static DistinctSketch FromBytes(const std::vector<uint8_t>& bytes) { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
