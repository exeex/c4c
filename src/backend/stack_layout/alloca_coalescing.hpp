#pragma once

#include "analysis.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace c4c::backend::stack_layout {

struct CoalescableAllocas {
  std::unordered_map<std::string, std::size_t> single_block_allocas;
  std::unordered_set<std::string> dead_allocas;

  [[nodiscard]] std::optional<std::size_t> find_single_block(
      std::string_view alloca_name) const;
  [[nodiscard]] bool is_dead_alloca(std::string_view alloca_name) const;
};

CoalescableAllocas compute_coalescable_allocas(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

CoalescableAllocas compute_coalescable_allocas(
    const c4c::codegen::lir::LirFunction& function,
    const StackLayoutAnalysis& analysis);

}  // namespace c4c::backend::stack_layout
