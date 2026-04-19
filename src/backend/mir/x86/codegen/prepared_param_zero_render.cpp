#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

PreparedParamZeroCompareShape to_prepared_param_zero_compare_shape(
    c4c::backend::prepare::PreparedParamZeroBranchCondition::CompareShape compare_shape) {
  switch (compare_shape) {
    case c4c::backend::prepare::PreparedParamZeroBranchCondition::CompareShape::SelfTest:
      return PreparedParamZeroCompareShape::SelfTest;
  }
  throw std::invalid_argument("unsupported prepared param-zero compare shape");
}

std::optional<std::string> render_prepared_param_zero_branch_prefix(
    PreparedParamZeroCompareShape compare_shape,
    std::string_view function_name,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view param_register_name) {
  if (false_branch_opcode == nullptr || false_label.empty()) {
    return std::nullopt;
  }

  std::string rendered_compare;
  switch (compare_shape) {
    case PreparedParamZeroCompareShape::SelfTest:
      rendered_compare = "    test " + std::string(param_register_name) + ", " +
                         std::string(param_register_name) + "\n";
      break;
  }

  return rendered_compare + "    " + std::string(false_branch_opcode) + " .L" +
         std::string(function_name) + "_" + std::string(false_label) + "\n";
}

}  // namespace

std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name) {
  if (current_materialized_compare.has_value() &&
      current_materialized_compare->i32_name.has_value()) {
    const bool lhs_is_materialized_rhs_is_zero =
        compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare.lhs.name == *current_materialized_compare->i32_name &&
        compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare.rhs.type == c4c::backend::bir::TypeKind::I32 &&
        compare.rhs.immediate == 0;
    const bool rhs_is_materialized_lhs_is_zero =
        compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        compare.rhs.name == *current_materialized_compare->i32_name &&
        compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
        compare.lhs.immediate == 0;
    if (lhs_is_materialized_rhs_is_zero || rhs_is_materialized_lhs_is_zero) {
      const bool branch_when_original_false =
          compare.opcode == c4c::backend::bir::BinaryOpcode::Ne;
      const char* branch_opcode = nullptr;
      if (current_materialized_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq) {
        branch_opcode = branch_when_original_false ? "jne" : "je";
      } else {
        branch_opcode = branch_when_original_false ? "je" : "jne";
      }
      return std::pair<std::string, std::string>{
          current_materialized_compare->compare_setup,
          branch_opcode,
      };
    }
  }

  if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
      compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.rhs.type == c4c::backend::bir::TypeKind::I32) {
    const auto compare_setup =
        "    mov eax, " + std::to_string(static_cast<std::int32_t>(compare.lhs.immediate)) +
        "\n    cmp eax, " + std::to_string(static_cast<std::int32_t>(compare.rhs.immediate)) +
        "\n";
    const char* branch_opcode = nullptr;
    switch (compare.opcode) {
      case c4c::backend::bir::BinaryOpcode::Eq:
        branch_opcode = "jne";
        break;
      case c4c::backend::bir::BinaryOpcode::Ne:
        branch_opcode = "je";
        break;
      case c4c::backend::bir::BinaryOpcode::Sgt:
        branch_opcode = "jle";
        break;
      case c4c::backend::bir::BinaryOpcode::Sge:
        branch_opcode = "jl";
        break;
      case c4c::backend::bir::BinaryOpcode::Slt:
        branch_opcode = "jge";
        break;
      case c4c::backend::bir::BinaryOpcode::Sle:
        branch_opcode = "jg";
        break;
      default:
        return std::nullopt;
    }
    return std::pair<std::string, std::string>{compare_setup, branch_opcode};
  }

  if (!current_i32_name.has_value()) {
    return std::nullopt;
  }
  const bool lhs_is_current_rhs_is_imm =
      compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      compare.lhs.name == *current_i32_name &&
      compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_current_lhs_is_imm =
      compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      compare.rhs.name == *current_i32_name &&
      compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      compare.lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_current_rhs_is_imm && !rhs_is_current_lhs_is_imm) {
    return std::nullopt;
  }
  const auto compare_immediate =
      lhs_is_current_rhs_is_imm ? compare.rhs.immediate : compare.lhs.immediate;
  const auto compare_setup =
      compare_immediate == 0
          ? std::string("    test eax, eax\n")
          : "    cmp eax, " + std::to_string(static_cast<std::int32_t>(compare_immediate)) +
                "\n";
  const auto branch_opcode_for_current_immediate =
      [&](bool current_is_lhs) -> const char* {
    switch (compare.opcode) {
      case c4c::backend::bir::BinaryOpcode::Eq:
        return "jne";
      case c4c::backend::bir::BinaryOpcode::Ne:
        return "je";
      case c4c::backend::bir::BinaryOpcode::Sgt:
        return current_is_lhs ? "jle" : "jge";
      case c4c::backend::bir::BinaryOpcode::Sge:
        return current_is_lhs ? "jl" : "jg";
      case c4c::backend::bir::BinaryOpcode::Slt:
        return current_is_lhs ? "jge" : "jle";
      case c4c::backend::bir::BinaryOpcode::Sle:
        return current_is_lhs ? "jg" : "jl";
      default:
        return nullptr;
    }
  };
  const char* branch_opcode = branch_opcode_for_current_immediate(lhs_is_current_rhs_is_imm);
  if (branch_opcode == nullptr) {
    return std::nullopt;
  }
  return std::pair<std::string, std::string>{compare_setup, branch_opcode};
}

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_from_condition(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name) {
  if (!condition.predicate.has_value() || !condition.lhs.has_value() ||
      !condition.rhs.has_value()) {
    return std::nullopt;
  }

  c4c::backend::bir::BinaryInst compare{
      .opcode = *condition.predicate,
      .result = condition.condition_value,
      .operand_type = condition.compare_type.value_or(c4c::backend::bir::TypeKind::I32),
      .lhs = *condition.lhs,
      .rhs = *condition.rhs,
  };
  return render_prepared_guard_false_branch_compare(
      compare, current_materialized_compare, current_i32_name);
}

std::optional<ShortCircuitEntryCompareContext> build_prepared_guard_compare_context(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name) {
  auto false_branch_compare = render_prepared_guard_false_branch_compare_from_condition(
      branch_condition, current_materialized_compare, current_i32_name);
  if (!false_branch_compare.has_value()) {
    return std::nullopt;
  }
  return ShortCircuitEntryCompareContext{
      .branch_condition = &branch_condition,
      .compare_setup = std::move(false_branch_compare->first),
      .false_branch_opcode = std::move(false_branch_compare->second),
  };
}

bool is_supported_guard_compare_opcode(c4c::backend::bir::BinaryOpcode opcode) {
  return opcode == c4c::backend::bir::BinaryOpcode::Eq ||
         opcode == c4c::backend::bir::BinaryOpcode::Ne ||
         opcode == c4c::backend::bir::BinaryOpcode::Sgt ||
         opcode == c4c::backend::bir::BinaryOpcode::Sge ||
         opcode == c4c::backend::bir::BinaryOpcode::Slt ||
         opcode == c4c::backend::bir::BinaryOpcode::Sle;
}

const c4c::backend::prepare::PreparedBranchCondition* find_prepared_branch_condition_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    std::string_view block_label) {
  if (function_control_flow == nullptr) {
    return nullptr;
  }
  const c4c::BlockLabelId block_label_id = prepared_names.block_labels.find(block_label);
  if (block_label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_branch_condition(
      *function_control_flow, block_label_id);
}

const c4c::backend::prepare::PreparedBranchCondition*
find_prepared_i32_immediate_branch_condition_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    std::string_view block_label,
    const std::optional<std::string_view>& current_i32_name) {
  if (function_control_flow == nullptr || !current_i32_name.has_value()) {
    return nullptr;
  }
  const c4c::BlockLabelId block_label_id = prepared_names.block_labels.find(block_label);
  const c4c::ValueNameId value_name_id = prepared_names.value_names.find(*current_i32_name);
  if (block_label_id == c4c::kInvalidBlockLabel || value_name_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_i32_immediate_branch_condition(
      prepared_names, *function_control_flow, block_label_id, value_name_id);
}

const c4c::backend::bir::BinaryInst* find_trailing_guard_compare_if_supported(
    const c4c::backend::bir::Block& source_block,
    std::size_t current_compare_index,
    std::optional<std::string_view> expected_condition_name) {
  if (current_compare_index + 1 != source_block.insts.size()) {
    return nullptr;
  }

  const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&source_block.insts.back());
  if (compare == nullptr || !is_supported_guard_compare_opcode(compare->opcode) ||
      compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
      (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
       compare->result.type != c4c::backend::bir::TypeKind::I32)) {
    return nullptr;
  }
  if (expected_condition_name.has_value() && compare->result.name != *expected_condition_name) {
    return nullptr;
  }
  return compare;
}

std::optional<DirectBranchTargets> resolve_direct_branch_targets(
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    std::string_view true_label,
    std::string_view false_label) {
  DirectBranchTargets targets{
      .true_block = find_block(true_label),
      .false_block = find_block(false_label),
  };
  if (targets.true_block == nullptr || targets.false_block == nullptr) {
    return std::nullopt;
  }
  return targets;
}

std::optional<ShortCircuitPlan> build_direct_branch_plan_from_targets(
    const c4c::backend::bir::Block& source_block,
    const DirectBranchTargets& direct_targets) {
  if (direct_targets.true_block == nullptr || direct_targets.false_block == nullptr ||
      direct_targets.true_block == &source_block ||
      direct_targets.false_block == &source_block) {
    return std::nullopt;
  }

  return ShortCircuitPlan{
      .on_compare_true =
          ShortCircuitTarget{
              .block = direct_targets.true_block,
              .continuation = std::nullopt,
          },
      .on_compare_false =
          ShortCircuitTarget{
              .block = direct_targets.false_block,
              .continuation = std::nullopt,
          },
  };
}

std::optional<DirectBranchTargets> build_plain_cond_direct_branch_targets(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  return resolve_direct_branch_targets(
      find_block,
      c4c::backend::prepare::prepared_block_label(prepared_names, branch_condition.true_label),
      c4c::backend::prepare::prepared_block_label(prepared_names, branch_condition.false_label));
}

std::optional<c4c::backend::prepare::PreparedShortCircuitJoinContext>
find_prepared_short_circuit_join_context_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    std::string_view source_block_label) {
  const c4c::BlockLabelId source_block_label_id = prepared_names.block_labels.find(source_block_label);
  if (source_block_label_id == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  const auto prepared_join_context = c4c::backend::prepare::
      find_prepared_short_circuit_join_context(prepared_names,
                                               control_flow,
                                               function,
                                               source_block_label_id);
  if (!prepared_join_context.has_value() || prepared_join_context->join_transfer == nullptr ||
      prepared_join_context->true_transfer == nullptr ||
      prepared_join_context->false_transfer == nullptr ||
      prepared_join_context->join_block == nullptr) {
    return std::nullopt;
  }
  return prepared_join_context;
}

std::optional<ShortCircuitTarget> build_prepared_short_circuit_target(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedShortCircuitTargetLabels& prepared_target,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  const std::string_view block_label =
      c4c::backend::prepare::prepared_block_label(prepared_names, prepared_target.block_label);
  if (block_label.empty()) {
    return std::nullopt;
  }
  const auto* block = find_block(block_label);
  if (block == nullptr) {
    return std::nullopt;
  }

  std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels> continuation;
  if (prepared_target.continuation.has_value()) {
    continuation = *prepared_target.continuation;
  }

  return ShortCircuitTarget{
      .block = block,
      .continuation = std::move(continuation),
  };
}

std::optional<ShortCircuitPlan> build_prepared_short_circuit_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  const auto on_compare_true =
      build_prepared_short_circuit_target(prepared_names, prepared_plan.on_compare_true, find_block);
  const auto on_compare_false =
      build_prepared_short_circuit_target(prepared_names, prepared_plan.on_compare_false, find_block);
  if (!on_compare_true.has_value() || !on_compare_false.has_value()) {
    return std::nullopt;
  }

  ShortCircuitPlan plan{
      .on_compare_true = *on_compare_true,
      .on_compare_false = *on_compare_false,
  };
  if (plan.on_compare_true.block == nullptr || plan.on_compare_false.block == nullptr) {
    return std::nullopt;
  }
  return plan;
}

CompareDrivenBranchRenderPlan build_compare_driven_render_plan(
    ShortCircuitPlan branch_plan,
    const ShortCircuitEntryCompareContext& compare_context) {
  return CompareDrivenBranchRenderPlan{
      .branch_plan = std::move(branch_plan),
      .compare_setup = compare_context.compare_setup,
      .false_branch_opcode = compare_context.false_branch_opcode,
  };
}

std::optional<CompareDrivenBranchRenderPlan> build_compare_driven_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::bir::BinaryInst& compare,
    bool require_prepared_branch_condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const ShortCircuitEntryCompareContext&)>& branch_plan_builder) {
  const auto* branch_condition =
      find_prepared_branch_condition_if_supported(
          prepared_names, function_control_flow, source_block.label);
  auto compare_context =
      branch_condition != nullptr
          ? render_prepared_guard_false_branch_compare_from_condition(
                *branch_condition, current_materialized_compare, current_i32_name)
          : std::optional<std::pair<std::string, std::string>>{};
  if (!compare_context.has_value() && require_prepared_branch_condition) {
    return std::nullopt;
  }
  if (!compare_context.has_value()) {
    compare_context = render_prepared_guard_false_branch_compare(
        compare, current_materialized_compare, current_i32_name);
  }
  if (!compare_context.has_value()) {
    return std::nullopt;
  }

  ShortCircuitEntryCompareContext entry_compare_context{
      .branch_condition = branch_condition,
      .compare_setup = std::move(compare_context->first),
      .false_branch_opcode = std::move(compare_context->second),
  };
  const auto branch_plan = branch_plan_builder(entry_compare_context);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }

  return build_compare_driven_render_plan(*branch_plan, entry_compare_context);
}

std::optional<CompareDrivenBranchRenderPlan> build_prepared_compare_driven_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Block& source_block,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const ShortCircuitEntryCompareContext&)>& branch_plan_builder) {
  const auto* branch_condition =
      find_prepared_branch_condition_if_supported(
          prepared_names, function_control_flow, source_block.label);
  if (branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto compare_context = build_prepared_guard_compare_context(
      *branch_condition, current_materialized_compare, current_i32_name);
  if (!compare_context.has_value()) {
    return std::nullopt;
  }

  const auto branch_plan = branch_plan_builder(*compare_context);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }

  return build_compare_driven_render_plan(*branch_plan, *compare_context);
}

std::optional<CompareDrivenBranchRenderPlan> build_compare_driven_direct_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::bir::BinaryInst& compare,
    bool require_prepared_branch_condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<DirectBranchTargets>(
        const ShortCircuitEntryCompareContext&)>& direct_target_builder) {
  return build_compare_driven_entry_render_plan(
      prepared_names,
      function_control_flow,
      source_block,
      compare,
      require_prepared_branch_condition,
      current_materialized_compare,
      current_i32_name,
      [&](const ShortCircuitEntryCompareContext& compare_context)
          -> std::optional<ShortCircuitPlan> {
        const auto direct_targets = direct_target_builder(compare_context);
        if (!direct_targets.has_value()) {
          return std::nullopt;
        }
        return build_direct_branch_plan_from_targets(source_block, *direct_targets);
      });
}

std::optional<std::string> render_prepared_param_zero_branch_function(
    std::string_view asm_prefix,
    std::string_view function_name,
    PreparedParamZeroCompareShape compare_shape,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view param_register_name,
    std::string_view true_body,
    std::string_view false_body,
    std::string_view trailing_data) {
  const auto rendered_branch_prefix =
      render_prepared_param_zero_branch_prefix(compare_shape,
                                               function_name,
                                               false_label,
                                               false_branch_opcode,
                                               param_register_name);
  if (!rendered_branch_prefix.has_value()) {
    return std::nullopt;
  }

  const std::string rendered_false_label =
      ".L" + std::string(function_name) + "_" + std::string(false_label);
  return std::string(asm_prefix) + *rendered_branch_prefix + std::string(true_body) +
         rendered_false_label + ":\n" + std::string(false_body) + std::string(trailing_data);
}

std::optional<std::string> find_and_render_prepared_param_zero_branch_return_context_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::bir::Param& param,
    std::string_view asm_prefix,
    std::string_view param_register_name,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Value&)>&
        render_return) {
  const auto prepared_branch_context =
      c4c::backend::prepare::find_prepared_param_zero_branch_return_context(
          prepared_names, function_control_flow, function, entry, param, true);
  if (!prepared_branch_context.has_value()) {
    return std::nullopt;
  }
  if (c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          prepared_names,
          function_control_flow,
          prepared_names.block_labels.find(entry.label))
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared compare-join handoff through the canonical prepared-module handoff");
  }

  const auto& prepared_branch = prepared_branch_context->prepared_branch;
  const auto* true_block = prepared_branch_context->true_block;
  const auto* false_block = prepared_branch_context->false_block;
  const auto& true_value = *true_block->terminator.value;
  const auto& false_value = *false_block->terminator.value;
  const auto true_return = render_return(true_value);
  const auto false_return = render_return(false_value);
  if (!true_return.has_value() || !false_return.has_value()) {
    return std::nullopt;
  }

  return render_prepared_param_zero_branch_function(
      asm_prefix,
      function.name,
      to_prepared_param_zero_compare_shape(prepared_branch.compare_shape),
      prepared_branch.false_label,
      prepared_branch.false_branch_opcode,
      param_register_name,
      *true_return,
      *false_return);
}

std::optional<std::string> render_prepared_minimal_compare_branch_entry_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Value&)>&
        render_return) {
  if (function.params.size() != 1 || prepared_arch != c4c::TargetArch::X86_64 ||
      entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
      param.is_byval) {
    return std::nullopt;
  }
  const auto param_register = minimal_param_register(param);
  if (!param_register.has_value() || function_control_flow == nullptr) {
    return std::nullopt;
  }

  return find_and_render_prepared_param_zero_branch_return_context_if_supported(
      prepared_names,
      *function_control_flow,
      function,
      entry,
      param,
      asm_prefix,
      *param_register,
      render_return);
}

std::optional<std::string>
find_and_render_prepared_materialized_compare_join_function_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::bir::Param& param,
    std::string_view asm_prefix,
    std::string_view param_register_name,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  const auto resolved_render_contract =
      c4c::backend::prepare::find_prepared_param_zero_resolved_materialized_compare_join_render_contract(
          const_cast<c4c::backend::prepare::PreparedNameTables&>(module.names),
          module.module,
          function_control_flow,
          function,
          entry,
          param,
          false);
    if (!resolved_render_contract.has_value()) {
      if (c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            module.names,
            function_control_flow,
            module.names.block_labels.find(entry.label))
            .has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared compare-join handoff through the canonical prepared-module handoff");
    }
    return std::nullopt;
  }

  const auto true_return = render_return(resolved_render_contract->true_return, param);
  const auto false_return = render_return(resolved_render_contract->false_return, param);
  if (!true_return.has_value() || !false_return.has_value()) {
    return std::nullopt;
  }

  std::string rendered_same_module_globals;
  for (const auto* global : resolved_render_contract->same_module_globals) {
    if (global == nullptr) {
      return std::nullopt;
    }
    const auto rendered_global_data = emit_same_module_global_data(*global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_same_module_globals += *rendered_global_data;
  }

  return render_prepared_param_zero_branch_function(
      asm_prefix,
      function.name,
      to_prepared_param_zero_compare_shape(
          resolved_render_contract->branch_plan.compare_shape),
      c4c::backend::prepare::prepared_block_label(
          module.names, resolved_render_contract->branch_plan.target_labels.false_label),
      resolved_render_contract->branch_plan.false_branch_opcode,
      param_register_name,
      *true_return,
      *false_return,
      rendered_same_module_globals);
}

std::optional<std::string> render_prepared_materialized_compare_join_entry_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  if (function.params.size() != 1 || prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
      param.is_byval) {
    return std::nullopt;
  }
  const auto param_register = minimal_param_register(param);
  if (!param_register.has_value() || function_control_flow == nullptr) {
    return std::nullopt;
  }

  return find_and_render_prepared_materialized_compare_join_function_if_supported(
      module,
      *function_control_flow,
      function,
      entry,
      param,
      asm_prefix,
      *param_register,
      render_return,
      emit_same_module_global_data);
}

std::optional<std::string> render_prepared_compare_driven_entry_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Value&)>&
        render_param_derived_return,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_materialized_compare_join_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    if (const auto rendered_branch = render_prepared_minimal_compare_branch_entry_if_supported(
            module.names,
            function_control_flow,
            function,
            entry,
            prepared_arch,
            asm_prefix,
            minimal_param_register,
            render_param_derived_return);
        rendered_branch.has_value()) {
      return rendered_branch;
    }
    return render_prepared_materialized_compare_join_entry_if_supported(
        module,
        function_control_flow,
        function,
        entry,
        prepared_arch,
        asm_prefix,
        minimal_param_register,
        render_materialized_compare_join_return,
        emit_same_module_global_data);
  }

  return std::nullopt;
}

std::optional<CompareDrivenBranchRenderPlan> build_prepared_short_circuit_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::prepare::PreparedShortCircuitJoinContext& join_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const c4c::backend::prepare::PreparedShortCircuitBranchPlan&)>&
        build_short_circuit_plan) {
  (void)function;
  const auto* branch_condition =
      find_prepared_branch_condition_if_supported(
          prepared_names, function_control_flow, source_block.label);
  if (branch_condition == nullptr) {
    return std::nullopt;
  }

  auto compare_context = build_prepared_guard_compare_context(
      *branch_condition, current_materialized_compare, current_i32_name);
  if (!compare_context.has_value()) {
    const std::optional<std::string_view> authoritative_condition_name =
        branch_condition->condition_value.kind == c4c::backend::bir::Value::Kind::Named
            ? std::optional<std::string_view>(branch_condition->condition_value.name)
            : std::nullopt;
    const auto* compare = find_trailing_guard_compare_if_supported(
        source_block, compare_index, authoritative_condition_name);
    if (compare == nullptr) {
      return std::nullopt;
    }
    auto false_branch_compare = render_prepared_guard_false_branch_compare(
        *compare, current_materialized_compare, current_i32_name);
    if (!false_branch_compare.has_value()) {
      return std::nullopt;
    }
    compare_context = ShortCircuitEntryCompareContext{
        .branch_condition = branch_condition,
        .compare_setup = std::move(false_branch_compare->first),
        .false_branch_opcode = std::move(false_branch_compare->second),
    };
  }
  if (!compare_context.has_value()) {
    return std::nullopt;
  }

  const auto prepared_target_labels =
      c4c::backend::prepare::find_prepared_compare_branch_target_labels(
          prepared_names, *branch_condition, source_block);
  if (!prepared_target_labels.has_value()) {
    return std::nullopt;
  }

  const auto prepared_short_circuit_plan =
      c4c::backend::prepare::find_prepared_short_circuit_branch_plan(
          prepared_names, join_context, *prepared_target_labels);
  if (!prepared_short_circuit_plan.has_value()) {
    return std::nullopt;
  }

  const auto short_circuit_branch_plan = build_short_circuit_plan(*prepared_short_circuit_plan);
  if (!short_circuit_branch_plan.has_value()) {
    return std::nullopt;
  }

  return build_compare_driven_render_plan(*short_circuit_branch_plan, *compare_context);
}

std::optional<CompareDrivenBranchRenderPlan> build_prepared_plain_cond_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Block& source_block,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  const auto* branch_condition = find_prepared_i32_immediate_branch_condition_if_supported(
      prepared_names, function_control_flow, source_block.label, current_i32_name);
  if (branch_condition != nullptr) {
    const auto compare_context = build_prepared_guard_compare_context(
        *branch_condition, current_materialized_compare, current_i32_name);
    if (!compare_context.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }

    const auto direct_targets =
        build_plain_cond_direct_branch_targets(prepared_names, find_block, *branch_condition);
    if (!direct_targets.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }

    const auto branch_plan = build_direct_branch_plan_from_targets(source_block, *direct_targets);
    if (!branch_plan.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }

    return build_compare_driven_render_plan(*branch_plan, *compare_context);
  }

  if (source_block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* compare =
      find_trailing_guard_compare_if_supported(source_block,
                                               compare_index,
                                               source_block.terminator.condition.name);
  if (compare == nullptr) {
    return std::nullopt;
  }

  return build_compare_driven_direct_entry_render_plan(
      prepared_names,
      function_control_flow,
      source_block,
      *compare,
      true,
      current_materialized_compare,
      current_i32_name,
      [&](const ShortCircuitEntryCompareContext& compare_context)
          -> std::optional<DirectBranchTargets> {
        if (compare_context.branch_condition == nullptr) {
          return std::nullopt;
        }
        return build_plain_cond_direct_branch_targets(prepared_names,
                                                      find_block,
                                                      *compare_context.branch_condition);
      });
}

std::optional<CompareDrivenBranchRenderPlan> build_prepared_compare_join_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::prepare::PreparedShortCircuitContinuationLabels& continuation_plan,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const c4c::backend::prepare::PreparedShortCircuitBranchPlan&)>&
        build_short_circuit_plan) {
  const auto prepared_branch_plan =
      c4c::backend::prepare::find_prepared_compare_join_entry_branch_plan(
          prepared_names, function_control_flow, function, source_block, continuation_plan);
  if (!prepared_branch_plan.has_value()) {
    return std::nullopt;
  }
  const auto* prepared_branch_condition =
      find_prepared_branch_condition_if_supported(
          prepared_names, function_control_flow, source_block.label);

  if (const auto prepared_render_plan = build_prepared_compare_driven_entry_render_plan(
          prepared_names,
          function_control_flow,
          source_block,
          current_materialized_compare,
          current_i32_name,
          [&](const ShortCircuitEntryCompareContext&) -> std::optional<ShortCircuitPlan> {
            return build_short_circuit_plan(*prepared_branch_plan);
          });
      prepared_render_plan.has_value()) {
    return prepared_render_plan;
  }
  if (prepared_branch_condition != nullptr) {
    return std::nullopt;
  }

  const auto* compare =
      find_trailing_guard_compare_if_supported(source_block, compare_index, std::nullopt);
  if (compare == nullptr) {
    return std::nullopt;
  }

  return build_compare_driven_entry_render_plan(
      prepared_names,
      function_control_flow,
      source_block,
      *compare,
      false,
      current_materialized_compare,
      current_i32_name,
      [&](const ShortCircuitEntryCompareContext&) -> std::optional<ShortCircuitPlan> {
        return build_short_circuit_plan(*prepared_branch_plan);
      });
}

std::optional<std::string> render_compare_driven_branch_plan(
    std::string_view function_name,
    std::string_view rendered_body,
    const CompareDrivenBranchRenderPlan& render_plan,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(const ShortCircuitTarget&, bool)>&
        render_short_circuit_target) {
  struct ShortCircuitRenderContext {
    bool omit_true_continuation = false;
    bool omit_false_lane = false;
  };
  struct RenderedShortCircuitFalseLane {
    std::string label;
    std::string rendered;
  };
  struct RenderedShortCircuitLanes {
    std::string rendered_true;
    RenderedShortCircuitFalseLane rendered_false_lane;
  };

  const auto build_short_circuit_render_context =
      [&](const ShortCircuitPlan& plan) -> std::optional<ShortCircuitRenderContext> {
    if ((plan.on_compare_true.continuation.has_value() ||
         plan.on_compare_false.continuation.has_value()) &&
        prepared_names == nullptr) {
      return std::nullopt;
    }
    const auto resolve_prepared_block =
        [&](c4c::BlockLabelId label_id) -> const c4c::backend::bir::Block* {
      if (prepared_names == nullptr) {
        return nullptr;
      }
      const std::string_view label =
          c4c::backend::prepare::prepared_block_label(*prepared_names, label_id);
      return label.empty() ? nullptr : find_block(label);
    };
    const bool true_target_renders_false_lane =
        plan.on_compare_true.continuation.has_value() &&
        plan.on_compare_false.block ==
            resolve_prepared_block(plan.on_compare_true.continuation->false_label);
    const bool false_target_renders_true_lane =
        plan.on_compare_false.continuation.has_value() &&
        plan.on_compare_true.block ==
            resolve_prepared_block(plan.on_compare_false.continuation->true_label);
    if (true_target_renders_false_lane && false_target_renders_true_lane) {
      return std::nullopt;
    }
    return ShortCircuitRenderContext{
        .omit_true_continuation = false_target_renders_true_lane,
        .omit_false_lane = true_target_renders_false_lane,
    };
  };
  const auto render_short_circuit_false_lane =
      [&](const ShortCircuitPlan& plan,
          const ShortCircuitRenderContext& render_context)
      -> std::optional<RenderedShortCircuitFalseLane> {
    if (plan.on_compare_false.block == nullptr) {
      return std::nullopt;
    }

    RenderedShortCircuitFalseLane rendered_false_lane{
        .label = ".L" + std::string(function_name) + "_" +
                 std::string(plan.on_compare_false.block->label),
        .rendered = {},
    };
    if (render_context.omit_false_lane) {
      return rendered_false_lane;
    }

    const auto rendered_false =
        render_short_circuit_target(plan.on_compare_false, false);
    if (!rendered_false.has_value()) {
      return std::nullopt;
    }
    rendered_false_lane.rendered = rendered_false_lane.label + ":\n" + *rendered_false;
    return rendered_false_lane;
  };
  const auto render_short_circuit_lanes =
      [&](const ShortCircuitPlan& plan,
          const ShortCircuitRenderContext& render_context)
      -> std::optional<RenderedShortCircuitLanes> {
    const auto rendered_true = render_short_circuit_target(
        plan.on_compare_true, render_context.omit_true_continuation);
    if (!rendered_true.has_value()) {
      return std::nullopt;
    }

    const auto rendered_false_lane =
        render_short_circuit_false_lane(plan, render_context);
    if (!rendered_false_lane.has_value()) {
      return std::nullopt;
    }
    return RenderedShortCircuitLanes{
        .rendered_true = *rendered_true,
        .rendered_false_lane = *rendered_false_lane,
    };
  };

  const auto render_context =
      build_short_circuit_render_context(render_plan.branch_plan);
  if (!render_context.has_value()) {
    return std::nullopt;
  }

  const auto rendered_lanes =
      render_short_circuit_lanes(render_plan.branch_plan, *render_context);
  if (!rendered_lanes.has_value()) {
    return std::nullopt;
  }

  std::string rendered = std::string(rendered_body) + render_plan.compare_setup + "    " +
                         render_plan.false_branch_opcode + " " +
                         rendered_lanes->rendered_false_lane.label + "\n" +
                         rendered_lanes->rendered_true;
  if (!rendered_lanes->rendered_false_lane.rendered.empty()) {
    rendered += rendered_lanes->rendered_false_lane.rendered;
  }
  return rendered;
}

std::optional<std::string> render_compare_driven_branch_plan_with_block_renderer(
    std::string_view function_name,
    std::string_view rendered_body,
    const CompareDrivenBranchRenderPlan& render_plan,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block_target) {
  return render_compare_driven_branch_plan(
      function_name,
      rendered_body,
      render_plan,
      prepared_names,
      find_block,
      [&](const ShortCircuitTarget& target,
          bool omit_continuation) -> std::optional<std::string> {
        if (target.block == nullptr) {
          return std::nullopt;
        }
        return render_block_target(*target.block,
                                   omit_continuation ? std::nullopt
                                                     : target.continuation);
      });
}

std::optional<std::string> render_prepared_supported_immediate_binary(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedSupportedImmediateBinary& binary) {
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    return "    add " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub) {
    return "    sub " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    return "    imul " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
    return "    and " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
    return "    or " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    return "    xor " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl) {
    return "    shl " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr) {
    return "    shr " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr) {
    return "    sar " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.immediate)) + "\n";
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_materialized_compare_join_value_if_supported(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
        prepared_return_arm,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load) {
  const auto& computed_value = prepared_return_arm.arm.context.selected_value;
  std::string rendered;
  switch (computed_value.base.kind) {
    case c4c::backend::prepare::PreparedComputedBaseKind::ImmediateI32:
      rendered = "    mov " + std::string(return_register) + ", " +
                 std::to_string(static_cast<std::int32_t>(computed_value.base.immediate)) +
                 "\n";
      break;
    case c4c::backend::prepare::PreparedComputedBaseKind::ParamValue: {
      if (c4c::backend::prepare::prepared_value_name(prepared_names,
                                                     computed_value.base.param_name_id) !=
          param.name) {
        return std::nullopt;
      }
      const auto param_register = minimal_param_register(param);
      if (!param_register.has_value()) {
        return std::nullopt;
      }
      rendered = "    mov " + std::string(return_register) + ", " + *param_register + "\n";
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::GlobalI32Load: {
      if (prepared_return_arm.global == nullptr ||
          !same_module_global_supports_scalar_load(
              *prepared_return_arm.global,
              c4c::backend::bir::TypeKind::I32,
              computed_value.base.global_byte_offset)) {
        return std::nullopt;
      }
      rendered = "    mov " + std::string(return_register) + ", DWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.global->name);
      if (computed_value.base.global_byte_offset != 0) {
        rendered += " + " + std::to_string(computed_value.base.global_byte_offset);
      }
      rendered += "]\n";
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::PointerBackedGlobalI32Load: {
      if (prepared_return_arm.pointer_root_global == nullptr ||
          prepared_return_arm.pointer_root_global->type != c4c::backend::bir::TypeKind::Ptr ||
          prepared_return_arm.global == nullptr ||
          !same_module_global_supports_scalar_load(
              *prepared_return_arm.global,
              c4c::backend::bir::TypeKind::I32,
              computed_value.base.global_byte_offset)) {
        return std::nullopt;
      }
      rendered = "    mov rax, QWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.pointer_root_global->name) +
                 "]\n    mov " + std::string(return_register) + ", DWORD PTR [rip + " +
                 render_asm_symbol_name(prepared_return_arm.global->name);
      if (computed_value.base.global_byte_offset != 0) {
        rendered += " + " + std::to_string(computed_value.base.global_byte_offset);
      }
      rendered += "]\n";
      break;
    }
  }

  for (const auto& operation : computed_value.operations) {
    const auto operation_render =
        render_prepared_supported_immediate_binary(return_register, operation);
    if (!operation_render.has_value()) {
      return std::nullopt;
    }
    rendered += *operation_render;
  }
  return rendered;
}

std::optional<std::string> render_prepared_materialized_compare_join_return_if_supported(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
        prepared_return_arm,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load) {
  const auto value_render = render_prepared_materialized_compare_join_value_if_supported(
      return_register,
      prepared_names,
      prepared_return_arm,
      param,
      minimal_param_register,
      render_asm_symbol_name,
      same_module_global_supports_scalar_load);
  if (!value_render.has_value()) {
    return std::nullopt;
  }

  const auto binary_plan =
      c4c::backend::prepare::find_prepared_materialized_compare_join_return_binary_plan(
          prepared_return_arm.arm);
  if (!binary_plan.has_value()) {
    return std::nullopt;
  }
  if (!binary_plan->trailing_binary.has_value()) {
    return render_prepared_return_body(*value_render);
  }

  const auto trailing_render =
      render_prepared_supported_immediate_binary(return_register, *binary_plan->trailing_binary);
  if (!trailing_render.has_value()) {
    return std::nullopt;
  }
  return render_prepared_return_body(*value_render, *trailing_render);
}

std::string render_prepared_return_body(std::string_view value_render,
                                        std::string_view trailing_render) {
  return std::string(value_render) + std::string(trailing_render) + "    ret\n";
}

}  // namespace c4c::backend::x86
