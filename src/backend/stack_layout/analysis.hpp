#pragma once

#include "../generation.hpp"
#include "../regalloc.hpp"
#include "../../codegen/lir/ir.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend::stack_layout {

struct StackLayoutAnalysis {
  std::unordered_map<std::string, std::vector<std::size_t>> value_use_blocks;
  std::unordered_set<std::string> used_values;
  std::unordered_set<std::string> dead_param_allocas;
  std::unordered_set<std::string> entry_allocas_overwritten_before_read;

  [[nodiscard]] const std::vector<std::size_t>* find_use_blocks(
      std::string_view value_name) const;
  [[nodiscard]] bool uses_value(std::string_view value_name) const;
  [[nodiscard]] bool is_dead_param_alloca(std::string_view value_name) const;
  [[nodiscard]] bool is_entry_alloca_overwritten_before_read(
      std::string_view value_name) const;
};

StackLayoutAnalysis analyze_stack_layout(
    const c4c::codegen::lir::LirFunction& function,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs);

}  // namespace c4c::backend::stack_layout
