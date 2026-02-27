#pragma once

#include <cstdint>

#include "sketch/macro.h"

namespace sketch {

class CountMinSketch {
 public:
  using ElementType = int64_t;
  using CounterType = int64_t;

  struct Parameters {
    uint64_t buffers_count;
    uint64_t buffer_size;
  };

  explicit CountMinSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Add(const ElementType& elem, CounterType weight) { THROW_NOT_IMPLEMENTED; }

  CounterType Estimate(const ElementType& elem) const { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
