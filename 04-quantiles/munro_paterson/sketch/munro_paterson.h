#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "sketch/macro.h"

namespace sketch {

class MunroPatersonSketch {
 public:
  using ElementType = int64_t;

  struct Parameters {
    uint64_t k;
  };

  explicit MunroPatersonSketch(Parameters params) { THROW_NOT_IMPLEMENTED; }

  void Add(ElementType value) { THROW_NOT_IMPLEMENTED; }

  void Finalize() { THROW_NOT_IMPLEMENTED; }

  // expected 1-indexing (for example, min value has k = 1)
  std::pair<ElementType, ElementType> KthRange(uint64_t kth) const { THROW_NOT_IMPLEMENTED; }

  std::vector<uint8_t> ToBytes() const { THROW_NOT_IMPLEMENTED; }

  static MunroPatersonSketch FromBytes(const std::vector<uint8_t>& bytes) { THROW_NOT_IMPLEMENTED; }
};

}  // namespace sketch
