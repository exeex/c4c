#include "inline_asm.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] std::optional<ValueNameId> maybe_inline_asm_named_value_id(
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const ValueNameId id = names.value_names.find(value.name);
  if (id == kInvalidValueName) {
    return std::nullopt;
  }
  return id;
}

[[nodiscard]] std::optional<PreparedValueHome> prepared_inline_asm_home_for_named_value(
    const PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& value) {
  if (value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = maybe_inline_asm_named_value_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, *value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  return *home;
}

void append_inline_asm_missing_fact(PreparedInlineAsmCarrierFunction& function_carriers,
                                    PreparedInlineAsmCarrier& carrier,
                                    std::string fact) {
  carrier.missing_required_facts.push_back(fact);
  function_carriers.missing_required_facts.push_back(
      "inst#" + std::to_string(carrier.inst_index) + ":" + std::move(fact));
}

[[nodiscard]] std::optional<ValueNameId> prepared_inline_asm_named_value_id(
    PreparedNameTables& names,
    const std::optional<bir::Value>& value);

[[nodiscard]] std::optional<std::size_t> parse_inline_asm_register_index(
    std::string_view digits,
    std::size_t max_index) {
  if (digits.empty()) {
    return std::nullopt;
  }
  std::size_t value = 0;
  const char* const begin = digits.data();
  const char* const end = digits.data() + digits.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  if (ec != std::errc{} || ptr != end || value > max_index) {
    return std::nullopt;
  }
  return value;
}

[[nodiscard]] std::optional<PreparedTargetRegisterIdentity>
aarch64_inline_asm_register_identity(std::string_view register_name) {
  if (register_name.size() < 2) {
    return std::nullopt;
  }
  const char prefix = register_name.front();
  const std::string_view digits = register_name.substr(1);
  switch (prefix) {
    case 'x':
    case 'w':
      if (const auto index = parse_inline_asm_register_index(digits, 30);
          index.has_value()) {
        return PreparedTargetRegisterIdentity{
            .target_arch = c4c::TargetArch::Aarch64,
            .bank = PreparedRegisterBank::Gpr,
            .register_class = PreparedRegisterClass::General,
            .physical_index = *index,
        };
      }
      return std::nullopt;
    case 's':
    case 'd':
      if (const auto index = parse_inline_asm_register_index(digits, 31);
          index.has_value()) {
        return PreparedTargetRegisterIdentity{
            .target_arch = c4c::TargetArch::Aarch64,
            .bank = PreparedRegisterBank::Fpr,
            .register_class = PreparedRegisterClass::Float,
            .physical_index = *index,
        };
      }
      return std::nullopt;
    case 'q':
    case 'v':
      if (const auto index = parse_inline_asm_register_index(digits, 31);
          index.has_value()) {
        return PreparedTargetRegisterIdentity{
            .target_arch = c4c::TargetArch::Aarch64,
            .bank = PreparedRegisterBank::Vreg,
            .register_class = PreparedRegisterClass::Vector,
            .physical_index = *index,
        };
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] PreparedTargetRegisterIdentity rv64_gpr_identity(std::size_t physical_index) {
  return PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Riscv64,
      .bank = PreparedRegisterBank::Gpr,
      .register_class = PreparedRegisterClass::General,
      .physical_index = physical_index,
  };
}

[[nodiscard]] PreparedTargetRegisterIdentity rv64_vector_identity(std::size_t physical_index) {
  return PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Riscv64,
      .bank = PreparedRegisterBank::Vreg,
      .register_class = PreparedRegisterClass::Vector,
      .physical_index = physical_index,
  };
}

[[nodiscard]] std::optional<PreparedTargetRegisterIdentity>
rv64_inline_asm_register_identity(std::string_view register_name) {
  if (register_name.size() >= 2 && register_name.front() == 'x') {
    const auto index = parse_inline_asm_register_index(register_name.substr(1), 31);
    if (index.has_value()) {
      return rv64_gpr_identity(*index);
    }
    return std::nullopt;
  }
  if (register_name.size() >= 2 && register_name.front() == 'v') {
    const auto index = parse_inline_asm_register_index(register_name.substr(1), 31);
    if (index.has_value()) {
      return rv64_vector_identity(*index);
    }
    return std::nullopt;
  }

  if (register_name == "zero") return rv64_gpr_identity(0);
  if (register_name == "ra") return rv64_gpr_identity(1);
  if (register_name == "sp") return rv64_gpr_identity(2);
  if (register_name == "gp") return rv64_gpr_identity(3);
  if (register_name == "tp") return rv64_gpr_identity(4);
  if (register_name == "t0") return rv64_gpr_identity(5);
  if (register_name == "t1") return rv64_gpr_identity(6);
  if (register_name == "t2") return rv64_gpr_identity(7);
  if (register_name == "s0" || register_name == "fp") return rv64_gpr_identity(8);
  if (register_name == "s1") return rv64_gpr_identity(9);
  if (register_name == "a0") return rv64_gpr_identity(10);
  if (register_name == "a1") return rv64_gpr_identity(11);
  if (register_name == "a2") return rv64_gpr_identity(12);
  if (register_name == "a3") return rv64_gpr_identity(13);
  if (register_name == "a4") return rv64_gpr_identity(14);
  if (register_name == "a5") return rv64_gpr_identity(15);
  if (register_name == "a6") return rv64_gpr_identity(16);
  if (register_name == "a7") return rv64_gpr_identity(17);
  if (register_name == "s2") return rv64_gpr_identity(18);
  if (register_name == "s3") return rv64_gpr_identity(19);
  if (register_name == "s4") return rv64_gpr_identity(20);
  if (register_name == "s5") return rv64_gpr_identity(21);
  if (register_name == "s6") return rv64_gpr_identity(22);
  if (register_name == "s7") return rv64_gpr_identity(23);
  if (register_name == "s8") return rv64_gpr_identity(24);
  if (register_name == "s9") return rv64_gpr_identity(25);
  if (register_name == "s10") return rv64_gpr_identity(26);
  if (register_name == "s11") return rv64_gpr_identity(27);
  if (register_name == "t3") return rv64_gpr_identity(28);
  if (register_name == "t4") return rv64_gpr_identity(29);
  if (register_name == "t5") return rv64_gpr_identity(30);
  if (register_name == "t6") return rv64_gpr_identity(31);
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedTargetRegisterIdentity>
inline_asm_register_identity(const c4c::TargetProfile& target_profile,
                             std::string_view register_name) {
  switch (target_profile.arch) {
    case c4c::TargetArch::Aarch64:
      return aarch64_inline_asm_register_identity(register_name);
    case c4c::TargetArch::Riscv64:
      return rv64_inline_asm_register_identity(register_name);
    case c4c::TargetArch::Unknown:
    case c4c::TargetArch::X86_64:
    case c4c::TargetArch::I686:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool inline_asm_identity_matches_register_constraint(
    const PreparedTargetRegisterIdentity& identity,
    bir::InlineAsmRegisterClass register_class) {
  switch (register_class) {
    case bir::InlineAsmRegisterClass::General:
      return identity.bank == PreparedRegisterBank::Gpr &&
             identity.register_class == PreparedRegisterClass::General;
    case bir::InlineAsmRegisterClass::Vector:
      return identity.bank == PreparedRegisterBank::Vreg &&
             identity.register_class == PreparedRegisterClass::Vector;
    case bir::InlineAsmRegisterClass::None:
      return true;
  }
  return false;
}

[[nodiscard]] PreparedRegisterClass prepared_register_class_from_inline_asm(
    bir::InlineAsmRegisterClass register_class) {
  switch (register_class) {
    case bir::InlineAsmRegisterClass::General:
      return PreparedRegisterClass::General;
    case bir::InlineAsmRegisterClass::Vector:
      return PreparedRegisterClass::Vector;
    case bir::InlineAsmRegisterClass::None:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] bool inline_asm_constraint_accepts_value_type(
    bir::InlineAsmRegisterClass register_class,
    std::size_t register_group_width,
    bir::TypeKind value_type) {
  if (register_class != bir::InlineAsmRegisterClass::Vector) {
    return true;
  }
  const std::size_t value_width = bir::vrm_register_group_width(value_type);
  if (value_width == 0) {
    return false;
  }
  return value_width == std::max<std::size_t>(register_group_width, 1);
}

[[nodiscard]] bir::InlineAsmRegisterClass tied_output_register_class(
    const std::vector<PreparedInlineAsmOperand>& operands,
    std::size_t tied_output_index) {
  for (const auto& operand : operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        operand.output_index.has_value() && *operand.output_index == tied_output_index) {
      return operand.register_class;
    }
  }
  return bir::InlineAsmRegisterClass::None;
}

void append_inline_asm_register_group_override(PreparedBirModule& prepared,
                                               FunctionNameId function_name,
                                               ValueNameId value_name,
                                               bir::InlineAsmRegisterClass register_class,
                                               std::size_t register_group_width,
                                               bir::TypeKind value_type) {
  const auto prepared_class = prepared_register_class_from_inline_asm(register_class);
  if (function_name == kInvalidFunctionName || value_name == kInvalidValueName ||
      prepared_class == PreparedRegisterClass::None) {
    return;
  }
  if (!inline_asm_constraint_accepts_value_type(
          register_class, register_group_width, value_type)) {
    return;
  }
  if (const auto* existing =
          find_prepared_register_group_override(prepared, function_name, value_name);
      existing != nullptr) {
    return;
  }
  prepared.register_group_overrides.values.push_back(PreparedRegisterGroupOverride{
      .function_name = function_name,
      .value_name = value_name,
      .register_class = prepared_class,
      .contiguous_width = register_group_width == 0 ? 1 : register_group_width,
  });
}

void publish_inline_asm_register_group_overrides(PreparedBirModule& prepared,
                                                 const PreparedInlineAsmCarrier& carrier) {
  for (const auto& operand : carrier.operands) {
    if (operand.value_name.has_value()) {
      append_inline_asm_register_group_override(prepared,
                                                carrier.function_name,
                                                *operand.value_name,
                                                operand.register_class,
                                                operand.register_group_width,
                                                operand.value->type);
    }
  }
}

void publish_inline_asm_metadata_register_group_overrides(PreparedBirModule& prepared,
                                                          FunctionNameId function_name,
                                                          const bir::CallInst& call) {
  if (!call.inline_asm.has_value()) {
    return;
  }
  for (const auto& operand : call.inline_asm->operands) {
    if (operand.arg_index.has_value() && *operand.arg_index < call.args.size()) {
      if (const auto value_name = prepared_inline_asm_named_value_id(
              prepared.names, call.args[*operand.arg_index]);
          value_name.has_value()) {
        append_inline_asm_register_group_override(prepared,
                                                  function_name,
                                                  *value_name,
                                                  operand.register_class,
                                                  operand.register_group_width,
                                                  call.args[*operand.arg_index].type);
      }
    }
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        call.result.has_value()) {
      if (const auto value_name =
              prepared_inline_asm_named_value_id(prepared.names, call.result);
          value_name.has_value()) {
        append_inline_asm_register_group_override(prepared,
                                                  function_name,
                                                  *value_name,
                                                  operand.register_class,
                                                  operand.register_group_width,
                                                  call.result->type);
      }
    }
  }
}

void populate_inline_asm_home_identity(const c4c::TargetProfile& target_profile,
                                       std::optional<PreparedValueHome>& home) {
  if (!home.has_value() || home->kind != PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return;
  }
  home->target_register_identity =
      inline_asm_register_identity(target_profile, *home->register_name);
}

[[nodiscard]] std::optional<ValueNameId> prepared_inline_asm_named_value_id(
    PreparedNameTables& names,
    const std::optional<bir::Value>& value) {
  if (!value.has_value()) {
    return std::nullopt;
  }
  return prepared_named_value_id(names, *value);
}

[[nodiscard]] bool is_inline_asm_integer_immediate_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return true;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::Void:
    case bir::TypeKind::Vrm1:
    case bir::TypeKind::Vrm2:
    case bir::TypeKind::Vrm4:
    case bir::TypeKind::Vrm8:
      return false;
  }
  return false;
}

[[nodiscard]] std::optional<std::int64_t> inline_asm_integer_immediate_value(
    const bir::Value& value,
    const std::optional<PreparedValueHome>& home) {
  if (value.kind == bir::Value::Kind::Immediate &&
      is_inline_asm_integer_immediate_type(value.type)) {
    return value.immediate;
  }
  if (home.has_value() &&
      home->kind == PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return *home->immediate_i32;
  }
  return std::nullopt;
}

[[nodiscard]] bool inline_asm_pointer_address_is_prepared_selectable(
    const PreparedInlineAsmOperand& operand,
    const bir::MemoryAddress& address) {
  return operand.value.has_value() &&
         address.base_kind == bir::MemoryAddress::BaseKind::PointerValue &&
         address.base_value == *operand.value && operand.home.has_value() &&
         operand.home->kind == PreparedValueHomeKind::Register &&
         operand.home->register_name.has_value();
}

[[nodiscard]] PreparedInlineAsmOperand make_prepared_inline_asm_operand(
    PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::CallInst& call,
    const bir::InlineAsmOperandMetadata& metadata) {
  PreparedInlineAsmOperand operand{
      .kind = metadata.kind,
      .constraint_index = metadata.constraint_index,
      .constraint = metadata.constraint,
      .arg_index = metadata.arg_index,
      .output_index = metadata.output_index,
      .tied_output_index = metadata.tied_output_index,
      .register_class = metadata.register_class,
      .register_group_width = metadata.register_group_width,
      .value = std::nullopt,
      .value_name = std::nullopt,
      .home = std::nullopt,
      .immediate_value = std::nullopt,
      .name = metadata.name,
      .memory_address = metadata.memory_address,
      .address = metadata.address,
  };
  if (metadata.arg_index.has_value() && *metadata.arg_index < call.args.size()) {
    operand.value = call.args[*metadata.arg_index];
    operand.value_name = prepared_inline_asm_named_value_id(names, operand.value);
    operand.home =
        prepared_inline_asm_home_for_named_value(names, value_locations, *operand.value);
    if (metadata.kind == bir::InlineAsmOperandKind::IntegerImmediateInput) {
      operand.immediate_value =
          inline_asm_integer_immediate_value(*operand.value, operand.home);
    }
  }
  return operand;
}

[[nodiscard]] std::size_t inline_asm_input_operand_count(
    const std::vector<PreparedInlineAsmOperand>& operands) {
  std::size_t count = 0;
  for (const auto& operand : operands) {
    if (operand.arg_index.has_value()) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] std::size_t inline_asm_output_operand_count(
    const std::vector<PreparedInlineAsmOperand>& operands) {
  std::size_t count = 0;
  for (const auto& operand : operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput) {
      ++count;
    }
  }
  return count;
}

void validate_inline_asm_carrier(PreparedInlineAsmCarrierFunction& function_carriers,
                                 PreparedInlineAsmCarrier& carrier,
                                 const bir::CallInst& call,
                                 const bir::InlineAsmMetadata& inline_asm) {
  for (const auto& fact : inline_asm.unsupported_facts) {
    append_inline_asm_missing_fact(function_carriers, carrier, fact);
  }

  const std::size_t output_count = inline_asm_output_operand_count(carrier.operands);
  if (call.result.has_value() && output_count == 0) {
    append_inline_asm_missing_fact(function_carriers, carrier, "missing_output_constraint");
  }
  if (!call.result.has_value() && output_count != 0) {
    append_inline_asm_missing_fact(function_carriers, carrier, "output_constraint_requires_result");
  }
  if (output_count > 1) {
    append_inline_asm_missing_fact(function_carriers, carrier, "unsupported_multiple_outputs");
  }
  if (call.args.size() != inline_asm_input_operand_count(carrier.operands)) {
    append_inline_asm_missing_fact(function_carriers, carrier, "constraint_operand_count_mismatch");
  }

  for (auto& operand : carrier.operands) {
    switch (operand.kind) {
      case bir::InlineAsmOperandKind::RegisterOutput:
        if (!call.result.has_value()) {
          break;
        }
        if (!carrier.result_home.has_value()) {
          append_inline_asm_missing_fact(function_carriers, carrier, "missing_result_home");
        } else if (carrier.result_home->kind != PreparedValueHomeKind::Register) {
          append_inline_asm_missing_fact(function_carriers, carrier, "result_requires_register_home");
        }
        if (!operand.output_index.has_value() || *operand.output_index != 0) {
          append_inline_asm_missing_fact(function_carriers, carrier, "unsupported_output_index");
        }
        if (carrier.result_home.has_value() &&
            carrier.result_home->target_register_identity.has_value() &&
            !inline_asm_identity_matches_register_constraint(
                *carrier.result_home->target_register_identity, operand.register_class)) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "result_home_incompatible_register_class");
        }
        break;
      case bir::InlineAsmOperandKind::RegisterInput:
        if (!operand.arg_index.has_value() || !operand.value.has_value()) {
          append_inline_asm_missing_fact(
              function_carriers,
              carrier,
              "missing_operand" +
                  std::to_string(operand.arg_index.value_or(call.args.size())) + "_value");
          break;
        }
        if (!operand.home.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "missing_operand" +
                                             std::to_string(*operand.arg_index) + "_home");
        } else if (operand.home->kind != PreparedValueHomeKind::Register) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "operand" + std::to_string(*operand.arg_index) +
                                             "_requires_register_home");
        } else if (operand.home->target_register_identity.has_value() &&
                   !inline_asm_identity_matches_register_constraint(
                       *operand.home->target_register_identity, operand.register_class)) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "operand" + std::to_string(*operand.arg_index) +
                                             "_home_incompatible_register_class");
        }
        break;
      case bir::InlineAsmOperandKind::TiedInput:
        if (!operand.arg_index.has_value() || !operand.value.has_value()) {
          append_inline_asm_missing_fact(
              function_carriers,
              carrier,
              "missing_operand" +
                  std::to_string(operand.arg_index.value_or(call.args.size())) + "_value");
          break;
        }
        if (!operand.tied_output_index.has_value() ||
            *operand.tied_output_index >= output_count) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "malformed_tied_operand" +
                                             std::to_string(*operand.arg_index));
        }
        if (!operand.home.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "missing_operand" +
                                             std::to_string(*operand.arg_index) + "_home");
        } else if (operand.home->kind != PreparedValueHomeKind::Register) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "operand" + std::to_string(*operand.arg_index) +
                                             "_requires_register_home");
        }
        if (!carrier.result_home.has_value()) {
          append_inline_asm_missing_fact(function_carriers, carrier, "missing_result_home");
        } else if (carrier.result_home->kind != PreparedValueHomeKind::Register) {
          append_inline_asm_missing_fact(function_carriers, carrier, "result_requires_register_home");
        } else if (!carrier.result_home->register_name.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "result_requires_concrete_register_home");
        }
        if (operand.home.has_value() &&
            operand.home->kind == PreparedValueHomeKind::Register &&
            !operand.home->register_name.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "operand" + std::to_string(*operand.arg_index) +
                                             "_requires_concrete_register_home");
        }
        if (operand.home.has_value() && carrier.result_home.has_value() &&
            operand.home->kind == PreparedValueHomeKind::Register &&
            carrier.result_home->kind == PreparedValueHomeKind::Register &&
            operand.home->register_name.has_value() &&
            carrier.result_home->register_name.has_value()) {
          const auto tied_identity = operand.home->target_register_identity;
          if (!tied_identity.has_value()) {
            append_inline_asm_missing_fact(function_carriers,
                                           carrier,
                                           "target_invalid_tied_input_register_home");
          }
          const auto output_identity = carrier.result_home->target_register_identity;
          if (!output_identity.has_value()) {
            append_inline_asm_missing_fact(function_carriers,
                                           carrier,
                                           "target_invalid_tied_output_register_home");
          }
          if (tied_identity.has_value() && output_identity.has_value()) {
            const auto expected_class =
                operand.tied_output_index.has_value()
                    ? tied_output_register_class(carrier.operands, *operand.tied_output_index)
                    : operand.register_class;
            if (!inline_asm_identity_matches_register_constraint(*tied_identity, expected_class) ||
                !inline_asm_identity_matches_register_constraint(*output_identity, expected_class)) {
              append_inline_asm_missing_fact(
                  function_carriers,
                  carrier,
                  "tied_input_output_home_incompatible_register_class");
            } else if (*tied_identity != *output_identity) {
              append_inline_asm_missing_fact(function_carriers,
                                             carrier,
                                             "tied_input_output_home_mismatch");
            } else if (operand.tied_output_index.has_value()) {
              operand.tied_home_authority = PreparedInlineAsmTiedHomeAuthority{
                  .tied_output_index = *operand.tied_output_index,
                  .shared_register = *tied_identity,
              };
            }
          }
        }
        break;
      case bir::InlineAsmOperandKind::IntegerImmediateInput:
        if (!operand.arg_index.has_value() || !operand.value.has_value()) {
          append_inline_asm_missing_fact(
              function_carriers,
              carrier,
              "missing_operand" +
                  std::to_string(operand.arg_index.value_or(call.args.size())) + "_value");
          break;
        }
        if (!operand.immediate_value.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "operand" + std::to_string(*operand.arg_index) +
                                             "_requires_integer_immediate");
        }
        break;
      case bir::InlineAsmOperandKind::MemoryInput:
        if (!operand.arg_index.has_value() || !operand.value.has_value()) {
          append_inline_asm_missing_fact(
              function_carriers,
              carrier,
              "missing_operand" +
                  std::to_string(operand.arg_index.value_or(call.args.size())) + "_value");
          break;
        }
        if (!operand.memory_address.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "missing_operand" +
                                             std::to_string(*operand.arg_index) +
                                             "_memory_address_authority");
        } else if (!inline_asm_pointer_address_is_prepared_selectable(
                       operand, *operand.memory_address)) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "unsupported_operand" +
                                             std::to_string(*operand.arg_index) +
                                             "_memory_address_selection");
        }
        break;
      case bir::InlineAsmOperandKind::AddressInput:
        if (!operand.arg_index.has_value() || !operand.value.has_value()) {
          append_inline_asm_missing_fact(
              function_carriers,
              carrier,
              "missing_operand" +
                  std::to_string(operand.arg_index.value_or(call.args.size())) + "_value");
          break;
        }
        if (!operand.address.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "missing_operand" +
                                             std::to_string(*operand.arg_index) +
                                             "_address_authority");
        } else if (!inline_asm_pointer_address_is_prepared_selectable(
                       operand, *operand.address)) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "unsupported_operand" +
                                             std::to_string(*operand.arg_index) +
                                             "_address_selection");
        }
        break;
      case bir::InlineAsmOperandKind::Clobber:
        if (!operand.name.has_value()) {
          append_inline_asm_missing_fact(function_carriers,
                                         carrier,
                                         "unsupported_clobber_operand" +
                                             std::to_string(operand.constraint_index));
        }
        break;
      case bir::InlineAsmOperandKind::Unsupported:
        append_inline_asm_missing_fact(function_carriers,
                                       carrier,
                                       "unsupported_operand_constraint" +
                                           std::to_string(operand.constraint_index));
        break;
    }
  }
}

[[nodiscard]] PreparedInlineAsmCarrier build_inline_asm_carrier(
    PreparedNameTables& names,
    PreparedInlineAsmCarrierFunction& function_carriers,
    const bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::TargetProfile& target_profile,
    const PreparedValueLocationFunction* value_locations) {
  const bir::InlineAsmMetadata inline_asm =
      call.inline_asm.value_or(bir::InlineAsmMetadata{});
  PreparedInlineAsmCarrier carrier{
      .function_name = function_carriers.function_name,
      .carrier_kind = PreparedInlineAsmCarrierKind::Missing,
      .block_index = block_index,
      .inst_index = instruction_index,
      .asm_text = inline_asm.asm_text,
      .constraints = inline_asm.constraints,
      .side_effects = inline_asm.side_effects,
      .has_named_operand_references = inline_asm.has_named_operand_references,
      .has_template_modifiers = inline_asm.has_template_modifiers,
      .operands = {},
      .clobbers = inline_asm.clobbers,
      .result = call.result,
      .result_value_name = prepared_inline_asm_named_value_id(names, call.result),
      .result_home = call.result.has_value()
                         ? prepared_inline_asm_home_for_named_value(names,
                                                                    value_locations,
                                                                    *call.result)
                         : std::nullopt,
      .missing_required_facts = {},
  };
  carrier.operands.reserve(inline_asm.operands.size());
  for (const auto& operand : inline_asm.operands) {
    carrier.operands.push_back(
        make_prepared_inline_asm_operand(names, value_locations, call, operand));
  }
  populate_inline_asm_home_identity(target_profile, carrier.result_home);
  for (auto& operand : carrier.operands) {
    populate_inline_asm_home_identity(target_profile, operand.home);
  }

  if (!call.inline_asm.has_value()) {
    append_inline_asm_missing_fact(function_carriers, carrier, "missing_inline_asm_metadata");
  }
  validate_inline_asm_carrier(function_carriers, carrier, call, inline_asm);
  if (carrier.missing_required_facts.empty()) {
    carrier.carrier_kind = PreparedInlineAsmCarrierKind::Complete;
  }
  return carrier;
}

}  // namespace

void populate_inline_asm_register_group_overrides(PreparedBirModule& prepared) {
  for (const auto& function : prepared.module.functions) {
    const FunctionNameId function_name =
        prepared.names.function_names.find(function.name);
    if (function_name == kInvalidFunctionName) {
      continue;
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<bir::CallInst>(&inst);
        if (call == nullptr || !call->inline_asm.has_value()) {
          continue;
        }
        publish_inline_asm_metadata_register_group_overrides(prepared,
                                                             function_name,
                                                             *call);
      }
    }
  }
}

void populate_inline_asm_carriers(PreparedBirModule& prepared) {
  prepared.inline_asm_carriers.functions.clear();

  for (const auto& function : prepared.module.functions) {
    const FunctionNameId function_name =
        prepared.names.function_names.find(function.name);
    if (function_name == kInvalidFunctionName) {
      continue;
    }
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_name);
    PreparedInlineAsmCarrierFunction function_carriers{
        .function_name = function_name,
        .carriers = {},
        .missing_required_facts = {},
    };

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
           ++instruction_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
        if (call == nullptr || !call->inline_asm.has_value()) {
          continue;
        }
        function_carriers.carriers.push_back(
            build_inline_asm_carrier(prepared.names,
                                     function_carriers,
                                     *call,
                                     block_index,
                                     instruction_index,
                                     prepared.target_profile,
                                     value_locations));
        publish_inline_asm_register_group_overrides(prepared,
                                                    function_carriers.carriers.back());
      }
    }
    if (!function_carriers.carriers.empty() ||
        !function_carriers.missing_required_facts.empty()) {
      prepared.inline_asm_carriers.functions.push_back(std::move(function_carriers));
    }
  }
}

}  // namespace c4c::backend::prepare
