#pragma once

#include "bir.hpp"

#include <string_view>

namespace c4c::backend::bir {

[[nodiscard]] inline bool route_block_matches(std::string_view record_label,
                                              BlockLabelId record_label_id,
                                              const Block& block) {
  if (record_label_id != kInvalidBlockLabel ||
      block.label_id != kInvalidBlockLabel) {
    return record_label_id == block.label_id;
  }
  return record_label == block.label;
}

}  // namespace c4c::backend::bir
