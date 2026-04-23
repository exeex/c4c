#include "prepared_fast_path_dispatch.hpp"

#include "../lowering/memory_lowering.hpp"

namespace c4c::backend::x86 {

namespace {

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

std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
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
    return std::pair<std::string, std::string>{*compare_setup, branch_opcode};
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
  return std::pair<std::string, std::string>{*compare_setup, branch_opcode};
}

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_from_condition(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
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
      compare, current_materialized_compare, current_i32_name, prepared_names, function_locations);
}

std::optional<ShortCircuitEntryCompareContext> build_prepared_guard_compare_context(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  auto false_branch_compare = render_prepared_guard_false_branch_compare_from_condition(
      branch_condition,
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

}  // namespace c4c::backend::x86
