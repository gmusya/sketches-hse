#pragma once

#include <cstdint>

#include "sketch/macro.h"

namespace sketch {

class StickySamplingSketch {
 public:
  using ElementType = int64_t;
  using CounterType = uint64_t;

  struct Parameters {
    double s;
    double eps;
    double delta;
  };

  explicit StickySamplingSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Add(const ElementType& elem) { THROW_NOT_IMPLEMENTED; }

  CounterType Estimate(const ElementType& elem) const { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
