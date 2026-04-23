#include "prepared_fast_path_dispatch.hpp"

#include "../lowering/memory_lowering.hpp"

namespace c4c::backend::x86 {

namespace {

std::optional<std::string> render_prepared_named_i32_immediate_compare_setup_if_supported(
    std::string_view compared_value_name,
    std::int64_t compare_immediate,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::string> render_prepared_named_i8_immediate_compare_setup_if_supported(
    std::string_view compared_value_name,
    std::int64_t compare_immediate,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

}  // namespace

std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i8_name.has_value() && compare.operand_type == c4c::backend::bir::TypeKind::I8) {
    const auto current_i8_compare =
        [&]() -> std::optional<std::pair<bool, std::int64_t>> {
      if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.lhs.name == *current_i8_name &&
          compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<bool, std::int64_t>{true, compare.rhs.immediate};
      }
      if (compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.rhs.name == *current_i8_name &&
          compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<bool, std::int64_t>{false, compare.lhs.immediate};
      }
      return std::nullopt;
    }();
    if (current_i8_compare.has_value()) {
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
      const char* branch_opcode =
          branch_opcode_for_current_immediate(current_i8_compare->first);
      if (branch_opcode != nullptr) {
        return std::pair<std::string, std::string>{
            "    cmp al, " +
                std::to_string(static_cast<std::int32_t>(
                    static_cast<std::int8_t>(current_i8_compare->second))) +
                "\n",
            branch_opcode,
        };
      }
    }
  }

  if (current_materialized_compare.has_value() &&
      current_materialized_compare->i32_name.has_value()) {
    const auto selected_compare =
        select_prepared_i32_named_immediate_compare_for_value_if_supported(
            compare.lhs, compare.rhs, *current_materialized_compare->i32_name);
    if (selected_compare.has_value() && selected_compare->immediate == 0) {
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
    // Fall through to authoritative prepared-home materialization.
  }
  if (compare.operand_type == c4c::backend::bir::TypeKind::I8) {
    const auto select_i8_named_immediate_compare =
        [&]() -> std::optional<std::pair<const c4c::backend::bir::Value*, std::int64_t>> {
      if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<const c4c::backend::bir::Value*, std::int64_t>{&compare.lhs,
                                                                         compare.rhs.immediate};
      }
      if (compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<const c4c::backend::bir::Value*, std::int64_t>{&compare.rhs,
                                                                         compare.lhs.immediate};
      }
      return std::nullopt;
    }();
    if (!select_i8_named_immediate_compare.has_value()) {
      return std::nullopt;
    }
    const bool current_is_lhs = select_i8_named_immediate_compare->first == &compare.lhs;
    const auto compare_setup = render_prepared_named_i8_immediate_compare_setup_if_supported(
        select_i8_named_immediate_compare->first->name,
        select_i8_named_immediate_compare->second,
        prepared_names,
        function_locations);
    if (!compare_setup.has_value()) {
      return std::nullopt;
    }
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
    const char* branch_opcode = branch_opcode_for_current_immediate(current_is_lhs);
    if (branch_opcode == nullptr) {
      return std::nullopt;
    }
    return std::pair<std::string, std::string>{*compare_setup, std::string(branch_opcode)};
  }
  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_if_supported(compare.lhs, compare.rhs);
  if (!selected_compare.has_value()) {
    return std::nullopt;
  }
  const bool current_is_lhs = selected_compare->named_value == &compare.lhs;
  const auto compare_setup = render_prepared_named_i32_immediate_compare_setup_if_supported(
      selected_compare->named_value->name,
      selected_compare->immediate,
      current_i32_name,
      prepared_names,
      function_locations);
  if (!compare_setup.has_value()) {
    return std::nullopt;
  }
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
  const char* branch_opcode = branch_opcode_for_current_immediate(current_is_lhs);
  if (branch_opcode == nullptr) {
    return std::nullopt;
  }
  return std::pair<std::string, std::string>{*compare_setup, std::string(branch_opcode)};
}

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_from_condition(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
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
      compare,
      current_i8_name,
      current_materialized_compare,
      current_i32_name,
      prepared_names,
      function_locations);
}

std::optional<ShortCircuitEntryCompareContext> build_prepared_guard_compare_context(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  auto false_branch_compare = render_prepared_guard_false_branch_compare_from_condition(
      branch_condition,
      current_i8_name,
      current_materialized_compare,
      current_i32_name,
      prepared_names,
      function_locations);
  if (!false_branch_compare.has_value()) {
    return std::nullopt;
  }
  return ShortCircuitEntryCompareContext{
      .branch_condition = &branch_condition,
      .compare_setup = std::move(false_branch_compare->first),
      .false_branch_opcode = std::move(false_branch_compare->second),
  };
}

namespace {

std::optional<CompareDrivenBranchRenderPlan> build_prepared_compare_driven_branch_render_plan(
    const std::optional<std::pair<std::string, std::string>>& compare_context,
    const std::optional<ShortCircuitPlan>& short_circuit_plan) {
  if (!compare_context.has_value() || !short_circuit_plan.has_value()) {
    return std::nullopt;
  }
  return CompareDrivenBranchRenderPlan{
      .branch_plan = *short_circuit_plan,
      .compare_setup = compare_context->first,
      .false_branch_opcode = compare_context->second,
  };
}

std::optional<c4c::backend::bir::BinaryInst>
build_prepared_authoritative_branch_compare_if_supported(
    const c4c::backend::prepare::PreparedBranchCondition* prepared_branch_condition) {
  if (prepared_branch_condition == nullptr || !prepared_branch_condition->predicate.has_value() ||
      !prepared_branch_condition->lhs.has_value() || !prepared_branch_condition->rhs.has_value()) {
    return std::nullopt;
  }
  return c4c::backend::bir::BinaryInst{
      .opcode = *prepared_branch_condition->predicate,
      .result = prepared_branch_condition->condition_value,
      .operand_type =
          prepared_branch_condition->compare_type.value_or(c4c::backend::bir::TypeKind::I32),
      .lhs = *prepared_branch_condition->lhs,
      .rhs = *prepared_branch_condition->rhs,
  };
}

}  // namespace

std::optional<CompareDrivenBranchRenderPlan>
build_prepared_short_circuit_cond_branch_render_plan_if_supported(
    const PreparedX86FunctionDispatchContext& function_context,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::prepare::PreparedShortCircuitJoinContext& join_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (!current_i8_name.has_value() || compare_index >= block.insts.size()) {
    return std::nullopt;
  }

  const auto* function_control_flow = function_context.function_control_flow;
  const auto* prepared_names = function_context.prepared_names;
  if (function_control_flow == nullptr || prepared_names == nullptr) {
    return std::nullopt;
  }

  (void)function;
  const auto* source_branch_condition =
      c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow, block_label_id);
  const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[compare_index]);
  const auto compare_context =
      compare != nullptr && compare->operand_type == c4c::backend::bir::TypeKind::I8 &&
              source_branch_condition != nullptr
          ? render_prepared_guard_false_branch_compare(
                *compare,
                current_i8_name,
                current_materialized_compare,
                current_i32_name,
                prepared_names,
                function_context.function_locations)
          : std::nullopt;
  const auto prepared_target_labels =
      compare_context.has_value() && source_branch_condition != nullptr
          ? c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
                *prepared_names, function_control_flow, block_label_id, block, *source_branch_condition)
          : std::nullopt;
  const auto prepared_short_circuit_plan =
      prepared_target_labels.has_value()
          ? c4c::backend::prepare::find_prepared_short_circuit_branch_plan(
                *prepared_names, join_context, *prepared_target_labels)
          : std::nullopt;
  const auto short_circuit_plan =
      prepared_short_circuit_plan.has_value()
          ? c4c::backend::x86::build_prepared_short_circuit_plan(
                *prepared_names, *prepared_short_circuit_plan, find_block)
          : std::nullopt;
  return build_prepared_compare_driven_branch_render_plan(compare_context, short_circuit_plan);
}

std::optional<CompareDrivenBranchRenderPlan>
build_prepared_compare_join_fallback_render_plan_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    const c4c::backend::prepare::PreparedShortCircuitContinuationLabels& continuation,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (compare_index >= block.insts.size() || block_context.function_context == nullptr ||
      block_context.function_context->prepared_names == nullptr) {
    return std::nullopt;
  }

  const auto& function_context = *block_context.function_context;
  const auto* prepared_names = function_context.prepared_names;
  const auto prepared_branch_plan =
      c4c::backend::prepare::find_prepared_compare_join_entry_branch_plan(
          *prepared_names, function_context.function_control_flow, function, block, continuation);
  const auto short_circuit_plan =
      prepared_branch_plan.has_value()
          ? c4c::backend::x86::build_prepared_short_circuit_plan(
                *prepared_names, *prepared_branch_plan, find_block)
          : std::nullopt;
  const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[compare_index]);
  const auto* prepared_branch_condition =
      function_context.function_control_flow != nullptr
          ? c4c::backend::prepare::find_prepared_branch_condition(
                *function_context.function_control_flow, block_context.block_label_id)
          : nullptr;
  const auto authoritative_compare =
      build_prepared_authoritative_branch_compare_if_supported(prepared_branch_condition);
  const auto compare_context =
      authoritative_compare.has_value()
          ? render_prepared_guard_false_branch_compare(
                *authoritative_compare,
                current_i8_name,
                current_materialized_compare,
                current_i32_name,
                prepared_names,
                function_context.function_locations)
          : prepared_branch_condition != nullptr
                ? std::nullopt
                : compare != nullptr
                      ? render_prepared_guard_false_branch_compare(
                            *compare,
                            current_i8_name,
                            current_materialized_compare,
                            current_i32_name,
                            prepared_names,
                            function_context.function_locations)
                      : std::nullopt;
  return build_prepared_compare_driven_branch_render_plan(compare_context, short_circuit_plan);
}

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

namespace {

bool prepared_i32_compare_value_name_matches_any_candidate(
    std::string_view compared_value_name,
    const std::vector<std::string_view>& candidate_compare_value_names) {
  return std::find(candidate_compare_value_names.begin(),
                   candidate_compare_value_names.end(),
                   compared_value_name) != candidate_compare_value_names.end();
}

std::optional<std::string> render_prepared_named_i32_immediate_compare_setup_if_supported(
    std::string_view compared_value_name,
    std::int64_t compare_immediate,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32_name.has_value() && *current_i32_name == compared_value_name) {
    return render_prepared_i32_eax_immediate_compare_setup(compare_immediate);
  }
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }

  const auto* home = c4c::backend::prepare::find_prepared_value_home(
      *prepared_names, *function_locations, compared_value_name);
  if (home == nullptr) {
    return std::nullopt;
  }

  std::string setup;
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto narrowed_register = narrow_i32_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return std::nullopt;
    }
    if (*narrowed_register != "eax") {
      setup += "    mov eax, " + *narrowed_register + "\n";
    }
  } else if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
             home->offset_bytes.has_value()) {
    setup += "    mov eax, " +
             render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD") + "\n";
  } else if (home->kind ==
                 c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
             home->immediate_i32.has_value()) {
    setup += "    mov eax, " +
             std::to_string(static_cast<std::int32_t>(*home->immediate_i32)) + "\n";
  } else {
    return std::nullopt;
  }

  setup += render_prepared_i32_eax_immediate_compare_setup(compare_immediate);
  return setup;
}

std::optional<std::string> render_prepared_named_i8_immediate_compare_setup_if_supported(
    std::string_view compared_value_name,
    std::int64_t compare_immediate,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }

  const auto* home = c4c::backend::prepare::find_prepared_value_home(
      *prepared_names, *function_locations, compared_value_name);
  if (home == nullptr) {
    return std::nullopt;
  }

  const auto immediate_i8 = static_cast<std::int32_t>(static_cast<std::int8_t>(compare_immediate));
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto narrowed_register = narrow_i8_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return std::nullopt;
    }
    return "    cmp " + *narrowed_register + ", " + std::to_string(immediate_i8) + "\n";
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return "    cmp " +
           render_prepared_stack_memory_operand(*home->offset_bytes, "BYTE") + ", " +
           std::to_string(immediate_i8) + "\n";
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return "    mov eax, " +
           std::to_string(static_cast<std::int32_t>(*home->immediate_i32)) +
           "\n    cmp al, " + std::to_string(immediate_i8) + "\n";
  }
  return std::nullopt;
}

}  // namespace

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

}  // namespace c4c::backend::x86
