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

enum class PointerAccessKind {
  Read,
  Store,
};

struct PointerAccess {
  std::string value_name;
  PointerAccessKind kind = PointerAccessKind::Read;
};

struct StackLayoutPoint {
  std::vector<std::string> used_names;
  std::vector<PointerAccess> pointer_accesses;
  std::vector<std::string> escaped_names;
  std::optional<std::pair<std::string, std::string>> derived_pointer_root;
};

struct EntryAllocaInput {
  std::string alloca_name;
  std::string type_str;
  int align = 0;
  std::optional<std::string> paired_store_value;
};

struct StackLayoutSignatureParam {
  std::string type;
  std::string operand;
  bool is_varargs = false;
};

struct StackLayoutCallResultInput {
  std::string value_name;
  std::string type_str;
};

struct StackLayoutBlockInput {
  std::string label;
  std::vector<StackLayoutPoint> insts;
  std::vector<std::string> terminator_used_names;
};

struct PhiIncomingUse {
  std::string predecessor_label;
  std::string value_name;
};

struct StackLayoutInput {
  std::vector<EntryAllocaInput> entry_allocas;
  std::vector<StackLayoutSignatureParam> signature_params;
  std::optional<std::string> return_type;
  bool is_variadic = false;
  std::vector<StackLayoutCallResultInput> call_results;
  std::vector<StackLayoutBlockInput> blocks;
  std::vector<PhiIncomingUse> phi_incoming_uses;
};

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

StackLayoutInput lower_lir_to_stack_layout_input(
    const c4c::codegen::lir::LirFunction& function);

std::vector<std::string> collect_stack_layout_value_names(const StackLayoutInput& input);

StackLayoutAnalysis analyze_stack_layout(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs);

}  // namespace c4c::backend::stack_layout
