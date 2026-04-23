#pragma once

#include "../../../../prealloc/prealloc.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend::x86 {

struct MaterializedI32Compare {
  std::string_view i1_name;
  std::optional<std::string_view> i32_name;
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  std::string compare_setup;
};

struct PreparedI32NamedImmediateCompareSelection {
  const c4c::backend::bir::Value* named_value = nullptr;
  std::int64_t immediate = 0;
};

struct ShortCircuitEntryCompareContext {
  const c4c::backend::prepare::PreparedBranchCondition* branch_condition = nullptr;
  std::string compare_setup;
  std::string false_branch_opcode;
};

struct ShortCircuitTarget {
  const c4c::backend::bir::Block* block = nullptr;
  std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels> continuation;
};

struct ShortCircuitPlan {
  ShortCircuitTarget on_compare_true;
  ShortCircuitTarget on_compare_false;
};

struct CompareDrivenBranchRenderPlan {
  ShortCircuitPlan branch_plan;
  std::string compare_setup;
  std::string false_branch_opcode;
};

struct PreparedModuleLocalSlotLayout {
  std::unordered_map<std::string_view, std::size_t> offsets;
  std::unordered_map<c4c::backend::prepare::PreparedFrameSlotId, std::size_t>
      frame_slot_offsets;
  std::size_t frame_size = 0;
  std::size_t frame_alignment = 0;
};

struct PreparedModuleMultiDefinedDispatchState {
  std::string helper_prefix;
  std::unordered_set<std::string_view> helper_names;
  std::unordered_set<std::string_view> helper_string_names;
  std::unordered_set<std::string_view> helper_global_names;
  std::optional<std::string> rendered_module;
  bool has_bounded_same_module_helpers = false;
};

struct PreparedX86FunctionDispatchContext;

struct PreparedX86BlockDispatchContext {
  const PreparedX86FunctionDispatchContext* function_context = nullptr;
  const c4c::backend::bir::Block* block = nullptr;
  const PreparedModuleLocalSlotLayout* local_layout = nullptr;
  c4c::BlockLabelId block_label_id = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
};

struct PreparedX86FunctionDispatchContext {
  const c4c::backend::prepare::PreparedBirModule* prepared_module = nullptr;
  const c4c::backend::bir::Module* module = nullptr;
  const c4c::backend::bir::Function* function = nullptr;
  const c4c::backend::bir::Block* entry = nullptr;
  const c4c::backend::prepare::PreparedStackLayout* stack_layout = nullptr;
  const c4c::backend::prepare::PreparedAddressingFunction* function_addressing = nullptr;
  const c4c::backend::prepare::PreparedNameTables* prepared_names = nullptr;
  const c4c::backend::prepare::PreparedValueLocationFunction* function_locations = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow = nullptr;
  c4c::TargetArch prepared_arch = c4c::TargetArch::Unknown;
  std::string_view asm_prefix;
  std::string_view return_register;
  const std::unordered_set<std::string_view>* bounded_same_module_helper_names = nullptr;
  const std::unordered_set<std::string_view>* bounded_same_module_helper_global_names = nullptr;
  std::function<const c4c::backend::bir::Block*(std::string_view)> find_block;
  std::function<const c4c::backend::bir::StringConstant*(std::string_view)> find_string_constant;
  std::function<const c4c::backend::bir::Global*(std::string_view)> find_same_module_global;
  std::function<bool(const c4c::backend::bir::Global&, c4c::backend::bir::TypeKind, std::size_t)>
      same_module_global_supports_scalar_load;
  std::function<std::string(std::string_view)> render_private_data_label;
  std::function<std::string(std::string_view)> render_asm_symbol_name;
  std::function<std::string(const c4c::backend::bir::StringConstant&)> emit_string_constant_data;
  std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>
      emit_same_module_global_data;
  std::function<std::string(std::string)> prepend_bounded_same_module_helpers;
  std::function<std::optional<std::string>(const c4c::backend::bir::Param&)> minimal_param_register;
  std::unordered_set<std::string_view>* used_string_names = nullptr;
  std::unordered_set<std::string_view>* used_same_module_global_names = nullptr;
  bool defer_module_data_emission = false;

  PreparedX86BlockDispatchContext make_block_context(
      const c4c::backend::bir::Block& block,
      const PreparedModuleLocalSlotLayout& local_layout) const {
    std::size_t block_index = 0;
    if (function != nullptr) {
      for (; block_index < function->blocks.size(); ++block_index) {
        if (&function->blocks[block_index] == &block) {
          break;
        }
      }
      if (block_index == function->blocks.size()) {
        block_index = 0;
      }
    }
    const c4c::BlockLabelId block_label_id =
        prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                  : c4c::backend::prepare::resolve_prepared_block_label_id(
                                        *prepared_names, block.label)
                                        .value_or(c4c::kInvalidBlockLabel);
    return PreparedX86BlockDispatchContext{
        .function_context = this,
        .block = &block,
        .local_layout = &local_layout,
        .block_label_id = block_label_id,
        .block_index = block_index,
    };
  }
};

PreparedModuleMultiDefinedDispatchState build_prepared_module_multi_defined_dispatch_state(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function* entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<std::string> render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function& candidate,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix);

std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i32_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_countdown_entry_routes_if_supported(
    const PreparedX86FunctionDispatchContext& context);

}  // namespace c4c::backend::x86
