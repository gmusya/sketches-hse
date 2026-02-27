#pragma once

#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "sketch/macro.h"

namespace sketch {

class MisraGriesSketch {
 public:
  using ElementType = int64_t;
  using CounterType = uint64_t;

  struct Parameters {
    uint64_t memory_limit_bytes;
  };

  explicit MisraGriesSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Append(const ElementType& value) { THROW_NOT_IMPLEMENTED; }

  std::unordered_map<ElementType, CounterType> Candidates() const { THROW_NOT_IMPLEMENTED; }

  CounterType Estimate(const ElementType& value) { THROW_NOT_IMPLEMENTED; }

  std::vector<uint8_t> ToBytes() const { THROW_NOT_IMPLEMENTED; }

  static MisraGriesSketch FromBytes(const std::vector<uint8_t>& bytes) { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
