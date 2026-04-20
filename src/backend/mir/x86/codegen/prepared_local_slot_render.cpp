#include "x86_codegen.hpp"

#include <algorithm>

namespace c4c::backend::x86 {

namespace {

std::size_t align_up(std::size_t value, std::size_t align) {
  if (align <= 1) {
    return value;
  }
  const auto remainder = value % align;
  return remainder == 0 ? value : value + (align - remainder);
}

}  // namespace

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
  const c4c::FunctionNameId function_name_id =
      prepared_names == nullptr ? c4c::kInvalidFunctionName
                                : c4c::backend::prepare::resolve_prepared_function_name_id(
                                      *prepared_names, function.name)
                                      .value_or(c4c::kInvalidFunctionName);
  for (const auto& slot : function.local_slots) {
    if (slot.type != c4c::backend::bir::TypeKind::I8 &&
        slot.type != c4c::backend::bir::TypeKind::I16 &&
        slot.type != c4c::backend::bir::TypeKind::I32 &&
        slot.type != c4c::backend::bir::TypeKind::I64 &&
        slot.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto slot_size = slot.size_bytes != 0
                               ? slot.size_bytes
                               : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                  : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                  : 1u);
    const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
    if (slot_size == 0 || slot_size > 8 || slot_align > 16) {
      return std::nullopt;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.offsets.emplace(slot.name, next_offset);
    next_offset += slot_size;
    max_align = std::max(max_align, slot_align);
  }
  if (stack_layout != nullptr) {
    for (const auto& frame_slot : stack_layout->frame_slots) {
      if (frame_slot.function_name != function_name_id) {
        continue;
      }
      layout.frame_slot_offsets.emplace(frame_slot.slot_id, frame_slot.offset_bytes);
    }
  }
  if (function_addressing != nullptr) {
    if (function_name_id == c4c::kInvalidFunctionName ||
        function_addressing->function_name != function_name_id) {
      return std::nullopt;
    }
    const auto required_frame_alignment =
        std::max<std::size_t>(16, function_addressing->frame_alignment_bytes);
    layout.frame_alignment = required_frame_alignment;
    layout.frame_size = align_up(function_addressing->frame_size_bytes, required_frame_alignment);
    return layout;
  }
  layout.frame_alignment = max_align;
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

namespace {

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_function_memory_access(
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  if (function_addressing == nullptr || block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_memory_access(
      *function_addressing, block_label, inst_index);
}

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_frame_memory_access(
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  const auto* access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_symbol_memory_access(
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  const auto* access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      c4c::backend::prepare::prepared_link_name(*prepared_names, *access->address.symbol_name).empty()) {
    return nullptr;
  }
  return access;
}

struct PreparedI32ValueSelection {
  std::optional<std::int32_t> immediate;
  std::optional<std::string> operand;
  bool in_eax = false;
};

std::optional<std::string_view> prepared_scalar_memory_operand_size_name(
    c4c::backend::bir::TypeKind type);
std::optional<std::string> render_prepared_symbol_memory_operand_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name);
const c4c::backend::bir::Block* resolve_authoritative_prepared_branch_target(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Block& block,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);
std::optional<std::string> render_prepared_scalar_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name);
std::optional<std::size_t> find_prepared_value_home_frame_offset(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name);
bool has_authoritative_prepared_control_flow_block(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::BlockLabelId block_label_id);

struct PreparedSameModuleScalarMemorySelection {
  const c4c::backend::bir::Global* global = nullptr;
  std::string memory_operand;
};

template <class ValidateGlobal>
std::optional<PreparedSameModuleScalarMemorySelection>
select_prepared_same_module_scalar_memory_for_inst_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    ValidateGlobal&& validate_global) {
  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }
  const auto* prepared_access =
      find_prepared_symbol_memory_access(&prepared_names, function_addressing, block_label, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  auto memory_operand = render_prepared_symbol_memory_operand_if_supported(
      prepared_names, prepared_access->address, *size_name, render_asm_symbol_name);
  if (!memory_operand.has_value()) {
    return std::nullopt;
  }
  const std::string_view resolved_global_name =
      c4c::backend::prepare::prepared_link_name(
          prepared_names, *prepared_access->address.symbol_name);
  const auto* global = find_same_module_global(resolved_global_name);
  if (global == nullptr || !validate_global(*global, prepared_access->address.byte_offset)) {
    return std::nullopt;
  }
  return PreparedSameModuleScalarMemorySelection{
      .global = global,
      .memory_operand = std::move(*memory_operand),
  };
}

template <class ResolveNamedOperand>
std::optional<PreparedI32ValueSelection> select_prepared_i32_value_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return PreparedI32ValueSelection{
        .immediate = static_cast<std::int32_t>(value.immediate),
        .operand = std::nullopt,
        .in_eax = false,
    };
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  if (current_i32_name.has_value() && value.name == *current_i32_name) {
    return PreparedI32ValueSelection{
        .immediate = std::nullopt,
        .operand = std::nullopt,
        .in_eax = true,
    };
  }
  const auto operand = resolve_named_operand(value.name);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  return PreparedI32ValueSelection{
      .immediate = std::nullopt,
      .operand = std::move(*operand),
      .in_eax = false,
  };
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_operand_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto selection = select_prepared_i32_value_if_supported(
      value, current_i32_name, resolve_named_operand);
  if (!selection.has_value()) {
    return std::nullopt;
  }
  if (selection->immediate.has_value()) {
    return std::to_string(*selection->immediate);
  }
  if (selection->in_eax) {
    return std::string("eax");
  }
  if (!selection->operand.has_value()) {
    return std::nullopt;
  }
  return *selection->operand;
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_move_to_register_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    std::string_view target_register,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto operand = render_prepared_i32_operand_if_supported(
      value, current_i32_name, resolve_named_operand);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  if (*operand == target_register) {
    return std::string{};
  }
  return "    mov " + std::string(target_register) + ", " + *operand + "\n";
}

std::optional<std::string> select_prepared_previous_i32_operand_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& previous_i32_name) {
  if (previous_i32_name.has_value() && value_name == *previous_i32_name) {
    return std::string("ecx");
  }
  return std::nullopt;
}

}  // namespace

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  const bool lhs_is_value_rhs_is_imm =
      lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_value_lhs_is_imm =
      rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
    return std::nullopt;
  }
  return PreparedI32NamedImmediateCompareSelection{
      .named_value = lhs_is_value_rhs_is_imm ? &lhs : &rhs,
      .immediate = lhs_is_value_rhs_is_imm ? rhs.immediate : lhs.immediate,
  };
}

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_value_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string_view compared_value_name) {
  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_if_supported(lhs, rhs);
  if (!selected_compare.has_value() ||
      selected_compare->named_value->name != compared_value_name) {
    return std::nullopt;
  }
  return selected_compare;
}

bool prepared_i32_compare_value_name_matches_any_candidate(
    std::string_view compared_value_name,
    const std::vector<std::string_view>& candidate_compare_value_names) {
  return std::find(candidate_compare_value_names.begin(),
                   candidate_compare_value_names.end(),
                   compared_value_name) != candidate_compare_value_names.end();
}

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_candidates_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    const std::vector<std::string_view>& candidate_compare_value_names) {
  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_if_supported(lhs, rhs);
  if (!selected_compare.has_value() ||
      !prepared_i32_compare_value_name_matches_any_candidate(
          selected_compare->named_value->name, candidate_compare_value_names)) {
    return std::nullopt;
  }
  return selected_compare;
}

std::string render_prepared_i32_eax_immediate_compare_setup(std::int64_t compare_immediate) {
  if (compare_immediate == 0) {
    return std::string("    test eax, eax\n");
  }
  return "    cmp eax, " +
         std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
}

namespace {

std::optional<std::string> select_prepared_materialized_i32_compare_setup_if_supported(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<std::string_view>& current_i32_name) {
  if (current_i32_name.has_value()) {
    const auto selected_compare =
        select_prepared_i32_named_immediate_compare_for_value_if_supported(
            compare.lhs, compare.rhs, *current_i32_name);
    if (selected_compare.has_value()) {
      return render_prepared_i32_eax_immediate_compare_setup(selected_compare->immediate);
    }
  }
  if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
      compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.rhs.type == c4c::backend::bir::TypeKind::I32) {
    return "    mov eax, " +
           std::to_string(static_cast<std::int32_t>(compare.lhs.immediate)) +
           "\n    cmp eax, " +
           std::to_string(static_cast<std::int32_t>(compare.rhs.immediate)) + "\n";
  }
  return std::nullopt;
}

struct PreparedI32ImmediateBranchPlan {
  std::int64_t compare_immediate = 0;
  c4c::backend::bir::BinaryOpcode compare_opcode = c4c::backend::bir::BinaryOpcode::Eq;
  std::string true_label;
  std::string false_label;
};

struct PreparedI32ComparedImmediateBranchPlan {
  c4c::backend::bir::Value compared_value;
  PreparedI32ImmediateBranchPlan branch_plan;
};

struct NamedLocalI32Expr {
  enum class Kind { LocalLoad, Add, Sub };
  Kind kind = Kind::LocalLoad;
  std::string operand;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

struct PreparedLocalI32GuardExpressionSurface {
  std::string setup_asm;
  std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
  std::vector<std::string_view> candidate_compare_value_names;
};

std::optional<std::string> render_prepared_frame_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name);

std::optional<PreparedLocalI32GuardExpressionSurface>
select_prepared_local_i32_guard_expression_surface_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    const PreparedModuleLocalSlotLayout& layout,
    c4c::BlockLabelId entry_label_id) {
  if (context.entry == nullptr) {
    return std::nullopt;
  }

  PreparedLocalI32GuardExpressionSurface selection;
  const auto& entry = *context.entry;
  for (std::size_t index = 0; index + 1 < entry.insts.size(); ++index) {
    const auto& inst = entry.insts[index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, index);
      if (store->byte_offset != 0 || store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      selection.setup_asm += "    mov " + *memory + ", " +
                             std::to_string(static_cast<std::int32_t>(store->value.immediate)) +
                             "\n";
      continue;
    }

    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, index);
      if (load->byte_offset != 0 || load->result.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      selection.named_i32_exprs[load->result.name] = NamedLocalI32Expr{
          .kind = NamedLocalI32Expr::Kind::LocalLoad,
          .operand = *memory,
      };
      selection.candidate_compare_value_names.push_back(load->result.name);
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        (binary->opcode != c4c::backend::bir::BinaryOpcode::Add &&
         binary->opcode != c4c::backend::bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    selection.named_i32_exprs[binary->result.name] = NamedLocalI32Expr{
        .kind = binary->opcode == c4c::backend::bir::BinaryOpcode::Add
                    ? NamedLocalI32Expr::Kind::Add
                    : NamedLocalI32Expr::Kind::Sub,
        .lhs = binary->lhs,
        .rhs = binary->rhs,
    };
    selection.candidate_compare_value_names.push_back(binary->result.name);
  }

  return selection;
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_raw_i32_compared_immediate_branch_plan_if_supported(
    const c4c::backend::bir::Block& entry,
    const std::vector<std::string_view>& compared_value_names) {
  const auto* compare = entry.insts.empty()
                            ? nullptr
                            : std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.back());
  if (compare == nullptr ||
      (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
       compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
      compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
      (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
       compare->result.type != c4c::backend::bir::TypeKind::I32) ||
      entry.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
      entry.terminator.condition.name != compare->result.name) {
    return std::nullopt;
  }

  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_for_candidates_if_supported(
          compare->lhs, compare->rhs, compared_value_names);
  if (!selected_compare.has_value()) {
    return std::nullopt;
  }

  return PreparedI32ComparedImmediateBranchPlan{
      .compared_value = *selected_compare->named_value,
      .branch_plan =
          PreparedI32ImmediateBranchPlan{
              .compare_immediate = selected_compare->immediate,
              .compare_opcode = compare->opcode,
              .true_label = entry.terminator.true_label,
              .false_label = entry.terminator.false_label,
          },
  };
}

std::optional<PreparedI32ImmediateBranchPlan>
select_raw_i32_immediate_branch_plan_if_supported(
    const c4c::backend::bir::Block& entry,
    std::string_view compared_value_name) {
  const auto raw_branch_plan = select_raw_i32_compared_immediate_branch_plan_if_supported(
      entry, {compared_value_name});
  if (!raw_branch_plan.has_value()) {
    return std::nullopt;
  }
  return raw_branch_plan->branch_plan;
}

std::optional<PreparedI32ImmediateBranchPlan>
select_prepared_i32_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition& prepared_branch_condition,
    std::string_view compared_value_name) {
  if (!context.prepared_names || !prepared_branch_condition.predicate.has_value() ||
      !prepared_branch_condition.compare_type.has_value() ||
      !prepared_branch_condition.lhs.has_value() || !prepared_branch_condition.rhs.has_value() ||
      *prepared_branch_condition.compare_type != c4c::backend::bir::TypeKind::I32 ||
      prepared_branch_condition.condition_value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_for_value_if_supported(
          *prepared_branch_condition.lhs,
          *prepared_branch_condition.rhs,
          compared_value_name);
  if (!selected_compare.has_value()) {
    return std::nullopt;
  }

  const auto target_labels = c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
      *context.prepared_names,
      context.function_control_flow,
      entry_label_id,
      entry,
      prepared_branch_condition);
  if (!target_labels.has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
  }

  return PreparedI32ImmediateBranchPlan{
      .compare_immediate = selected_compare->immediate,
      .compare_opcode = *prepared_branch_condition.predicate,
      .true_label = std::string(c4c::backend::prepare::prepared_block_label(
          *context.prepared_names, target_labels->true_label)),
      .false_label = std::string(c4c::backend::prepare::prepared_block_label(
          *context.prepared_names, target_labels->false_label)),
  };
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_prepared_i32_compared_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition& prepared_branch_condition,
    const std::vector<std::string_view>& compared_value_names) {
  const auto* matched_prepared_immediate_branch =
      static_cast<const c4c::backend::prepare::PreparedBranchCondition*>(nullptr);
  const auto* matched_prepared_compared_value = static_cast<const c4c::backend::bir::Value*>(nullptr);
  for (const auto value_name : compared_value_names) {
    const auto value_name_id =
        c4c::backend::prepare::resolve_prepared_value_name_id(*context.prepared_names, value_name);
    if (!value_name_id.has_value()) {
      continue;
    }
    const auto* candidate = c4c::backend::prepare::find_prepared_i32_immediate_branch_condition(
        *context.prepared_names, *context.function_control_flow, entry_label_id, *value_name_id);
    if (candidate == nullptr) {
      continue;
    }

    const auto selected_compare =
        select_prepared_i32_named_immediate_compare_for_value_if_supported(
            *candidate->lhs, *candidate->rhs, value_name);
    if (!selected_compare.has_value()) {
      return std::nullopt;
    }
    const auto* candidate_value = selected_compare->named_value;
    if (matched_prepared_immediate_branch != nullptr &&
        candidate_value->name != matched_prepared_compared_value->name) {
      return std::nullopt;
    }
    matched_prepared_immediate_branch = candidate;
    matched_prepared_compared_value = candidate_value;
  }

  if (matched_prepared_immediate_branch == nullptr || matched_prepared_compared_value == nullptr) {
    return std::nullopt;
  }

  const auto prepared_branch_plan = select_prepared_i32_immediate_branch_plan_if_supported(
      context,
      entry_label_id,
      entry,
      prepared_branch_condition,
      matched_prepared_compared_value->name);
  if (!prepared_branch_plan.has_value()) {
    return std::nullopt;
  }

  return PreparedI32ComparedImmediateBranchPlan{
      .compared_value = *matched_prepared_compared_value,
      .branch_plan = *prepared_branch_plan,
  };
}

std::optional<PreparedI32ImmediateBranchPlan>
select_prepared_or_raw_i32_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition* prepared_branch_condition,
    std::string_view compared_value_name) {
  if (prepared_branch_condition != nullptr) {
    return select_prepared_i32_immediate_branch_plan_if_supported(
        context, entry_label_id, entry, *prepared_branch_condition, compared_value_name);
  }

  const auto raw_branch_plan =
      select_raw_i32_immediate_branch_plan_if_supported(entry, compared_value_name);
  if (!raw_branch_plan.has_value()) {
    return std::nullopt;
  }
  return raw_branch_plan;
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_prepared_or_raw_i32_compared_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition* prepared_branch_condition,
    const std::vector<std::string_view>& compared_value_names) {
  if (prepared_branch_condition != nullptr) {
    return select_prepared_i32_compared_immediate_branch_plan_if_supported(
        context, entry_label_id, entry, *prepared_branch_condition, compared_value_names);
  }
  return select_raw_i32_compared_immediate_branch_plan_if_supported(entry, compared_value_names);
}

std::string render_prepared_i32_compare_and_branch(c4c::backend::bir::BinaryOpcode compare_opcode,
                                                   std::int64_t compare_immediate,
                                                   std::string_view function_name,
                                                   std::string_view false_label) {
  std::string rendered =
      render_prepared_i32_eax_immediate_compare_setup(compare_immediate);
  rendered += "    ";
  rendered += compare_opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je";
  rendered += " .L";
  rendered += function_name;
  rendered += "_";
  rendered += false_label;
  rendered += "\n";
  return rendered;
}

struct PreparedI32EaxImmediateBinarySelection {
  std::int32_t immediate = 0;
};

std::optional<PreparedI32EaxImmediateBinarySelection>
select_prepared_i32_eax_immediate_binary_if_supported(
    const c4c::backend::bir::BinaryInst& binary,
    const std::optional<std::string_view>& current_i32_name) {
  if (!current_i32_name.has_value() ||
      (binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub) ||
      binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const bool lhs_is_current_rhs_is_imm =
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.lhs.name == *current_i32_name &&
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_current_lhs_is_imm =
      binary.opcode == c4c::backend::bir::BinaryOpcode::Add &&
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.rhs.name == *current_i32_name &&
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_current_rhs_is_imm && !rhs_is_current_lhs_is_imm) {
    return std::nullopt;
  }
  return PreparedI32EaxImmediateBinarySelection{
      .immediate = static_cast<std::int32_t>(
          lhs_is_current_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate),
  };
}

std::optional<std::string> render_prepared_i32_binary_inst_if_supported(
    const c4c::backend::bir::BinaryInst& binary,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  const auto selected_binary =
      select_prepared_i32_eax_immediate_binary_if_supported(binary, *current_i32_name);
  if (selected_binary.has_value()) {
    std::string rendered_binary = "    mov ecx, eax\n";
    rendered_binary += std::string("    ") +
                       (binary.opcode == c4c::backend::bir::BinaryOpcode::Add ? "add" : "sub") +
                       " eax, " + std::to_string(selected_binary->immediate) + "\n";
    *current_materialized_compare = std::nullopt;
    *previous_i32_name = *current_i32_name;
    *current_i32_name = binary.result.name;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return rendered_binary;
  }

  if ((binary.opcode != c4c::backend::bir::BinaryOpcode::Eq &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
      binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary.result.type != c4c::backend::bir::TypeKind::I1) {
    return std::nullopt;
  }

  const auto compare_setup =
      select_prepared_materialized_i32_compare_setup_if_supported(binary, *current_i32_name);
  if (!compare_setup.has_value()) {
    return std::nullopt;
  }
  *current_materialized_compare = MaterializedI32Compare{
      .i1_name = binary.result.name,
      .opcode = binary.opcode,
      .compare_setup = *compare_setup,
  };
  *current_i32_name = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return std::string{};
}

std::optional<std::string> render_prepared_cast_inst_if_supported(
    const c4c::backend::bir::CastInst& cast,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I1 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32 &&
      cast.operand.kind == c4c::backend::bir::Value::Kind::Named &&
      current_materialized_compare->has_value() &&
      (*current_materialized_compare)->i1_name == cast.operand.name) {
    (*current_materialized_compare)->i32_name = cast.result.name;
    *current_i32_name = cast.result.name;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return std::string{};
  }

  if (cast.opcode != c4c::backend::bir::CastOpcode::SExt ||
      cast.operand.type != c4c::backend::bir::TypeKind::I8 ||
      cast.result.type != c4c::backend::bir::TypeKind::I32 ||
      cast.operand.kind != c4c::backend::bir::Value::Kind::Named ||
      !current_i8_name->has_value() || **current_i8_name != cast.operand.name) {
    return std::nullopt;
  }

  *current_materialized_compare = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_i32_name = cast.result.name;
  *previous_i32_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return std::string{};
}

std::optional<std::string> render_prepared_block_return_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    const c4c::backend::bir::Value& returned,
    const std::optional<std::string_view>& current_i32_name) {
  if (block_context.local_layout == nullptr) {
    return std::nullopt;
  }
  const auto selected_return = select_prepared_i32_value_if_supported(
      returned,
      current_i32_name,
      [](std::string_view) -> std::optional<std::string> { return std::nullopt; });
  if (!selected_return.has_value()) {
    return std::nullopt;
  }
  if (selected_return->immediate.has_value()) {
    body += "    mov eax, " + std::to_string(*selected_return->immediate) + "\n";
    if (block_context.local_layout->frame_size != 0) {
      body += "    add rsp, " + std::to_string(block_context.local_layout->frame_size) + "\n";
    }
    body += "    ret\n";
    return body;
  }
  if (!selected_return->in_eax) {
    return std::nullopt;
  }
  if (block_context.local_layout->frame_size != 0) {
    body += "    add rsp, " + std::to_string(block_context.local_layout->frame_size) + "\n";
  }
  body += "    ret\n";
  return body;
}

std::optional<std::string> render_prepared_block_plain_branch_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::size_t compare_index,
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>& continuation,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  const auto& block = *block_context.block;
  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
      compare_index != block.insts.size()) {
    return std::nullopt;
  }
  const auto* target = resolve_authoritative_prepared_branch_target(
      function_context.function_control_flow,
      function_context.prepared_names,
      block_context.block_label_id,
      block,
      find_block);
  if (target == nullptr) {
    target = find_block(block.terminator.target_label);
    if (target == nullptr || target == &block) {
      return std::nullopt;
    }
  }
  const auto rendered_target = render_block(*target, continuation);
  if (!rendered_target.has_value()) {
    return std::nullopt;
  }
  return body + *rendered_target;
}

std::optional<std::string> render_prepared_block_cond_branch_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.function == nullptr) {
    return std::nullopt;
  }
  const auto& function = *function_context.function;
  const auto& block = *block_context.block;
  const auto* function_control_flow = function_context.function_control_flow;
  const auto* prepared_names = function_context.prepared_names;
  const auto block_label_id = block_context.block_label_id;

  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      compare_index != block.insts.size() - 1) {
    return std::nullopt;
  }

  const auto try_render_short_circuit_plan = [&]() -> std::optional<std::string> {
    if (function_control_flow == nullptr || prepared_names == nullptr) {
      return std::nullopt;
    }
    const auto resolved_block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, block.label);
    if (!resolved_block_label_id.has_value()) {
      return std::nullopt;
    }

    const auto join_context =
        c4c::backend::x86::find_prepared_short_circuit_join_context_if_supported(
            *prepared_names, *function_control_flow, function, *resolved_block_label_id);
    if (!join_context.has_value()) {
      return std::nullopt;
    }

    const auto short_circuit_render_plan =
        c4c::backend::x86::build_prepared_short_circuit_entry_render_plan(
            *prepared_names,
            function_control_flow,
            function,
            block,
            *join_context,
            compare_index,
            current_materialized_compare,
            current_i32_name,
            [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan)
                -> std::optional<ShortCircuitPlan> {
              return c4c::backend::x86::build_prepared_short_circuit_plan(
                  *prepared_names, prepared_plan, find_block);
            });
    if (!short_circuit_render_plan.has_value()) {
      return std::nullopt;
    }
    return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
        function.name,
        body,
        *short_circuit_render_plan,
        prepared_names,
        find_block,
        render_block);
  };
  if (const auto rendered_short_circuit = try_render_short_circuit_plan();
      rendered_short_circuit.has_value()) {
    return rendered_short_circuit;
  }
  if (has_authoritative_prepared_control_flow_block(function_control_flow, block_label_id) &&
      c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                            block_label_id) == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
  }
  if (function_control_flow != nullptr &&
      prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *prepared_names, *function_control_flow, block_label_id)
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  if (prepared_names == nullptr) {
    return std::nullopt;
  }
  const auto plain_cond_render_plan =
      c4c::backend::x86::build_prepared_plain_cond_entry_render_plan(
          *prepared_names,
          function_control_flow,
          block,
          compare_index,
          current_materialized_compare,
          current_i32_name,
          find_block);
  if (!plain_cond_render_plan.has_value()) {
    return std::nullopt;
  }
  return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
      function.name,
      body,
      *plain_cond_render_plan,
      nullptr,
      find_block,
      render_block);
}

std::optional<std::string> render_prepared_scalar_load_inst_if_supported(
    const c4c::backend::bir::Inst& inst,
    const PreparedModuleLocalSlotLayout& layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::size_t inst_index,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&, c4c::backend::bir::TypeKind,
                             std::int64_t)>& same_module_global_supports_scalar_load,
    std::unordered_set<std::string_view>* same_module_global_names,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (same_module_global_names == nullptr || current_i32_name == nullptr ||
      previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  auto finish_rendered_load =
      [&](const c4c::backend::bir::Value& result,
          std::optional<std::string> rendered_load) -> std::optional<std::string> {
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    *current_materialized_compare = std::nullopt;
    if (result.type == c4c::backend::bir::TypeKind::Ptr) {
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      *current_ptr_name = result.name;
      return rendered_load;
    }
    *current_ptr_name = std::nullopt;
    if (result.type == c4c::backend::bir::TypeKind::I8) {
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = result.name;
      return rendered_load;
    }
    if (result.type == c4c::backend::bir::TypeKind::I32) {
      *current_i32_name = result.name;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      return rendered_load;
    }
    return std::nullopt;
  };

  if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
    const auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
        layout,
        prepared_names,
        function_addressing,
        block_label_id,
        inst_index,
        load->result.type,
        &render_asm_symbol_name);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    return finish_rendered_load(
        load->result,
        render_prepared_scalar_load_from_memory_if_supported(load->result.type, *memory));
  }

  const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
  if (load == nullptr ||
      (load->result.type != c4c::backend::bir::TypeKind::I32 &&
       load->result.type != c4c::backend::bir::TypeKind::I8 &&
       load->result.type != c4c::backend::bir::TypeKind::Ptr) ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto selected_global_memory =
      select_prepared_same_module_scalar_memory_for_inst_if_supported(
          *prepared_names,
          function_addressing,
          block_label_id,
          inst_index,
          load->result.type,
          render_asm_symbol_name,
          find_same_module_global,
          [&](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
            return same_module_global_supports_scalar_load(global, load->result.type, byte_offset);
          });
  if (!selected_global_memory.has_value()) {
    return std::nullopt;
  }
  same_module_global_names->insert(selected_global_memory->global->name);
  return finish_rendered_load(
      load->result,
      render_prepared_scalar_load_from_memory_if_supported(
          load->result.type, selected_global_memory->memory_operand));
}

std::optional<std::string> render_prepared_scalar_store_inst_if_supported(
    const c4c::backend::bir::Inst& inst,
    const PreparedModuleLocalSlotLayout& layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::size_t inst_index,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    std::unordered_set<std::string_view>* same_module_global_names,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (same_module_global_names == nullptr || current_i32_name == nullptr ||
      previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  auto resolve_previous_i32_operand =
      [&](std::string_view value_name) -> std::optional<std::string> {
    return select_prepared_previous_i32_operand_if_supported(value_name, *previous_i32_name);
  };

  auto render_current_i32_operand =
      [&](const c4c::backend::bir::Value& value,
          const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
    return render_prepared_i32_operand_if_supported(value, current_name, resolve_previous_i32_operand);
  };
  auto render_value_home_stack_address =
      [&](std::string_view value_name) -> std::optional<std::string> {
    const auto frame_offset = find_prepared_value_home_frame_offset(
        layout, prepared_names, function_locations, value_name);
    if (!frame_offset.has_value()) {
      return std::nullopt;
    }
    if (*frame_offset == 0) {
      return std::string("[rsp]");
    }
    return "[rsp + " + std::to_string(*frame_offset) + "]";
  };

  if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
    if (store->address.has_value() ||
        (store->value.type != c4c::backend::bir::TypeKind::I32 &&
         store->value.type != c4c::backend::bir::TypeKind::I8)) {
      return std::nullopt;
    }
    const auto store_type = store->value.type;
    const auto selected_global_memory =
        select_prepared_same_module_scalar_memory_for_inst_if_supported(
            *prepared_names,
            function_addressing,
            block_label_id,
            inst_index,
            store_type,
            render_asm_symbol_name,
            find_same_module_global,
            [&](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
              if (store_type == c4c::backend::bir::TypeKind::I32) {
                return global.type == store_type;
              }
              return byte_offset == 0 && global.type == store_type;
            });
    if (!selected_global_memory.has_value()) {
      return std::nullopt;
    }
    same_module_global_names->insert(selected_global_memory->global->name);
    *current_materialized_compare = std::nullopt;
    if (store_type == c4c::backend::bir::TypeKind::I32) {
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
      }
      return render_prepared_i32_store_to_memory_if_supported(
          store->value,
          *current_i32_name,
          selected_global_memory->memory_operand,
          render_current_i32_operand);
    }
    const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
        store->value,
        *current_i8_name,
        selected_global_memory->memory_operand,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
          if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            return std::to_string(static_cast<std::int8_t>(value.immediate));
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_name.has_value() || *current_name != value.name) {
            return std::nullopt;
          }
          return "al";
        });
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    if (rendered_store.has_value() &&
        store->value.kind == c4c::backend::bir::Value::Kind::Named) {
      *current_i8_name = store->value.name;
    } else {
      *current_i8_name = std::nullopt;
    }
    return rendered_store;
  }

  const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
  if (store == nullptr || store->byte_offset != 0) {
    return std::nullopt;
  }
  const auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
      layout,
      prepared_names,
      function_addressing,
      block_label_id,
      inst_index,
      store->value.type,
      &render_asm_symbol_name);
  if (!memory.has_value()) {
    return std::nullopt;
  }
  *current_materialized_compare = std::nullopt;
  if (store->value.type == c4c::backend::bir::TypeKind::I32) {
    const auto selected_store_value = select_prepared_i32_value_if_supported(
        store->value, *current_i32_name, resolve_previous_i32_operand);
    if (selected_store_value.has_value()) {
      if (selected_store_value->immediate.has_value()) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      } else if (selected_store_value->in_eax) {
        *current_i32_name = store->value.name;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      } else {
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      }
      return render_prepared_i32_store_to_memory_if_supported(
          store->value, *current_i32_name, *memory, render_current_i32_operand);
    }
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
      store->value.type == c4c::backend::bir::TypeKind::I8) {
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return render_prepared_i8_store_to_memory_if_supported(
        store->value,
        std::nullopt,
        *memory,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>&) -> std::optional<std::string> {
          if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
            return std::nullopt;
          }
          return std::to_string(static_cast<std::int8_t>(value.immediate));
        });
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
      store->value.type == c4c::backend::bir::TypeKind::I8) {
    const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
        store->value,
        *current_i8_name,
        *memory,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
          if (value.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_name.has_value() || *current_name != value.name) {
            return std::nullopt;
          }
          return "al";
        });
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    if (rendered_store.has_value()) {
      *current_i8_name = store->value.name;
    } else {
      *current_i8_name = std::nullopt;
    }
    return rendered_store;
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
      store->value.type == c4c::backend::bir::TypeKind::Ptr) {
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    const auto rendered_store = render_prepared_ptr_store_to_memory_if_supported(
        store->value,
        *current_ptr_name,
        *memory,
        render_value_home_stack_address);
    *current_ptr_name = std::nullopt;
    if (!rendered_store.has_value()) {
      return std::string{};
    }
    return rendered_store;
  }
  return std::nullopt;
}

std::optional<std::string> select_prepared_i32_call_argument_move_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::string_view abi_register,
    const std::optional<std::string_view>& current_i32_name) {
  if (arg_type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return render_prepared_i32_move_to_register_if_supported(
      arg,
      current_i32_name,
      abi_register,
      [](std::string_view) -> std::optional<std::string> { return std::nullopt; });
}

void finalize_prepared_same_module_helper_call_state(
    const c4c::backend::bir::CallInst& call,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  *current_materialized_compare = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  *current_i32_name =
      call.result.has_value() && call.result->type == c4c::backend::bir::TypeKind::I32
          ? std::optional<std::string_view>(call.result->name)
          : std::nullopt;
}

std::optional<std::string> render_prepared_bounded_same_module_helper_call_if_supported(
    const c4c::backend::bir::Inst& inst,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_names,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
      call->is_variadic || call->args.size() != call->arg_types.size() || call->args.size() > 6 ||
      bounded_same_module_helper_names.find(call->callee) == bounded_same_module_helper_names.end()) {
    return std::nullopt;
  }

  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

  std::string rendered_call;
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto selected_arg_move = select_prepared_i32_call_argument_move_if_supported(
        call->args[arg_index], call->arg_types[arg_index], kArgRegs32[arg_index], *current_i32_name);
    if (!selected_arg_move.has_value()) {
      return std::nullopt;
    }
    rendered_call += *selected_arg_move;
  }
  rendered_call += "    xor eax, eax\n";
  rendered_call += "    call ";
  rendered_call += render_asm_symbol_name(call->callee);
  rendered_call += "\n";

  finalize_prepared_same_module_helper_call_state(
      *call,
      current_i32_name,
      previous_i32_name,
      current_i8_name,
      current_ptr_name,
      current_materialized_compare);
  return rendered_call;
}

std::optional<std::string> render_prepared_block_same_module_helper_call_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr) {
    return std::nullopt;
  }
  static const std::unordered_set<std::string_view> kEmptyHelperNames;
  const auto& function_context = *block_context.function_context;
  const auto& bounded_same_module_helper_names =
      function_context.bounded_same_module_helper_names == nullptr
          ? kEmptyHelperNames
          : *function_context.bounded_same_module_helper_names;
  return render_prepared_bounded_same_module_helper_call_if_supported(
      inst,
      bounded_same_module_helper_names,
      function_context.render_asm_symbol_name,
      current_i32_name,
      previous_i32_name,
      current_i8_name,
      current_ptr_name,
      current_materialized_compare);
}

bool prepared_frame_memory_accesses_match(
    const c4c::backend::prepare::PreparedMemoryAccess* lhs,
    const c4c::backend::prepare::PreparedMemoryAccess* rhs) {
  return lhs != nullptr && rhs != nullptr && lhs->address.frame_slot_id.has_value() &&
         rhs->address.frame_slot_id.has_value() &&
         *lhs->address.frame_slot_id == *rhs->address.frame_slot_id &&
         lhs->address.byte_offset == rhs->address.byte_offset;
}

std::optional<std::string> render_prepared_frame_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name) {
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !address.frame_slot_id.has_value()) {
    return std::nullopt;
  }
  const auto frame_slot_it = local_layout.frame_slot_offsets.find(*address.frame_slot_id);
  if (frame_slot_it == local_layout.frame_slot_offsets.end()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(frame_slot_it->second) + address.byte_offset;
  if (signed_byte_offset < 0) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset),
                                              size_name);
}

std::optional<std::string> render_prepared_symbol_memory_operand_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !address.symbol_name.has_value()) {
    return std::nullopt;
  }
  const std::string_view symbol_name =
      c4c::backend::prepare::prepared_link_name(prepared_names, *address.symbol_name);
  if (symbol_name.empty()) {
    return std::nullopt;
  }
  std::string memory = std::string(size_name) + " PTR [rip + " +
                       render_asm_symbol_name(symbol_name);
  if (address.byte_offset > 0) {
    memory += " + " + std::to_string(address.byte_offset);
  } else if (address.byte_offset < 0) {
    memory += " - " + std::to_string(-address.byte_offset);
  }
  memory += "]";
  return memory;
}

std::optional<std::string_view> prepared_scalar_memory_operand_size_name(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::Ptr:
      return "QWORD";
    case c4c::backend::bir::TypeKind::I8:
      return "BYTE";
    case c4c::backend::bir::TypeKind::I32:
      return "DWORD";
    default:
      return std::nullopt;
  }
}

std::optional<std::string> render_prepared_scalar_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddress& address,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name) {
  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }
  if (address.base_kind == c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot) {
    return render_prepared_frame_slot_memory_operand_if_supported(local_layout, address, *size_name);
  }
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      prepared_names == nullptr || render_asm_symbol_name == nullptr) {
    return std::nullopt;
  }
  return render_prepared_symbol_memory_operand_if_supported(
      *prepared_names, address, *size_name, *render_asm_symbol_name);
}

std::optional<std::string> render_prepared_scalar_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name) {
  const auto* prepared_access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  return render_prepared_scalar_memory_operand_if_supported(
      local_layout,
      prepared_names,
      prepared_access->address,
      type,
      render_asm_symbol_name);
}

std::optional<std::size_t> find_prepared_value_home_frame_offset(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr || value_name.empty()) {
    return std::nullopt;
  }

  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(*prepared_names, *function_locations, value_name);
  if (home == nullptr || home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !home->slot_id.has_value()) {
    return std::nullopt;
  }
  // Pointer-derived stack addresses must come from canonical prepared frame-slot identity.
  const auto frame_slot_it = local_layout.frame_slot_offsets.find(*home->slot_id);
  if (frame_slot_it == local_layout.frame_slot_offsets.end()) {
    return std::nullopt;
  }
  return frame_slot_it->second;
}

const c4c::backend::prepare::PreparedBranchCondition*
find_required_prepared_guard_branch_condition(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id) {
  if (function_control_flow == nullptr || prepared_names == nullptr ||
      block_label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  if (c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow,
                                                              block_label_id) == nullptr) {
    return nullptr;
  }
  const auto* branch_condition =
      c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow, block_label_id);
  if (branch_condition == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
  }
  return branch_condition;
}

bool has_authoritative_prepared_control_flow_block(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::BlockLabelId block_label_id) {
  if (function_control_flow == nullptr || block_label_id == c4c::kInvalidBlockLabel) {
    return false;
  }
  return c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow,
                                                                 block_label_id) != nullptr;
}

bool has_authoritative_prepared_short_circuit_continuation(
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation) {
  if (!continuation.has_value()) {
    return false;
  }
  return continuation->incoming_label != c4c::kInvalidBlockLabel ||
         continuation->true_label != c4c::kInvalidBlockLabel ||
         continuation->false_label != c4c::kInvalidBlockLabel;
}

const c4c::backend::bir::Block* resolve_authoritative_prepared_branch_target(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Block& source_block,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (function_control_flow == nullptr || prepared_names == nullptr ||
      block_label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const auto* prepared_block =
      c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow, block_label_id);
  if (prepared_block == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }
  if (prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
      prepared_block->branch_target_label == c4c::kInvalidBlockLabel) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  const auto* target = find_block(c4c::backend::prepare::prepared_block_label(
      *prepared_names, prepared_block->branch_target_label));
  if (target == nullptr || target == &source_block) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }
  return target;
}

}  // namespace

std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string render_prepared_stack_address_expr(std::size_t byte_offset) {
  if (byte_offset == 0) {
    return "[rsp]";
  }
  return "[rsp + " + std::to_string(byte_offset) + "]";
}

std::optional<std::string> render_prepared_value_home_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  const auto frame_offset =
      find_prepared_value_home_frame_offset(
          local_layout, prepared_names, function_locations, value_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset);
}

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name) {
  const auto slot_it = local_layout.offsets.find(slot_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(slot_it->second + stack_byte_bias, size_name);
}

std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& module = *context.module;
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto* function_control_flow = context.function_control_flow;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  static const std::unordered_set<std::string_view> kEmptyHelperNames;
  const auto& bounded_same_module_helper_names =
      context.bounded_same_module_helper_names == nullptr
          ? kEmptyHelperNames
          : *context.bounded_same_module_helper_names;
  const auto& bounded_same_module_helper_global_names =
      context.bounded_same_module_helper_global_names == nullptr
          ? kEmptyHelperNames
          : *context.bounded_same_module_helper_global_names;
  const auto& find_block = context.find_block;
  const auto& find_same_module_global = context.find_same_module_global;
  const auto& same_module_global_supports_scalar_load =
      context.same_module_global_supports_scalar_load;
  const auto& render_asm_symbol_name = context.render_asm_symbol_name;
  const auto& emit_same_module_global_data = context.emit_same_module_global_data;
  const auto& prepend_bounded_same_module_helpers =
      context.prepend_bounded_same_module_helpers;
  if (!function.params.empty() || function.blocks.empty() || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  std::unordered_set<std::string_view> same_module_global_names;

  std::unordered_set<std::string_view> rendered_blocks;
  std::function<std::optional<std::string>(
      const c4c::backend::bir::Block&,
      const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>
      render_block =
          [&](const c4c::backend::bir::Block& block,
              const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
                  continuation) -> std::optional<std::string> {
    const auto block_context = context.make_block_context(block, *layout);
    if (!rendered_blocks.insert(block.label).second &&
        block.terminator.kind != c4c::backend::bir::TerminatorKind::Return) {
      return std::nullopt;
    }
    const c4c::BlockLabelId block_label_id = block_context.block_label_id;

    auto rendered_load_or_store =
        [&](const c4c::backend::bir::Inst& inst,
            std::size_t inst_index,
            std::optional<std::string_view>* current_i32_name,
            std::optional<std::string_view>* previous_i32_name,
            std::optional<std::string_view>* current_i8_name,
            std::optional<std::string_view>* current_ptr_name,
            std::optional<MaterializedI32Compare>* current_materialized_compare)
        -> std::optional<std::string> {
      const auto rendered_load = render_prepared_scalar_load_inst_if_supported(
          inst,
          *layout,
          prepared_names,
          function_addressing,
          block_label_id,
          inst_index,
          render_asm_symbol_name,
          find_same_module_global,
          same_module_global_supports_scalar_load,
          &same_module_global_names,
          current_i32_name,
          previous_i32_name,
          current_i8_name,
          current_ptr_name,
          current_materialized_compare);
      if (rendered_load.has_value()) {
        return rendered_load;
      }
      return render_prepared_scalar_store_inst_if_supported(
          inst,
          *layout,
          prepared_names,
          function_locations,
          function_addressing,
          block_label_id,
          inst_index,
          render_asm_symbol_name,
          find_same_module_global,
          &same_module_global_names,
          current_i32_name,
          previous_i32_name,
          current_i8_name,
          current_ptr_name,
          current_materialized_compare);
    };

    std::string body;
    std::optional<std::string_view> current_i32_name;
    std::optional<std::string_view> previous_i32_name;
    std::optional<std::string_view> current_i8_name;
    std::optional<std::string_view> current_ptr_name;
    std::optional<MaterializedI32Compare> current_materialized_compare;
    std::size_t compare_index = block.insts.size();
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
      if (block.insts.empty()) {
        return std::nullopt;
      }
      compare_index = block.insts.size() - 1;
    } else if (continuation.has_value() &&
               block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch &&
               !block.insts.empty()) {
      const auto* branch_compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
      if (branch_compare != nullptr &&
          (branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
           branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
          branch_compare->operand_type == c4c::backend::bir::TypeKind::I32 &&
          (branch_compare->result.type == c4c::backend::bir::TypeKind::I1 ||
           branch_compare->result.type == c4c::backend::bir::TypeKind::I32)) {
        compare_index = block.insts.size() - 1;
      }
    }

    for (std::size_t index = 0; index < compare_index; ++index) {
      const auto rendered_inst =
          rendered_load_or_store(block.insts[index],
                                 index,
                                 &current_i32_name,
                                 &previous_i32_name,
                                 &current_i8_name,
                                 &current_ptr_name,
                                 &current_materialized_compare);
      if (rendered_inst.has_value()) {
        body += *rendered_inst;
        continue;
      }

      if (const auto rendered_call = render_prepared_block_same_module_helper_call_inst_if_supported(
              block_context,
              block.insts[index],
              &current_i32_name,
              &previous_i32_name,
              &current_i8_name,
              &current_ptr_name,
              &current_materialized_compare);
          rendered_call.has_value()) {
        body += *rendered_call;
        continue;
      }

      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index]);
      if (binary != nullptr) {
        const auto rendered_binary = render_prepared_i32_binary_inst_if_supported(
            *binary,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare);
        if (rendered_binary.has_value()) {
          body += *rendered_binary;
          continue;
        }
      }

      const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&block.insts[index]);
      if (cast != nullptr) {
        const auto rendered_cast = render_prepared_cast_inst_if_supported(
            *cast,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare);
        if (rendered_cast.has_value()) {
          body += *rendered_cast;
          continue;
        }
      }
      return std::nullopt;
    }

    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Return) {
      if (compare_index != block.insts.size() || !block.terminator.value.has_value()) {
        return std::nullopt;
      }
      return render_prepared_block_return_terminator_if_supported(
          block_context, std::move(body), *block.terminator.value, current_i32_name);
    }

    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
      if (continuation.has_value()) {
        if (prepared_names == nullptr) {
          return std::nullopt;
        }
        const auto compare_join_render_plan =
            c4c::backend::x86::build_prepared_compare_join_entry_render_plan(
                *prepared_names,
                function_control_flow,
                function,
                block,
                *continuation,
                compare_index,
                current_materialized_compare,
                current_i32_name,
                [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan)
                    -> std::optional<ShortCircuitPlan> {
                  return c4c::backend::x86::build_prepared_short_circuit_plan(
                      *prepared_names, prepared_plan, find_block);
                });
        if (compare_join_render_plan.has_value()) {
          return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
              function.name,
              body,
              *compare_join_render_plan,
              prepared_names,
              find_block,
              render_block);
        }
        if (has_authoritative_prepared_short_circuit_continuation(continuation)) {
          throw std::invalid_argument(
              "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
        }
      }
      return render_prepared_block_plain_branch_terminator_if_supported(
          block_context, std::move(body), compare_index, continuation, find_block, render_block);
    }

    return render_prepared_block_cond_branch_terminator_if_supported(
        block_context,
        std::move(body),
        compare_index,
        current_materialized_compare,
        current_i32_name,
        find_block,
        render_block);
  };

  auto asm_text = std::string(asm_prefix);
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const auto rendered_entry = render_block(entry, std::nullopt);
  if (!rendered_entry.has_value()) {
    return std::nullopt;
  }
  std::string rendered_same_module_globals;
  for (const auto& global : module.globals) {
    if (same_module_global_names.find(global.name) == same_module_global_names.end() &&
        bounded_same_module_helper_global_names.find(global.name) ==
            bounded_same_module_helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_same_module_globals += *rendered_global_data;
  }
  asm_text += *rendered_entry;
  asm_text += rendered_same_module_globals;
  return prepend_bounded_same_module_helpers(std::move(asm_text));
}

std::optional<std::string> render_prepared_local_i32_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  if (!function.params.empty() || function.blocks.size() != 3 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      context.prepared_arch != c4c::TargetArch::X86_64 || entry.insts.empty()) {
    return std::nullopt;
  }

  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              context.stack_layout,
                                              context.function_addressing,
                                              context.prepared_names,
                                              context.prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
  std::vector<std::string_view> candidate_compare_value_names;
  const auto render_guard_return = [&](std::int32_t returned_imm) -> std::string {
    std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
    if (layout->frame_size != 0) {
      rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    rendered += "    ret\n";
    return rendered;
  };

  std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_operand;
  std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_value;
  render_i32_operand = [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      return std::to_string(static_cast<std::int32_t>(value.immediate));
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    const auto expr_it = named_i32_exprs.find(value.name);
    if (expr_it == named_i32_exprs.end() ||
        expr_it->second.kind != NamedLocalI32Expr::Kind::LocalLoad) {
      return std::nullopt;
    }
    return expr_it->second.operand;
  };
  render_i32_value = [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (const auto operand = render_i32_operand(value); operand.has_value()) {
      return "    mov eax, " + *operand + "\n";
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    const auto expr_it = named_i32_exprs.find(value.name);
    if (expr_it == named_i32_exprs.end()) {
      return std::nullopt;
    }
    const auto& expr = expr_it->second;
    if (expr.kind == NamedLocalI32Expr::Kind::Add) {
      if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
        if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
          return *lhs_render + "    add eax, " + *rhs_operand + "\n";
        }
      }
      if (const auto rhs_render = render_i32_value(expr.rhs); rhs_render.has_value()) {
        if (const auto lhs_operand = render_i32_operand(expr.lhs); lhs_operand.has_value()) {
          return *rhs_render + "    add eax, " + *lhs_operand + "\n";
        }
      }
      return std::nullopt;
    }
    if (expr.kind == NamedLocalI32Expr::Kind::Sub) {
      if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
        if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
          return *lhs_render + "    sub eax, " + *rhs_operand + "\n";
        }
      }
      return std::nullopt;
    }
    return "    mov eax, " + expr.operand + "\n";
  };

  auto asm_text = std::string(context.asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const c4c::BlockLabelId entry_label_id =
      context.prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *context.prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  const auto guard_expression_surface =
      select_prepared_local_i32_guard_expression_surface_if_supported(
          context, *layout, entry_label_id);
  if (!guard_expression_surface.has_value()) {
    return std::nullopt;
  }
  named_i32_exprs = std::move(guard_expression_surface->named_i32_exprs);
  candidate_compare_value_names =
      std::move(guard_expression_surface->candidate_compare_value_names);
  asm_text += guard_expression_surface->setup_asm;

  const auto* prepared_branch_condition =
      find_required_prepared_guard_branch_condition(
          context.function_control_flow, context.prepared_names, entry_label_id);
  if (context.function_control_flow != nullptr && context.prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *context.prepared_names, *context.function_control_flow, entry_label_id)
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  const auto compared_branch_plan =
      select_prepared_or_raw_i32_compared_immediate_branch_plan_if_supported(
          context,
          entry_label_id,
          entry,
          prepared_branch_condition,
          candidate_compare_value_names);
  if (!compared_branch_plan.has_value()) {
    return std::nullopt;
  }

  const auto& compared_value = compared_branch_plan->compared_value;
  const auto compare_immediate = compared_branch_plan->branch_plan.compare_immediate;
  const auto compare_opcode = compared_branch_plan->branch_plan.compare_opcode;
  const auto& true_label = compared_branch_plan->branch_plan.true_label;
  const auto& false_label = compared_branch_plan->branch_plan.false_label;

  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  const auto* true_block = context.find_block(true_label);
  const auto* false_block = context.find_block(false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto compared_render = render_i32_value(compared_value);
  if (!compared_render.has_value()) {
    return std::nullopt;
  }
  asm_text += *compared_render;
  asm_text += render_prepared_i32_compare_and_branch(
      compare_opcode, compare_immediate, function.name, false_block->label);

  const auto render_return_block =
      [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
    const auto& value = *block.terminator.value;
    if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    return render_guard_return(static_cast<std::int32_t>(value.immediate));
  };
  const auto rendered_true = render_return_block(*true_block);
  const auto rendered_false = render_return_block(*false_block);
  if (!rendered_true.has_value() || !rendered_false.has_value()) {
    return std::nullopt;
  }
  asm_text += *rendered_true;
  asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
  asm_text += *rendered_false;
  return asm_text;
}

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  if (!function.params.empty() || function.blocks.size() != 3 ||
      context.prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      entry.insts.size() != 9) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              context.stack_layout,
                                              context.function_addressing,
                                              context.prepared_names,
                                              context.prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const c4c::backend::bir::Block* true_block = context.find_block(entry.terminator.true_label);
  const c4c::backend::bir::Block* false_block = context.find_block(entry.terminator.false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto* store_zero = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[0]);
  const auto* load_initial = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[1]);
  const auto* sext_initial = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[2]);
  const auto* add_one = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[3]);
  const auto* trunc_updated = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[4]);
  const auto* store_updated = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[5]);
  const auto* load_updated = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[6]);
  const auto* sext_updated = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[7]);
  const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[8]);
  if (store_zero == nullptr || load_initial == nullptr || sext_initial == nullptr ||
      add_one == nullptr || trunc_updated == nullptr || store_updated == nullptr ||
      load_updated == nullptr || sext_updated == nullptr || compare == nullptr) {
    return std::nullopt;
  }
  if (store_zero->byte_offset != 0 || load_initial->byte_offset != 0 ||
      store_updated->byte_offset != 0 || load_updated->byte_offset != 0 ||
      store_zero->address.has_value() || load_initial->address.has_value() ||
      store_updated->address.has_value() || load_updated->address.has_value()) {
    return std::nullopt;
  }
  if (store_zero->slot_name != load_initial->slot_name ||
      store_zero->slot_name != store_updated->slot_name ||
      store_zero->slot_name != load_updated->slot_name) {
    return std::nullopt;
  }
  if (store_zero->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_zero->value.type != c4c::backend::bir::TypeKind::I16 ||
      load_initial->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_initial->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_initial->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_initial->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_initial->operand.name != load_initial->result.name ||
      sext_initial->result.type != c4c::backend::bir::TypeKind::I32 ||
      add_one->opcode != c4c::backend::bir::BinaryOpcode::Add ||
      add_one->operand_type != c4c::backend::bir::TypeKind::I32 ||
      add_one->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      add_one->lhs.name != sext_initial->result.name ||
      add_one->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      add_one->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      add_one->rhs.immediate != 1 ||
      add_one->result.type != c4c::backend::bir::TypeKind::I32 ||
      trunc_updated->opcode != c4c::backend::bir::CastOpcode::Trunc ||
      trunc_updated->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      trunc_updated->operand.type != c4c::backend::bir::TypeKind::I32 ||
      trunc_updated->operand.name != add_one->result.name ||
      trunc_updated->result.type != c4c::backend::bir::TypeKind::I16 ||
      store_updated->value.kind != c4c::backend::bir::Value::Kind::Named ||
      store_updated->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_updated->value.name != trunc_updated->result.name ||
      load_updated->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_updated->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_updated->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_updated->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_updated->operand.name != load_updated->result.name ||
      sext_updated->result.type != c4c::backend::bir::TypeKind::I32 ||
      (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
       compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
      compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
      compare->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      compare->lhs.name != sext_updated->result.name ||
      compare->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      compare->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      compare->result.kind != c4c::backend::bir::Value::Kind::Named ||
      compare->result.name != entry.terminator.condition.name) {
    return std::nullopt;
  }

  const c4c::BlockLabelId entry_label_id =
      context.prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *context.prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  std::optional<std::string> short_memory;
  if (const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, 0);
      prepared_access != nullptr) {
    short_memory =
        render_prepared_frame_slot_memory_operand_if_supported(*layout,
                                                               prepared_access->address,
                                                               "WORD");
  }
  if (!short_memory.has_value()) {
    return std::nullopt;
  }
  const auto render_return_block =
      [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
    const auto& value = *block.terminator.value;
    if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    std::string rendered =
        "    mov eax, " + std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
    if (layout->frame_size != 0) {
      rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    rendered += "    ret\n";
    return rendered;
  };
  const auto* prepared_branch_condition =
      find_required_prepared_guard_branch_condition(
          context.function_control_flow, context.prepared_names, entry_label_id);
  if (context.function_control_flow != nullptr && context.prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *context.prepared_names, *context.function_control_flow, entry_label_id)
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  const auto branch_plan = select_prepared_or_raw_i32_immediate_branch_plan_if_supported(
      context, entry_label_id, entry, prepared_branch_condition, sext_updated->result.name);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }
  auto compare_opcode = branch_plan->compare_opcode;
  std::int64_t compare_immediate = branch_plan->compare_immediate;
  std::string true_label = branch_plan->true_label;
  std::string false_label = branch_plan->false_label;
  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  true_block = context.find_block(true_label);
  false_block = context.find_block(false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto rendered_true = render_return_block(*true_block);
  const auto rendered_false = render_return_block(*false_block);
  if (!rendered_true.has_value() || !rendered_false.has_value()) {
    return std::nullopt;
  }

  std::string asm_text = std::string(context.asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    mov " + *short_memory + ", " +
              std::to_string(static_cast<std::int16_t>(store_zero->value.immediate)) + "\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += "    add eax, 1\n";
  asm_text += "    mov " + *short_memory + ", ax\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += render_prepared_i32_compare_and_branch(
      compare_opcode, compare_immediate, function.name, false_block->label);
  asm_text += *rendered_true;
  asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
  asm_text += *rendered_false;
  return asm_text;
}

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  return render_prepared_local_i16_arithmetic_guard_if_supported(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_control_flow = function_control_flow,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .find_block = find_block,
      });
}

namespace {

std::optional<std::string> render_prepared_local_i16_i64_sub_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  if (!function.params.empty() || function.blocks.size() != 1 ||
      prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value() || entry.insts.size() != 10) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const auto* store_short = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[0]);
  const auto* store_long = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[1]);
  const auto* load_long = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[2]);
  const auto* load_short = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[3]);
  const auto* sext_short = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[4]);
  const auto* sub = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[5]);
  const auto* trunc_result = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[6]);
  const auto* store_result = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[7]);
  const auto* load_result = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[8]);
  const auto* sext_result = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[9]);
  if (store_short == nullptr || store_long == nullptr || load_long == nullptr ||
      load_short == nullptr || sext_short == nullptr || sub == nullptr ||
      trunc_result == nullptr || store_result == nullptr || load_result == nullptr ||
      sext_result == nullptr) {
    return std::nullopt;
  }
  if (store_short->byte_offset != 0 || store_long->byte_offset != 0 ||
      load_long->byte_offset != 0 || load_short->byte_offset != 0 ||
      store_result->byte_offset != 0 || load_result->byte_offset != 0 ||
      store_short->address.has_value() || store_long->address.has_value() ||
      load_long->address.has_value() || load_short->address.has_value() ||
      store_result->address.has_value() || load_result->address.has_value()) {
    return std::nullopt;
  }
  const auto& returned = *entry.terminator.value;
  if (store_short->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_short->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_long->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_long->value.type != c4c::backend::bir::TypeKind::I64 ||
      load_long->result.type != c4c::backend::bir::TypeKind::I64 ||
      load_short->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_short->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_short->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_short->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_short->operand.name != load_short->result.name ||
      sext_short->result.type != c4c::backend::bir::TypeKind::I64 ||
      sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      sub->operand_type != c4c::backend::bir::TypeKind::I64 ||
      sub->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->lhs.name != sext_short->result.name ||
      sub->rhs.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->rhs.name != load_long->result.name ||
      sub->rhs.type != c4c::backend::bir::TypeKind::I64 ||
      sub->result.type != c4c::backend::bir::TypeKind::I64 ||
      trunc_result->opcode != c4c::backend::bir::CastOpcode::Trunc ||
      trunc_result->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      trunc_result->operand.type != c4c::backend::bir::TypeKind::I64 ||
      trunc_result->operand.name != sub->result.name ||
      trunc_result->result.type != c4c::backend::bir::TypeKind::I16 ||
      store_result->value.kind != c4c::backend::bir::Value::Kind::Named ||
      store_result->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_result->value.name != trunc_result->result.name ||
      load_result->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_result->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_result->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_result->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_result->operand.name != load_result->result.name ||
      sext_result->result.type != c4c::backend::bir::TypeKind::I32 ||
      returned.kind != c4c::backend::bir::Value::Kind::Named ||
      returned.name != sext_result->result.name) {
    return std::nullopt;
  }

  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  const auto* prepared_store_short =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 0);
  const auto* prepared_store_long =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 1);
  const auto* prepared_load_long =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 2);
  const auto* prepared_load_short =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 3);
  const auto* prepared_store_result =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 7);
  const auto* prepared_load_result =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 8);
  if (!prepared_frame_memory_accesses_match(prepared_store_short, prepared_load_short) ||
      !prepared_frame_memory_accesses_match(prepared_store_short, prepared_store_result) ||
      !prepared_frame_memory_accesses_match(prepared_store_short, prepared_load_result) ||
      !prepared_frame_memory_accesses_match(prepared_store_long, prepared_load_long)) {
    return std::nullopt;
  }

  const auto short_memory = render_prepared_frame_slot_memory_operand_if_supported(
      *layout, prepared_store_short->address, "WORD");
  const auto long_memory = render_prepared_frame_slot_memory_operand_if_supported(
      *layout, prepared_store_long->address, "QWORD");
  if (!short_memory.has_value() || !long_memory.has_value()) {
    return std::nullopt;
  }

  std::string asm_text = std::string(asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    mov " + *short_memory + ", " +
              std::to_string(static_cast<std::int16_t>(store_short->value.immediate)) + "\n";
  asm_text += "    mov " + *long_memory + ", " +
              std::to_string(static_cast<std::int64_t>(store_long->value.immediate)) + "\n";
  asm_text += "    movsx rax, " + *short_memory + "\n";
  asm_text += "    sub rax, " + *long_memory + "\n";
  asm_text += "    mov " + *short_memory + ", ax\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  if (layout->frame_size != 0) {
    asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

}  // namespace

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  return render_prepared_local_i16_i64_sub_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
      });
}

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_local_i16_i64_sub_return_from_context(context);
}

namespace {

std::optional<std::string> render_prepared_constant_folded_single_block_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  if (!function.params.empty() || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      prepared_arch != c4c::TargetArch::X86_64 || stack_layout == nullptr ||
      function_addressing == nullptr || prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto wrap_i32 = [](std::int64_t value) -> std::int32_t {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
  };
  const auto entry_label_id =
      c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, entry.label);
  if (!entry_label_id.has_value()) {
    return std::nullopt;
  }
  const auto layout = build_prepared_module_local_slot_layout(
      function, stack_layout, function_addressing, prepared_names, prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  struct ConstantValue {
    c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
    std::int64_t value = 0;
  };

  std::unordered_map<std::string_view, ConstantValue> named_values;
  std::unordered_map<std::size_t, ConstantValue> local_memory;

  const auto resolve_i32_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::int32_t> {
        if (value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return static_cast<std::int32_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it == named_values.end() ||
            value_it->second.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        return static_cast<std::int32_t>(value_it->second.value);
      };

  const auto resolve_ptr_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::size_t> {
        if (value.type != c4c::backend::bir::TypeKind::Ptr) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          if (value.immediate < 0) {
            return std::nullopt;
          }
          return static_cast<std::size_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it != named_values.end()) {
          if (value_it->second.type != c4c::backend::bir::TypeKind::Ptr ||
              value_it->second.value < 0) {
            return std::nullopt;
          }
          return static_cast<std::size_t>(value_it->second.value);
        }
        return find_prepared_value_home_frame_offset(
            *layout, prepared_names, function_locations, value.name);
      };

  const auto resolve_prepared_memory_address = [&](std::size_t inst_index)
      -> std::optional<std::size_t> {
    const auto* prepared_access =
        find_prepared_function_memory_access(function_addressing, *entry_label_id, inst_index);
    if (prepared_access == nullptr) {
      return std::nullopt;
    }

    std::optional<std::size_t> base_offset;
    switch (prepared_access->address.base_kind) {
      case c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot: {
        if (!prepared_access->address.frame_slot_id.has_value()) {
          return std::nullopt;
        }
        const auto frame_slot_it =
            layout->frame_slot_offsets.find(*prepared_access->address.frame_slot_id);
        if (frame_slot_it == layout->frame_slot_offsets.end()) {
          return std::nullopt;
        }
        base_offset = frame_slot_it->second;
        break;
      }
      case c4c::backend::prepare::PreparedAddressBaseKind::PointerValue: {
        if (!prepared_access->address.pointer_value_name.has_value() ||
            !prepared_access->address.can_use_base_plus_offset) {
          return std::nullopt;
        }
        const auto pointer_name =
            c4c::backend::prepare::prepared_value_name(
                *prepared_names, *prepared_access->address.pointer_value_name);
        if (pointer_name.empty()) {
          return std::nullopt;
        }
        base_offset = resolve_ptr_value(c4c::backend::bir::Value{
            .kind = c4c::backend::bir::Value::Kind::Named,
            .type = c4c::backend::bir::TypeKind::Ptr,
            .name = std::string(pointer_name),
        });
        break;
      }
      default:
        return std::nullopt;
    }

    if (!base_offset.has_value()) {
      return std::nullopt;
    }
    const auto signed_offset = static_cast<std::int64_t>(*base_offset) +
                               prepared_access->address.byte_offset;
    if (signed_offset < 0) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(signed_offset);
  };

  const auto fold_binary_i32 =
      [&](const c4c::backend::bir::BinaryInst& binary) -> std::optional<std::int32_t> {
        const auto lhs = resolve_i32_value(binary.lhs);
        const auto rhs = resolve_i32_value(binary.rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        switch (binary.opcode) {
          case c4c::backend::bir::BinaryOpcode::Add:
            return wrap_i32(static_cast<std::int64_t>(*lhs) + *rhs);
          case c4c::backend::bir::BinaryOpcode::Sub:
            return wrap_i32(static_cast<std::int64_t>(*lhs) - *rhs);
          case c4c::backend::bir::BinaryOpcode::Mul:
            return wrap_i32(static_cast<std::int64_t>(*lhs) * *rhs);
          case c4c::backend::bir::BinaryOpcode::And:
            return static_cast<std::int32_t>(*lhs & *rhs);
          case c4c::backend::bir::BinaryOpcode::Or:
            return static_cast<std::int32_t>(*lhs | *rhs);
          case c4c::backend::bir::BinaryOpcode::Xor:
            return static_cast<std::int32_t>(*lhs ^ *rhs);
          case c4c::backend::bir::BinaryOpcode::Shl:
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(*lhs) <<
                                             (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::LShr:
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(*lhs) >>
                                             (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::AShr:
            return static_cast<std::int32_t>(*lhs >> (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::SDiv:
            if (*rhs == 0) {
              return std::nullopt;
            }
            return static_cast<std::int32_t>(*lhs / *rhs);
          case c4c::backend::bir::BinaryOpcode::SRem:
            if (*rhs == 0) {
              return std::nullopt;
            }
            return static_cast<std::int32_t>(*lhs % *rhs);
          default:
            return std::nullopt;
        }
      };

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); ++inst_index) {
    const auto& inst = entry.insts[inst_index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto address = resolve_prepared_memory_address(inst_index);
      if (!address.has_value()) {
        return std::nullopt;
      }
      if (store->value.type == c4c::backend::bir::TypeKind::I32) {
        const auto stored_value = resolve_i32_value(store->value);
        if (!stored_value.has_value()) {
          return std::nullopt;
        }
        local_memory[*address] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::I32,
            .value = *stored_value,
        };
        continue;
      }
      if (store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        const auto stored_value = resolve_ptr_value(store->value);
        local_memory[*address] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::Ptr,
            .value = stored_value.has_value() ? static_cast<std::int64_t>(*stored_value) : -1,
        };
        continue;
      }
      return std::nullopt;
    }

    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      const auto address = resolve_prepared_memory_address(inst_index);
      if (!address.has_value()) {
        return std::nullopt;
      }
      const auto value_it = local_memory.find(*address);
      if (value_it == local_memory.end() || value_it->second.type != load->result.type) {
        return std::nullopt;
      }
      named_values[load->result.name] = value_it->second;
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    const auto folded = fold_binary_i32(*binary);
    if (!folded.has_value()) {
      return std::nullopt;
    }
    named_values[binary->result.name] = ConstantValue{
        .type = c4c::backend::bir::TypeKind::I32,
        .value = *folded,
    };
  }

  const auto folded_return = resolve_i32_value(*entry.terminator.value);
  if (!folded_return.has_value()) {
    return std::nullopt;
  }
  return std::string(asm_prefix) + "    mov " + std::string(return_register) + ", " +
         std::to_string(static_cast<std::int32_t>(*folded_return)) + "\n    ret\n";
}

}  // namespace

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register) {
  return render_prepared_constant_folded_single_block_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
      });
}

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_constant_folded_single_block_return_from_context(context);
}

std::optional<std::string> render_prepared_param_derived_i32_value_if_supported(
    std::string_view return_register,
    const c4c::backend::bir::Value& value,
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
        named_binaries,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named || param.type != value.type ||
      param.is_varargs || param.is_sret || param.is_byval) {
    return std::nullopt;
  }

  if (const auto param_register = minimal_param_register(param);
      value.name == param.name && param_register.has_value()) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n";
  }

  const auto binary_it = named_binaries.find(value.name);
  if (binary_it == named_binaries.end()) {
    return std::nullopt;
  }

  const auto& binary = *binary_it->second;
  if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  const bool lhs_is_param_rhs_is_imm =
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.lhs.name == param.name &&
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_param_lhs_is_imm =
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.rhs.name == param.name &&
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_param_rhs_is_imm && !rhs_is_param_lhs_is_imm) {
    return std::nullopt;
  }

  const auto param_register = minimal_param_register(param);
  if (!param_register.has_value()) {
    return std::nullopt;
  }
  const auto immediate =
      lhs_is_param_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    add " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sub " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    return "    mov " + std::string(return_register) + ", " + *param_register +
           "\n    imul " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    and " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    or " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    xor " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shl " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shr " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sar " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  return std::nullopt;
}

namespace {

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr || !context.minimal_param_register) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  const auto& minimal_param_register = context.minimal_param_register;
  if (function.blocks.size() != 1 || entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& returned = *entry.terminator.value;
  if (returned.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (entry.insts.empty() && function.params.empty() &&
      returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return std::string(asm_prefix) + "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n    ret\n";
  }
  if (prepared_arch != c4c::TargetArch::X86_64 || function.params.size() != 1) {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
  if (entry.insts.empty()) {
    const auto value_render = render_prepared_param_derived_i32_value_if_supported(
        return_register, returned, named_binaries, param, minimal_param_register);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return std::string(asm_prefix) + render_prepared_return_body(*value_render);
  }

  if (entry.insts.size() != 1 || returned.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
  if (binary == nullptr || returned.name != binary->result.name) {
    return std::nullopt;
  }
  named_binaries.emplace(binary->result.name, binary);

  const auto value_render = render_prepared_param_derived_i32_value_if_supported(
      return_register, returned, named_binaries, param, minimal_param_register);
  if (!value_render.has_value()) {
    return std::nullopt;
  }
  return std::string(asm_prefix) + render_prepared_return_body(*value_render);
}

}  // namespace

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  return render_prepared_minimal_immediate_or_param_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
          .minimal_param_register = minimal_param_register,
      });
}

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_immediate_or_param_return_from_context(context);
}

namespace {

std::optional<std::string> render_prepared_minimal_local_slot_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  if (function.params.empty() == false || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      function.blocks.front().insts.empty()) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto& returned = *entry.terminator.value;
  const auto* final_load = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts.back());
  if (final_load == nullptr || returned.kind != c4c::backend::bir::Value::Kind::Named ||
      returned.type != c4c::backend::bir::TypeKind::I32 ||
      final_load->result.name != returned.name ||
      final_load->result.type != c4c::backend::bir::TypeKind::I32 ||
      final_load->byte_offset != 0) {
    return std::nullopt;
  }

  auto asm_text = std::string(asm_prefix);
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); ++inst_index) {
    const auto& inst = entry.insts[inst_index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (store->byte_offset != 0) {
        return std::nullopt;
      }
      auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
          *layout,
          prepared_names,
          function_addressing,
          entry_label_id,
          inst_index,
          store->value.type,
          nullptr);
      if (!memory.has_value()) {
        return std::nullopt;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I32) {
        const auto rendered_store = render_prepared_i32_store_to_memory_if_supported(
            store->value,
            std::nullopt,
            *memory,
            [](const c4c::backend::bir::Value& value,
               const std::optional<std::string_view>&) -> std::optional<std::string> {
              if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
                return std::nullopt;
              }
              return std::to_string(static_cast<std::int32_t>(value.immediate));
            });
        if (!rendered_store.has_value()) {
          return std::nullopt;
        }
        asm_text += *rendered_store;
        continue;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        const auto rendered_store = render_prepared_ptr_store_to_memory_if_supported(
            store->value,
            std::nullopt,
            *memory,
            [&](std::string_view value_name) -> std::optional<std::string> {
              return render_prepared_value_home_stack_address_if_supported(
                  *layout, prepared_names, function_locations, value_name);
            });
        if (!rendered_store.has_value()) {
          return std::nullopt;
        }
        asm_text += *rendered_store;
        continue;
      }
      return std::nullopt;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0) {
      return std::nullopt;
    }
    auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
        *layout,
        prepared_names,
        function_addressing,
        entry_label_id,
        inst_index,
        load->result.type,
        nullptr);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    const auto rendered_load =
        render_prepared_scalar_load_from_memory_if_supported(load->result.type, *memory);
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    asm_text += *rendered_load;
  }

  if (layout->frame_size != 0) {
    asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

}  // namespace

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  return render_prepared_minimal_local_slot_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
      });
}

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_local_slot_return_from_context(context);
}

std::optional<std::string> render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function& candidate,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix) {
  if (prepared_arch != c4c::TargetArch::X86_64 || !candidate.params.empty() ||
      !candidate.local_slots.empty() || candidate.blocks.size() != 1) {
    return std::nullopt;
  }
  const auto& candidate_entry = candidate.blocks.front();
  if (!candidate_entry.insts.empty() ||
      candidate_entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return) {
    return std::nullopt;
  }
  if (candidate.return_type == c4c::backend::bir::TypeKind::Void &&
      !candidate_entry.terminator.value.has_value()) {
    return minimal_function_asm_prefix(candidate) + "    ret\n";
  }
  if (candidate.return_type != c4c::backend::bir::TypeKind::I32 ||
      !candidate_entry.terminator.value.has_value() ||
      candidate_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto candidate_return_register = minimal_function_return_register(candidate);
  if (!candidate_return_register.has_value()) {
    return std::nullopt;
  }
  return minimal_function_asm_prefix(candidate) + "    mov " + *candidate_return_register + ", " +
         std::to_string(static_cast<std::int32_t>(candidate_entry.terminator.value->immediate)) +
         "\n    ret\n";
}

namespace {

struct PreparedDirectExternNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

struct PreparedDirectExternCurrentI32Carrier {
  std::string_view value_name;
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
};

const c4c::backend::prepare::PreparedValueHome* find_prepared_direct_extern_named_value_home(
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_value_home(
      *prepared_names, *function_locations, value_name);
}

std::optional<PreparedDirectExternNamedI32Source>
select_prepared_direct_extern_named_i32_source_if_supported(
    std::string_view value_name,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32.has_value() && current_i32->value_name == value_name) {
    return PreparedDirectExternNamedI32Source{
        .register_name = current_i32->register_name,
        .stack_operand = current_i32->stack_operand,
        .immediate_i32 = std::nullopt,
    };
  }
  const auto constant_it = i32_constants.find(value_name);
  if (constant_it != i32_constants.end()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = constant_it->second,
    };
  }
  const auto* home = find_prepared_direct_extern_named_value_home(
      prepared_names, function_locations, value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = narrow_i32_register(*home->register_name),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD"),
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = *home->immediate_i32,
    };
  }
  return std::nullopt;
}

bool append_prepared_direct_extern_i32_move_into_register_if_supported(
    std::string* rendered_body,
    std::string_view destination_register,
    const PreparedDirectExternNamedI32Source& source) {
  if (source.immediate_i32.has_value()) {
    *rendered_body += "    mov " + std::string(destination_register) + ", " +
                      std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
    return true;
  }
  if (source.register_name.has_value()) {
    if (*source.register_name != destination_register) {
      *rendered_body +=
          "    mov " + std::string(destination_register) + ", " + *source.register_name + "\n";
    }
    return true;
  }
  if (source.stack_operand.has_value()) {
    *rendered_body += "    mov " + std::string(destination_register) + ", " +
                      *source.stack_operand + "\n";
    return true;
  }
  return false;
}

bool append_prepared_direct_extern_call_argument_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::size_t arg_index,
    std::size_t instruction_index,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::unordered_set<std::string_view>* emitted_string_names,
    std::vector<const c4c::backend::bir::StringConstant*>* used_string_constants,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::string* body) {
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
    if (arg.kind != c4c::backend::bir::Value::Kind::Named || arg.name.empty() ||
        arg.name.front() != '@') {
      return false;
    }
    const std::string_view symbol_name(arg.name.data() + 1, arg.name.size() - 1);
    if (const auto* string_constant = find_string_constant(symbol_name); string_constant != nullptr) {
      if (emitted_string_names->insert(symbol_name).second) {
        used_string_constants->push_back(string_constant);
      }
      *body += "    lea ";
      *body += kArgRegs64[arg_index];
      *body += ", [rip + ";
      *body += render_private_data_label(arg.name);
      *body += "]\n";
      return true;
    }
    const auto* global = find_same_module_global(symbol_name);
    if (global == nullptr) {
      return false;
    }
    used_same_module_globals->insert(global->name);
    *body += "    lea ";
    *body += kArgRegs64[arg_index];
    *body += ", [rip + ";
    *body += render_asm_symbol_name(global->name);
    *body += "]\n";
    return true;
  }

  if (arg_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  PreparedDirectExternNamedI32Source source;
  if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
    source.immediate_i32 = static_cast<std::int32_t>(arg.immediate);
  } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
    const auto named_source = select_prepared_direct_extern_named_i32_source_if_supported(
        arg.name, current_i32, i32_constants, prepared_names, function_locations);
    if (!named_source.has_value()) {
      return false;
    }
    source = *named_source;
  } else {
    return false;
  }

  const auto destination_register = select_prepared_i32_call_argument_abi_register_if_supported(
      function_locations, instruction_index, arg_index);
  if (!destination_register.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  return append_prepared_direct_extern_i32_move_into_register_if_supported(
      body, *destination_register, source);
}

bool finalize_prepared_direct_extern_call_result_if_supported(
    const c4c::backend::bir::CallInst& call,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string* body,
    std::optional<PreparedDirectExternCurrentI32Carrier>* current_i32) {
  if (!call.result.has_value() || call.result->type != c4c::backend::bir::TypeKind::I32 ||
      call.result->kind != c4c::backend::bir::Value::Kind::Named) {
    *current_i32 = std::nullopt;
    return true;
  }

  const auto* result_home = find_prepared_direct_extern_named_value_home(
      prepared_names, function_locations, call.result->name);
  const auto call_result_selection =
      select_prepared_i32_call_result_abi_if_supported(
          function_locations, instruction_index, result_home);
  if (!call_result_selection.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  if (call_result_selection->move != nullptr) {
    if (call_result_selection->move->destination_storage_kind !=
        c4c::backend::prepare::PreparedMoveStorageKind::Register) {
      return false;
    }
    if (result_home == nullptr) {
      return false;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto home_register = narrow_i32_register(*result_home->register_name);
      if (!home_register.has_value()) {
        return false;
      }
      if (*home_register != call_result_selection->abi_register) {
        *body += "    mov " + *home_register + ", " + call_result_selection->abi_register + "\n";
      }
      *current_i32 = PreparedDirectExternCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = *home_register,
          .stack_operand = std::nullopt,
      };
      return true;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        result_home->offset_bytes.has_value()) {
      const auto stack_operand =
          render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD");
      *body += "    mov " + stack_operand + ", " + call_result_selection->abi_register + "\n";
      *current_i32 = PreparedDirectExternCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = std::nullopt,
          .stack_operand = stack_operand,
      };
      return true;
    }
    return false;
  }

  *current_i32 = PreparedDirectExternCurrentI32Carrier{
      .value_name = call.result->name,
      .register_name = call_result_selection->abi_register,
      .stack_operand = std::nullopt,
  };
  return true;
}

bool finalize_prepared_direct_extern_return_if_supported(
    const c4c::backend::bir::Value& returned,
    std::string_view return_register,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::function<std::optional<std::int64_t>(const c4c::backend::bir::Value&)>&
        resolve_i32_constant,
    std::string* body) {
  if (returned.kind == c4c::backend::bir::Value::Kind::Named &&
      current_i32.has_value() && returned.name == current_i32->value_name &&
      current_i32->register_name.has_value() &&
      *current_i32->register_name == std::string(return_register)) {
    *body += "    add rsp, 8\n    ret\n";
    return true;
  }

  const auto returned_value = resolve_i32_constant(returned);
  if (!returned_value.has_value()) {
    return false;
  }
  *body += "    mov ";
  *body += std::string(return_register);
  *body += ", ";
  *body += std::to_string(static_cast<std::int32_t>(*returned_value));
  *body += "\n    add rsp, 8\n    ret\n";
  return true;
}

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr ||
      context.bounded_same_module_helper_global_names == nullptr || !context.find_string_constant ||
      !context.find_same_module_global || !context.render_private_data_label ||
      !context.render_asm_symbol_name || !context.emit_string_constant_data ||
      !context.emit_same_module_global_data || !context.prepend_bounded_same_module_helpers) {
    return std::nullopt;
  }
  const auto& module = *context.module;
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  const auto& bounded_same_module_helper_global_names =
      *context.bounded_same_module_helper_global_names;
  const auto& find_string_constant = context.find_string_constant;
  const auto& find_same_module_global = context.find_same_module_global;
  const auto& render_private_data_label = context.render_private_data_label;
  const auto& render_asm_symbol_name = context.render_asm_symbol_name;
  const auto& emit_string_constant_data = context.emit_string_constant_data;
  const auto& emit_same_module_global_data = context.emit_same_module_global_data;
  const auto& prepend_bounded_same_module_helpers = context.prepend_bounded_same_module_helpers;
  if (prepared_arch != c4c::TargetArch::X86_64 || !function.params.empty() ||
      !function.local_slots.empty() || function.blocks.size() != 1 || entry.label != "entry" ||
      entry.insts.empty() ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::int64_t> i32_constants;
  std::unordered_set<std::string_view> emitted_string_names;
  std::vector<const c4c::backend::bir::StringConstant*> used_string_constants;
  std::unordered_set<std::string_view> used_same_module_globals;
  std::string body = "    sub rsp, 8\n";
  bool saw_call = false;
  std::optional<PreparedDirectExternCurrentI32Carrier> current_i32;

  const auto resolve_i32_constant =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::int64_t> {
        if (value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return static_cast<std::int32_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto constant_it = i32_constants.find(value.name);
        if (constant_it == i32_constants.end()) {
          return std::nullopt;
        }
        return constant_it->second;
      };
  const auto fold_binary_immediate =
      [&](const c4c::backend::bir::BinaryInst& binary) -> std::optional<std::int64_t> {
        const auto lhs = resolve_i32_constant(binary.lhs);
        const auto rhs = resolve_i32_constant(binary.rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        switch (binary.opcode) {
          case c4c::backend::bir::BinaryOpcode::Add:
            return static_cast<std::int32_t>(*lhs + *rhs);
          case c4c::backend::bir::BinaryOpcode::Sub:
            return static_cast<std::int32_t>(*lhs - *rhs);
          case c4c::backend::bir::BinaryOpcode::Mul:
            return static_cast<std::int32_t>(*lhs * *rhs);
          case c4c::backend::bir::BinaryOpcode::And:
            return static_cast<std::int32_t>(*lhs & *rhs);
          case c4c::backend::bir::BinaryOpcode::Or:
            return static_cast<std::int32_t>(*lhs | *rhs);
          case c4c::backend::bir::BinaryOpcode::Xor:
            return static_cast<std::int32_t>(*lhs ^ *rhs);
          case c4c::backend::bir::BinaryOpcode::Shl:
            return static_cast<std::int32_t>(
                static_cast<std::uint32_t>(*lhs) << static_cast<std::uint32_t>(*rhs));
          case c4c::backend::bir::BinaryOpcode::LShr:
            return static_cast<std::int32_t>(
                static_cast<std::uint32_t>(*lhs) >> static_cast<std::uint32_t>(*rhs));
          case c4c::backend::bir::BinaryOpcode::AShr:
            return static_cast<std::int32_t>(*lhs >> *rhs);
          default:
            return std::nullopt;
        }
      };
  for (const auto& inst : entry.insts) {
    if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
      if (binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
          binary->result.type != c4c::backend::bir::TypeKind::I32 ||
          binary->operand_type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto folded = fold_binary_immediate(*binary);
      if (!folded.has_value()) {
        return std::nullopt;
      }
      i32_constants.emplace(binary->result.name, *folded);
      current_i32 = std::nullopt;
      continue;
    }

    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr || call->is_indirect || call->callee.empty() ||
        call->callee_value.has_value() || call->args.size() != call->arg_types.size() ||
        call->args.size() > 6) {
      return std::nullopt;
    }
    saw_call = true;
    const auto instruction_index =
        static_cast<std::size_t>(&inst - entry.insts.data());

    for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
      if (!append_prepared_direct_extern_call_argument_if_supported(
              call->args[arg_index],
              call->arg_types[arg_index],
              arg_index,
              instruction_index,
              current_i32,
              i32_constants,
              prepared_names,
              function_locations,
              find_string_constant,
              find_same_module_global,
              render_private_data_label,
              render_asm_symbol_name,
              &emitted_string_names,
              &used_string_constants,
              &used_same_module_globals,
              &body)) {
        return std::nullopt;
      }
    }

    body += "    xor eax, eax\n";
    body += "    call ";
    body += render_asm_symbol_name(call->callee);
    body += "\n";
    if (!finalize_prepared_direct_extern_call_result_if_supported(
            *call, instruction_index, prepared_names, function_locations, &body, &current_i32)) {
      return std::nullopt;
    }
  }

  if (!saw_call) {
    return std::nullopt;
  }

  if (!finalize_prepared_direct_extern_return_if_supported(
          *entry.terminator.value, return_register, current_i32, resolve_i32_constant, &body)) {
    return std::nullopt;
  }

  std::string rendered_data;
  for (const auto* string_constant : used_string_constants) {
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& global : module.globals) {
    if (used_same_module_globals.find(global.name) == used_same_module_globals.end() &&
        bounded_same_module_helper_global_names.find(global.name) ==
            bounded_same_module_helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_data += *rendered_global_data;
  }
  return prepend_bounded_same_module_helpers(std::string(asm_prefix) + body + rendered_data);
}

}  // namespace

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers) {
  return render_prepared_minimal_direct_extern_call_sequence_from_context(
      PreparedX86FunctionDispatchContext{
          .module = &module,
          .function = &function,
          .entry = &entry,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
          .bounded_same_module_helper_global_names = &bounded_same_module_helper_global_names,
          .find_string_constant = find_string_constant,
          .find_same_module_global = find_same_module_global,
          .render_private_data_label = render_private_data_label,
          .render_asm_symbol_name = render_asm_symbol_name,
          .emit_string_constant_data = emit_string_constant_data,
          .emit_same_module_global_data = emit_same_module_global_data,
          .prepend_bounded_same_module_helpers = prepend_bounded_same_module_helpers,
      });
}

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_direct_extern_call_sequence_from_context(context);
}

std::optional<std::string> render_prepared_single_block_return_direct_extern_call_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_direct_extern_call_sequence_if_supported(context);
}

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr ||
      context.bounded_same_module_helper_global_names == nullptr ||
      !context.find_string_constant || !context.find_same_module_global ||
      !context.render_private_data_label || !context.render_asm_symbol_name ||
      !context.emit_string_constant_data || !context.emit_same_module_global_data ||
      !context.prepend_bounded_same_module_helpers || !context.minimal_param_register) {
    return std::nullopt;
  }
  if (const auto rendered_direct_calls =
          render_prepared_single_block_return_direct_extern_call_if_supported(context);
      rendered_direct_calls.has_value()) {
    return *rendered_direct_calls;
  }
  if (const auto rendered_local_slot =
          render_prepared_minimal_local_slot_return_if_supported(context);
      rendered_local_slot.has_value()) {
    return *rendered_local_slot;
  }
  if (const auto rendered_constant_folded =
          render_prepared_constant_folded_single_block_return_if_supported(context);
      rendered_constant_folded.has_value()) {
    return *rendered_constant_folded;
  }
  if (const auto rendered_local_i16_i64_return =
          render_prepared_local_i16_i64_sub_return_if_supported(context);
      rendered_local_i16_i64_return.has_value()) {
    return *rendered_local_i16_i64_return;
  }
  return render_prepared_minimal_immediate_or_param_return_if_supported(context);
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }

  PreparedBoundedMultiDefinedCallLaneModuleRender rendered;
  std::unordered_set<std::string> emitted_string_names;
  std::unordered_set<std::string> emitted_same_module_global_names;
  bool saw_bounded_entry = false;

  for (const auto* candidate : defined_functions) {
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered.rendered_functions += *rendered_trivial;
      continue;
    }

    if (saw_bounded_entry || candidate->return_type != c4c::backend::bir::TypeKind::I32 ||
        !candidate->params.empty() || candidate->blocks.size() != 1 ||
        candidate->blocks.front().label != "entry" ||
        candidate->blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !candidate->blocks.front().terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto candidate_return_register = minimal_function_return_register(*candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }

    const auto candidate_layout = build_prepared_module_local_slot_layout(
        *candidate,
        static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
        prepared_arch);
    if (!candidate_layout.has_value()) {
      return std::nullopt;
    }
    const auto* function_locations =
        c4c::backend::prepare::find_prepared_value_location_function(module, candidate->name);
    if (function_locations == nullptr) {
      return std::nullopt;
    }
    const auto rendered_candidate =
        render_prepared_bounded_multi_defined_call_lane_body_if_supported(
            module, *function_locations, *candidate, defined_functions, *candidate_layout,
            *candidate_return_register,
            has_string_constant, has_same_module_global, render_private_data_label,
            render_asm_symbol_name);
    if (!rendered_candidate.has_value()) {
      return std::nullopt;
    }
    for (const auto& string_name : rendered_candidate->used_string_names) {
      if (emitted_string_names.insert(string_name).second) {
        rendered.used_string_names.push_back(string_name);
      }
    }
    for (const auto& global_name : rendered_candidate->used_same_module_global_names) {
      if (emitted_same_module_global_names.insert(global_name).second) {
        rendered.used_same_module_global_names.push_back(global_name);
      }
    }
    rendered.rendered_functions +=
        minimal_function_asm_prefix(*candidate) + rendered_candidate->body;
    saw_bounded_entry = true;
  }

  if (!saw_bounded_entry) {
    return std::nullopt;
  }

  return rendered;
}

std::optional<PreparedBoundedSameModuleHelperPrefixRender>
render_prepared_bounded_same_module_helper_prefix_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || entry_function.name != "main") {
    return std::nullopt;
  }

  struct BoundedSameModuleHelperRender {
    std::string asm_text;
    std::unordered_set<std::string_view> used_same_module_globals;
  };

  const auto render_bounded_same_module_helper_function_if_supported =
      [&](const c4c::backend::bir::Function& candidate)
      -> std::optional<BoundedSameModuleHelperRender> {
    if (prepared_arch != c4c::TargetArch::X86_64 || candidate.local_slots.empty() == false ||
        candidate.blocks.size() != 1 || candidate.blocks.front().label != "entry" ||
        candidate.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !candidate.blocks.front().terminator.value.has_value() || candidate.params.size() > 6) {
      return std::nullopt;
    }

    std::unordered_map<std::string_view, std::string> param_registers;
    for (std::size_t param_index = 0; param_index < candidate.params.size(); ++param_index) {
      const auto& param = candidate.params[param_index];
      if (param.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto param_register = minimal_param_register_at(param, param_index);
      if (!param_register.has_value()) {
        return std::nullopt;
      }
      param_registers.emplace(param.name, *param_register);
    }

    const auto candidate_return_register = minimal_function_return_register(candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }
    const auto candidate_function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, candidate.name)
            .value_or(c4c::kInvalidFunctionName);
    const auto* candidate_function_addressing =
        c4c::backend::prepare::find_prepared_addressing(module, candidate_function_name_id);
    const c4c::BlockLabelId entry_block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(
            module.names, candidate.blocks.front().label)
            .value_or(c4c::kInvalidBlockLabel);

    const auto render_value_to_eax =
        [&](const c4c::backend::bir::Value& value,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          return render_prepared_i32_move_to_register_if_supported(
              value,
              current_i32_name,
              "eax",
              [&](std::string_view value_name) -> std::optional<std::string> {
                const auto param_it = param_registers.find(value_name);
                if (param_it == param_registers.end()) {
                  return std::nullopt;
                }
                return param_it->second;
              });
        };
    const auto render_i32_operand =
        [&](const c4c::backend::bir::Value& value,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          return render_prepared_i32_operand_if_supported(
              value,
              current_i32_name,
              [&](std::string_view value_name) -> std::optional<std::string> {
                const auto param_it = param_registers.find(value_name);
                if (param_it == param_registers.end()) {
                  return std::nullopt;
                }
                return param_it->second;
              });
        };
    const auto render_supported_helper_inst =
        [&](const c4c::backend::bir::Inst& inst,
            std::size_t inst_index,
            std::optional<std::string_view>* current_i32_name)
        -> std::optional<BoundedSameModuleHelperRender> {
      if (current_i32_name == nullptr) {
        return std::nullopt;
      }
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (binary->result.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto rendered_binary = render_prepared_i32_binary_in_eax_if_supported(
            *binary, *current_i32_name, render_value_to_eax, render_i32_operand);
        if (!rendered_binary.has_value()) {
          return std::nullopt;
        }
        *current_i32_name = binary->result.name;
        return BoundedSameModuleHelperRender{
            .asm_text = std::move(*rendered_binary),
            .used_same_module_globals = {},
        };
      }

      const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst);
      if (store == nullptr || store->address.has_value() || store->byte_offset != 0 ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto selected_global_memory =
          select_prepared_same_module_scalar_memory_for_inst_if_supported(
              module.names,
              candidate_function_addressing,
              entry_block_label_id,
              inst_index,
              c4c::backend::bir::TypeKind::I32,
              render_asm_symbol_name,
              find_same_module_global,
              [](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
                return global.type == c4c::backend::bir::TypeKind::I32 && byte_offset == 0;
              });
      if (!selected_global_memory.has_value()) {
        return std::nullopt;
      }

      const auto rendered_store = render_prepared_i32_store_to_memory_if_supported(
          store->value,
          *current_i32_name,
          selected_global_memory->memory_operand,
          [&](const c4c::backend::bir::Value& value,
              const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
            return render_prepared_i32_operand_if_supported(
                value,
                current_name,
                [&](std::string_view value_name) -> std::optional<std::string> {
                  const auto param_it = param_registers.find(value_name);
                  if (param_it == param_registers.end()) {
                    return std::nullopt;
                  }
                  return param_it->second;
                });
          });
      if (!rendered_store.has_value()) {
        return std::nullopt;
      }
      BoundedSameModuleHelperRender rendered{
          .asm_text = std::move(*rendered_store),
          .used_same_module_globals = {},
      };
      rendered.used_same_module_globals.insert(selected_global_memory->global->name);
      return rendered;
    };
    std::unordered_set<std::string_view> used_same_module_globals;
    std::string body;
    std::optional<std::string_view> current_i32_name;
    for (const auto& inst : candidate.blocks.front().insts) {
      const auto inst_index = static_cast<std::size_t>(&inst - candidate.blocks.front().insts.data());
      const auto rendered_inst =
          render_supported_helper_inst(inst, inst_index, &current_i32_name);
      if (!rendered_inst.has_value()) {
        return std::nullopt;
      }
      body += rendered_inst->asm_text;
      used_same_module_globals.insert(
          rendered_inst->used_same_module_globals.begin(),
          rendered_inst->used_same_module_globals.end());
    }

    const auto& returned = *candidate.blocks.front().terminator.value;
    const auto render_return_to_register = render_prepared_i32_move_to_register_if_supported(
        returned,
        current_i32_name,
        *candidate_return_register,
        [&](std::string_view value_name) -> std::optional<std::string> {
          const auto param_it = param_registers.find(value_name);
          if (param_it == param_registers.end()) {
            return std::nullopt;
          }
          return param_it->second;
        });
    if (!render_return_to_register.has_value()) {
      return std::nullopt;
    }
    body += *render_return_to_register;
    body += "    ret\n";
    return BoundedSameModuleHelperRender{
        .asm_text = minimal_function_asm_prefix(candidate) + body,
        .used_same_module_globals = std::move(used_same_module_globals),
    };
  };

  PreparedBoundedSameModuleHelperPrefixRender rendered_helpers;
  for (const auto* candidate : defined_functions) {
    if (candidate == &entry_function) {
      continue;
    }
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered_helpers.helper_names.insert(candidate->name);
      rendered_helpers.helper_prefix += *rendered_trivial;
      continue;
    }
    const auto rendered_helper =
        render_bounded_same_module_helper_function_if_supported(*candidate);
    if (!rendered_helper.has_value()) {
      return std::nullopt;
    }
    for (const auto global_name : rendered_helper->used_same_module_globals) {
      rendered_helpers.helper_global_names.insert(global_name);
    }
    rendered_helpers.helper_names.insert(candidate->name);
    rendered_helpers.helper_prefix += rendered_helper->asm_text;
  }

  return rendered_helpers;
}

std::optional<std::string>
render_prepared_bounded_multi_defined_call_lane_data_if_supported(
    const PreparedBoundedMultiDefinedCallLaneModuleRender& rendered_module,
    const c4c::backend::bir::Module& module,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  std::string rendered_data;
  std::unordered_set<std::string_view> used_same_module_globals(
      rendered_module.used_same_module_global_names.begin(),
      rendered_module.used_same_module_global_names.end());
  for (const auto& string_name : rendered_module.used_string_names) {
    const auto* string_constant = find_string_constant(string_name);
    if (string_constant == nullptr) {
      return std::nullopt;
    }
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& global : module.globals) {
    if (used_same_module_globals.find(global.name) == used_same_module_globals.end() &&
        helper_global_names.find(global.name) == helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_data += *rendered_global_data;
  }
  return rendered_data;
}

std::optional<std::string> render_prepared_bounded_multi_defined_call_lane_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  const auto rendered_module = render_prepared_bounded_multi_defined_call_lane_module_if_supported(
      module, defined_functions, prepared_arch, render_trivial_defined_function,
      minimal_function_return_register, minimal_function_asm_prefix, has_string_constant,
      has_same_module_global, render_private_data_label, render_asm_symbol_name);
  if (!rendered_module.has_value()) {
    return std::nullopt;
  }

  const auto rendered_data = render_prepared_bounded_multi_defined_call_lane_data_if_supported(
      *rendered_module, module.module, helper_global_names, find_string_constant,
      emit_string_constant_data,
      emit_same_module_global_data);
  if (!rendered_data.has_value()) {
    return std::nullopt;
  }
  return rendered_module->rendered_functions + *rendered_data;
}

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
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  PreparedModuleMultiDefinedDispatchState state;
  if (entry_function != nullptr) {
    if (const auto helpers = render_prepared_bounded_same_module_helper_prefix_if_supported(
            module, defined_functions, *entry_function, prepared_arch,
            render_trivial_defined_function, minimal_function_return_register,
            minimal_function_asm_prefix, find_same_module_global, minimal_param_register_at,
            render_asm_symbol_name);
        helpers.has_value()) {
      state.helper_prefix = helpers->helper_prefix;
      state.helper_names = std::move(helpers->helper_names);
      state.helper_global_names = std::move(helpers->helper_global_names);
      state.has_bounded_same_module_helpers = true;
    }
  }
  state.rendered_module = render_prepared_bounded_multi_defined_call_lane_if_supported(
      module, defined_functions, prepared_arch, state.helper_global_names,
      render_trivial_defined_function, minimal_function_return_register, minimal_function_asm_prefix,
      has_string_constant, has_same_module_global, render_private_data_label, render_asm_symbol_name,
      find_string_constant, emit_string_constant_data, emit_same_module_global_data);
  return state;
}

}  // namespace c4c::backend::x86
