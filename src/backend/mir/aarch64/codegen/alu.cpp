#include "alu.hpp"
#include "cast_ops.hpp"
#include "float_ops.hpp"
#include "operands.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace mir = c4c::backend::mir;

[[nodiscard]] PreparedScalarAluRecordResult scalar_alu_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarAluRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarUnaryRecordResult scalar_unary_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarUnaryRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] PreparedScalarInstructionRecordResult scalar_instruction_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarInstructionRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] std::optional<std::string> scalar_gp_register_name_with_view(
    const RegisterOperand& operand,
    abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] std::string scalar_immediate_name(const ImmediateOperand& operand) {
  return "#" + std::to_string(operand.signed_value);
}

[[nodiscard]] bool is_plain_add_sub_immediate(const ImmediateOperand& operand) {
  return operand.kind == ImmediateKind::SignedInteger && operand.signed_value >= 0 &&
         operand.signed_value <= 4095;
}

[[nodiscard]] std::optional<unsigned> integer_scalar_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<unsigned> unsigned_power_of_two_log2(
    const ImmediateOperand& immediate,
    bir::TypeKind type) {
  const auto width = integer_scalar_bit_width(type);
  if (!width.has_value()) {
    return std::nullopt;
  }
  const auto value = immediate.unsigned_value;
  if (value <= 1U || (value & (value - 1U)) != 0U) {
    return std::nullopt;
  }
  unsigned shift = 0U;
  for (std::uint64_t cursor = value; cursor > 1U; cursor >>= 1U) {
    ++shift;
  }
  if (shift >= *width) {
    return std::nullopt;
  }
  return shift;
}

[[nodiscard]] std::optional<ImmediateOperand> unsigned_reduction_replacement_immediate(
    bir::BinaryOpcode opcode,
    bir::TypeKind type,
    const ImmediateOperand& divisor) {
  const auto shift = unsigned_power_of_two_log2(divisor, type);
  if (!shift.has_value()) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::UDiv) {
    return ImmediateOperand{
        .kind = ImmediateKind::UnsignedInteger,
        .type = type,
        .signed_value = static_cast<std::int64_t>(*shift),
        .unsigned_value = *shift,
        .source_value_id = divisor.source_value_id,
        .source_value_name = divisor.source_value_name,
    };
  }
  if (opcode == bir::BinaryOpcode::URem) {
    const auto mask = divisor.unsigned_value - 1U;
    return ImmediateOperand{
        .kind = ImmediateKind::UnsignedInteger,
        .type = type,
        .signed_value = static_cast<std::int64_t>(mask),
        .unsigned_value = mask,
        .source_value_id = divisor.source_value_id,
        .source_value_name = divisor.source_value_name,
    };
  }
  return std::nullopt;
}

[[nodiscard]] bool is_unsigned_power_of_two_reduction_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::UDiv || opcode == bir::BinaryOpcode::URem;
}

[[nodiscard]] std::optional<unsigned> unsigned_reduction_post_zero_extend_bits(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<unsigned> scalar_alu_post_sign_extend_bits(
    bir::BinaryOpcode opcode,
    bir::TypeKind operand_type,
    bir::TypeKind result_type) {
  if ((opcode == bir::BinaryOpcode::Add || opcode == bir::BinaryOpcode::Sub) &&
      operand_type == bir::TypeKind::I32 && result_type == bir::TypeKind::I64) {
    return 32U;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<ScalarAluOperationKind> unsigned_reduction_operation(
    bir::BinaryOpcode opcode,
    bir::TypeKind type,
    const OperandRecord& rhs) {
  const auto* immediate = std::get_if<ImmediateOperand>(&rhs.payload);
  if (rhs.kind != OperandKind::Immediate || immediate == nullptr ||
      !unsigned_reduction_replacement_immediate(opcode, type, *immediate).has_value()) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::UDiv) {
    return ScalarAluOperationKind::LogicalShiftRight;
  }
  if (opcode == bir::BinaryOpcode::URem) {
    return ScalarAluOperationKind::And;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_storage_register_view(
    bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
}

[[nodiscard]] const prepare::PreparedValueHome* find_prepared_scalar_value_home(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(names, value_locations, value.name);
}

[[nodiscard]] const prepare::PreparedStoragePlanValue* find_prepared_scalar_storage(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

[[nodiscard]] std::string_view register_display_name(abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
}

[[nodiscard]] std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg) {
  const auto display_name = register_display_name(reg);
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

[[nodiscard]] std::vector<abi::RegisterReference> occupied_register_references(
    abi::RegisterReference reg) {
  return {reg};
}

[[nodiscard]] std::optional<RegisterOperand> make_prepared_scalar_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type,
    RegisterOperandRole role) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register) {
    return std::nullopt;
  }

  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_class = register_class_from_bank(storage.bank);
  abi::PreparedRegisterConversionResult converted;
  if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    if (!home.register_name.has_value() || !storage.register_name.has_value() ||
        *home.register_name != *storage.register_name) {
      return std::nullopt;
    }
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

[[nodiscard]] std::optional<OperandRecord> make_immediate_scalar_operand(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  ImmediateKind immediate_kind = ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
    immediate_kind = ImmediateKind::Boolean;
  }

  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return make_immediate_operand(ImmediateOperand{
          .kind = immediate_kind,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits != 0U
                                ? value.immediate_bits
                                : static_cast<std::uint64_t>(value.immediate),
      });
    default:
      return std::nullopt;
  }
}

[[nodiscard]] RegisterOperandRole register_role_from_authority(
    OperandAuthority authority) {
  switch (authority) {
    case OperandAuthority::RegallocAssignment:
      return RegisterOperandRole::AllocationResult;
    case OperandAuthority::StoragePlan:
      return RegisterOperandRole::StoragePlan;
    case OperandAuthority::PreparedValueHome:
      return RegisterOperandRole::ValueHome;
    default:
      return RegisterOperandRole::Physical;
  }
}

[[nodiscard]] std::optional<OperandRecord> make_resolved_scalar_operand(
    const ResolvedOperand& resolved,
    const bir::Value& value) {
  if (const auto* immediate =
          std::get_if<mir::Immediate>(&resolved.operand.payload)) {
    return make_immediate_operand(ImmediateOperand{
        .kind = immediate->kind == mir::ImmediateKind::Boolean
                    ? ImmediateKind::Boolean
                    : ImmediateKind::SignedInteger,
        .type = value.type,
        .signed_value = immediate->signed_value,
        .unsigned_value = immediate->unsigned_value,
        .source_value_id = resolved.value_id,
        .source_value_name = resolved.value_name,
    });
  }

  if (!resolved.register_reference.has_value()) {
    return std::nullopt;
  }

  return make_register_operand(RegisterOperand{
      .reg = *resolved.register_reference,
      .role = register_role_from_authority(resolved.authority),
      .value_id = resolved.value_id,
      .value_name = resolved.value_name,
      .expected_view = scalar_register_view(value.type),
  });
}

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const bir::Value& value,
    const module::FunctionLoweringContext& context) {
  if (context.prepared == nullptr || context.value_locations == nullptr ||
      value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto value_name =
      prepare::resolve_prepared_value_name_id(context.prepared->names, value.name);
  if (!value_name.has_value()) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(*context.value_locations, *value_name);
}

[[nodiscard]] std::optional<OperandRecord> make_named_scalar_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr) {
    return std::nullopt;
  }
  auto resolved =
      resolve_value_operand(home->value_id, context.function, diagnostics);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  return make_resolved_scalar_operand(*resolved, value);
}

[[nodiscard]] std::optional<RegisterOperand> find_return_abi_register(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(type);
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BeforeReturn ||
        bundle.block_index != context.block_index) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id != value_id ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      abi::PreparedRegisterConversionResult converted;
      if (move.destination_register_placement.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_placement,
                                                   std::nullopt,
                                                   expected_view);
      } else if (move.destination_register_name.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_name,
                                                   std::nullopt,
                                                   std::nullopt,
                                                   expected_view);
      } else {
        continue;
      }
      if (!converted.reg.has_value()) {
        continue;
      }
      return RegisterOperand{
          .reg = *converted.reg,
          .role = RegisterOperandRole::CallAbi,
          .value_id = value_id,
          .value_name = value_name,
          .expected_view = expected_view,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<RegisterOperand> retarget_register_operand(
    RegisterOperand reg,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  reg.value_id = value_id;
  reg.value_name = value_name;
  reg.expected_view = scalar_register_view(type);
  return reg;
}

[[nodiscard]] bool value_matches_name(const bir::Value& value,
                                      std::string_view name) {
  return value.kind == bir::Value::Kind::Named && value.name == name;
}

[[nodiscard]] std::optional<RegisterOperand> find_return_chain_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedValueHome& result_home,
    bir::TypeKind result_type) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr) {
    return std::nullopt;
  }

  std::string current_name =
      std::string(prepare::prepared_value_name(context.function.prepared->names,
                                               result_home.value_name));
  if (current_name.empty()) {
    return std::nullopt;
  }

  for (std::size_t next_index = instruction_index + 1;
       next_index < context.bir_block->insts.size();
       ++next_index) {
    const auto* next_binary =
        std::get_if<bir::BinaryInst>(&context.bir_block->insts[next_index]);
    if (next_binary == nullptr ||
        !is_scalar_alu_integer_opcode(next_binary->opcode)) {
      return std::nullopt;
    }
    const bool consumes_current = value_matches_name(next_binary->lhs, current_name) ||
                                  value_matches_name(next_binary->rhs, current_name);
    if (!consumes_current || next_binary->result.kind != bir::Value::Kind::Named) {
      return std::nullopt;
    }
    current_name = next_binary->result.name;
  }

  if (!context.bir_block->terminator.value.has_value() ||
      !value_matches_name(*context.bir_block->terminator.value, current_name)) {
    return std::nullopt;
  }

  const auto* terminal_home =
      find_named_value_home(*context.bir_block->terminator.value, context.function);
  if (terminal_home == nullptr) {
    return std::nullopt;
  }
  auto terminal_register =
      find_return_abi_register(context,
                               terminal_home->value_id,
                               terminal_home->value_name,
                               context.bir_block->terminator.value->type);
  if (!terminal_register.has_value()) {
    return std::nullopt;
  }
  return retarget_register_operand(*terminal_register,
                                   result_home.value_id,
                                   result_home.value_name,
                                   result_type);
}

[[nodiscard]] std::optional<OperandRecord> make_scalar_fallback_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return make_immediate_scalar_operand(value);
  }
  const auto* home = find_named_value_home(value, context.function);
  const auto emitted =
      home == nullptr ? std::optional<RegisterOperand>{}
                      : find_emitted_scalar_register(scalar_state, home->value_name);
  if (emitted.has_value()) {
    return make_register_operand(*emitted);
  }
  return make_named_scalar_operand(value, context, diagnostics);
}

}  // namespace

ScalarAluPrintResult make_scalar_alu_print_lines(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar) {
  if (scalar.scalar_unary.has_value() &&
      scalar.scalar_unary->supported_integer_operation) {
    const auto& unary = *scalar.scalar_unary;
    if (scalar.inputs.size() != 1) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unary node requires exactly one structured register operand"};
    }
    const auto result_view = scalar_register_view(unary.result_type);
    const auto operand_view = scalar_register_view(unary.operand_type);
    if (!result_view.has_value() || !operand_view.has_value() ||
        result_view != operand_view) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unary node requires matching I32/I64 operand and result widths"};
    }
    const auto* operand_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    if (scalar.inputs[0].kind != OperandKind::Register || operand_register == nullptr) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar unary node requires a structured register operand"};
    }
    if (!scalar.result_register.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar unary node requires a structured result register"};
    }

    std::string_view mnemonic;
    switch (unary.operation) {
      case ScalarUnaryOperationKind::Neg:
        mnemonic = "neg";
        break;
      case ScalarUnaryOperationKind::BitNot:
        mnemonic = "mvn";
        break;
      case ScalarUnaryOperationKind::CountLeadingZeros:
        mnemonic = "clz";
        break;
      case ScalarUnaryOperationKind::CountTrailingZeros:
        mnemonic = "rbit";
        break;
      case ScalarUnaryOperationKind::ByteSwap:
        mnemonic = "rev";
        break;
      case ScalarUnaryOperationKind::Deferred:
        break;
    }
    if (mnemonic.empty()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar unary operation is not printable"};
    }
    const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
    const auto operand = scalar_gp_register_name_with_view(*operand_register, *operand_view);
    if (!result.has_value() || !operand.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar unary node has incomplete printable register facts"};
    }
    std::vector<std::string> lines;
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *operand;
    lines.push_back(out.str());
    if (unary.operation == ScalarUnaryOperationKind::CountTrailingZeros) {
      std::ostringstream clz;
      clz << "clz " << *result << ", " << *result;
      lines.push_back(clz.str());
    } else if (unary.operation == ScalarUnaryOperationKind::ByteSwap &&
               unary.result_type == bir::TypeKind::I16) {
      std::ostringstream shift;
      shift << "lsr " << *result << ", " << *result << ", #16";
      lines.push_back(shift.str());
    }
    return {.lines = std::move(lines), .diagnostic = {}};
  }
  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_floating_operation) {
    return make_scalar_float_alu_print_lines(scalar);
  }
  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_integer_operation &&
      (scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::UDiv ||
       scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::URem)) {
    const auto& alu = *scalar.scalar_alu;
    if (scalar.inputs.size() != 2) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar unsigned reduction node requires two structured operands"};
    }
    const auto result_view = scalar_register_view(alu.result_type);
    const auto operand_view = scalar_register_view(alu.operand_type);
    if (!result_view.has_value() || result_view != operand_view ||
        !integer_scalar_bit_width(alu.result_type).has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node requires matching integer widths"};
    }
    if (alu.post_zero_extend_result_bits.has_value() &&
        *alu.post_zero_extend_result_bits != 8U &&
        *alu.post_zero_extend_result_bits != 16U) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node has unsupported post-zero-extension width"};
    }
    const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
    if (scalar.inputs[0].kind != OperandKind::Register ||
        scalar.inputs[1].kind != OperandKind::Immediate || lhs_register == nullptr ||
        rhs_immediate == nullptr || !scalar.result_register.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node requires register lhs, immediate reduction, and result register"};
    }
    const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
    const auto lhs = scalar_gp_register_name_with_view(*lhs_register, *result_view);
    if (!result.has_value() || !lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node has incomplete printable register facts"};
    }
    std::ostringstream out;
    if (alu.operation == ScalarAluOperationKind::LogicalShiftRight &&
        alu.source_binary_opcode == bir::BinaryOpcode::UDiv) {
      out << "lsr " << *result << ", " << *lhs << ", #" << rhs_immediate->unsigned_value;
      std::vector<std::string> lines{out.str()};
      if (alu.post_zero_extend_result_bits.has_value()) {
        std::ostringstream extend;
        extend << "ubfx " << *result << ", " << *result << ", #0, #"
               << *alu.post_zero_extend_result_bits;
        lines.push_back(extend.str());
      }
      return {.lines = std::move(lines), .diagnostic = {}};
    }
    if (alu.operation == ScalarAluOperationKind::And &&
        alu.source_binary_opcode == bir::BinaryOpcode::URem) {
      out << "and " << *result << ", " << *lhs << ", #" << rhs_immediate->unsigned_value;
      std::vector<std::string> lines{out.str()};
      if (alu.post_zero_extend_result_bits.has_value()) {
        std::ostringstream extend;
        extend << "ubfx " << *result << ", " << *result << ", #0, #"
               << *alu.post_zero_extend_result_bits;
        lines.push_back(extend.str());
      }
      return {.lines = std::move(lines), .diagnostic = {}};
    }
    return {.lines = std::nullopt,
            .diagnostic = "scalar unsigned reduction operation is not printable"};
  }
  if (instruction.opcode != MachineOpcode::Add && instruction.opcode != MachineOpcode::Sub) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar node opcode is outside the printable add/sub subset"};
  }
  if (scalar.inputs.size() != 2) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub node requires exactly two register or immediate operands"};
  }

  const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
  const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
  const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
  const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
  const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                               lhs_register != nullptr;
  const bool rhs_is_register = scalar.inputs[1].kind == OperandKind::Register &&
                               rhs_register != nullptr;
  const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                lhs_immediate != nullptr;
  const bool rhs_is_immediate = scalar.inputs[1].kind == OperandKind::Immediate &&
                                rhs_immediate != nullptr;
  if ((!lhs_is_register && !lhs_is_immediate) || (!rhs_is_register && !rhs_is_immediate)) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub node requires register or immediate operands"};
  }
  if ((lhs_is_immediate && !is_plain_add_sub_immediate(*lhs_immediate)) ||
      (rhs_is_immediate && !is_plain_add_sub_immediate(*rhs_immediate))) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub immediate operand is outside the plain #imm encoding range 0..4095"};
  }

  const auto mnemonic = machine_instruction_primary_printer_mnemonic(instruction);
  if (mnemonic.empty()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub mnemonic is not printable"};
  }
  const auto& alu = *scalar.scalar_alu;
  if (alu.post_zero_extend_result_bits.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub node does not support post-zero-extension facts"};
  }
  if (alu.post_sign_extend_result_bits.has_value() &&
      (*alu.post_sign_extend_result_bits != 32U ||
       alu.operand_type != bir::TypeKind::I32 ||
       alu.result_type != bir::TypeKind::I64)) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub node has unsupported post-sign-extension width"};
  }

  std::vector<std::string> lines;
  const auto result_view =
      alu.post_sign_extend_result_bits.has_value() ? scalar_register_view(alu.operand_type)
                                                   : scalar_register_view(alu.result_type);
  if (!result_view.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub node has unsupported printable result width"};
  }
  const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
  if (!result.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub node has incomplete printable result register fact"};
  }
  if (lhs_is_register && rhs_is_register) {
    const auto lhs = scalar_gp_register_name_with_view(*lhs_register, *result_view);
    const auto rhs = scalar_gp_register_name_with_view(*rhs_register, *result_view);
    if (!lhs.has_value() || !rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub node has incomplete printable operand register facts"};
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *lhs << ", " << *rhs;
    lines.push_back(out.str());
  } else if (lhs_is_register && rhs_is_immediate) {
    const auto lhs = scalar_gp_register_name_with_view(*lhs_register, *result_view);
    if (!lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub node has incomplete printable operand register facts"};
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *lhs << ", "
        << scalar_immediate_name(*rhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_register && instruction.opcode == MachineOpcode::Add) {
    const auto rhs = scalar_gp_register_name_with_view(*rhs_register, *result_view);
    if (!rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub node has incomplete printable operand register facts"};
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << *rhs << ", "
        << scalar_immediate_name(*lhs_immediate);
    lines.push_back(out.str());
  } else if (lhs_is_immediate && rhs_is_immediate) {
    const auto move_mnemonic =
        machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Move);
    std::ostringstream move_line;
    move_line << move_mnemonic << " " << *result << ", "
              << scalar_immediate_name(*lhs_immediate);
    lines.push_back(move_line.str());
    std::ostringstream add_line;
    add_line << mnemonic << " " << *result << ", " << *result << ", "
             << scalar_immediate_name(*rhs_immediate);
    lines.push_back(add_line.str());
  } else {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar sub with an immediate lhs and register rhs is not printable"};
  }
  if (alu.post_sign_extend_result_bits.has_value()) {
    const auto extended_result =
        scalar_gp_register_name_with_view(*scalar.result_register, abi::RegisterView::X);
    if (!extended_result.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub node has incomplete printable post-sign-extension result fact"};
    }
    std::ostringstream extend;
    extend << "sxtw " << *extended_result << ", " << *result;
    lines.push_back(extend.str());
  }
  return {.lines = std::move(lines), .diagnostic = {}};
}

bool is_scalar_alu_integer_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return true;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

bool is_scalar_unary_integer_operation(ScalarUnaryOperationKind operation,
                                       bir::TypeKind type) {
  switch (operation) {
    case ScalarUnaryOperationKind::Neg:
    case ScalarUnaryOperationKind::BitNot:
    case ScalarUnaryOperationKind::CountLeadingZeros:
    case ScalarUnaryOperationKind::CountTrailingZeros:
      return type == bir::TypeKind::I32 || type == bir::TypeKind::I64;
    case ScalarUnaryOperationKind::ByteSwap:
      return type == bir::TypeKind::I16 || type == bir::TypeKind::I32 ||
             type == bir::TypeKind::I64;
    case ScalarUnaryOperationKind::Deferred:
      return false;
  }
  return false;
}

ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return ScalarAluOperationKind::Add;
    case bir::BinaryOpcode::Sub:
      return ScalarAluOperationKind::Sub;
    case bir::BinaryOpcode::Mul:
      return ScalarAluOperationKind::Mul;
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
      return ScalarAluOperationKind::Div;
    case bir::BinaryOpcode::And:
      return ScalarAluOperationKind::And;
    case bir::BinaryOpcode::Or:
      return ScalarAluOperationKind::Or;
    case bir::BinaryOpcode::Xor:
      return ScalarAluOperationKind::Xor;
    case bir::BinaryOpcode::LShr:
      return ScalarAluOperationKind::LogicalShiftRight;
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return ScalarAluOperationKind::Deferred;
  }
  return ScalarAluOperationKind::Deferred;
}

std::optional<abi::RegisterView> scalar_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    default:
      return std::nullopt;
  }
}

std::optional<RegisterOperand> find_emitted_scalar_register(
    const BlockScalarLoweringState& state,
    c4c::ValueNameId value_name) {
  const auto emitted = state.emitted_registers.find(value_name);
  if (emitted == state.emitted_registers.end()) {
    return std::nullopt;
  }
  return emitted->second;
}

void record_emitted_scalar_register(BlockScalarLoweringState& state,
                                    c4c::ValueNameId value_name,
                                    RegisterOperand reg) {
  if (value_name == c4c::kInvalidValueName) {
    return;
  }
  state.emitted_registers[value_name] = std::move(reg);
}

std::optional<ImmediateOperand> make_scalar_immediate_operand(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id,
    c4c::ValueNameId source_value_name) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  ImmediateKind kind = ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
    kind = ImmediateKind::Boolean;
  } else if (!scalar_register_view(value.type).has_value()) {
    return std::nullopt;
  }

  return ImmediateOperand{
      .kind = kind,
      .type = value.type,
      .signed_value = value.immediate,
      .unsigned_value = value.immediate_bits != 0U ? value.immediate_bits
                                                   : static_cast<std::uint64_t>(value.immediate),
      .source_value_id = source_value_id,
      .source_value_name = source_value_name,
  };
}

PreparedScalarAluRecordError make_prepared_scalar_operand(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& value,
    OperandRecord& out) {
  if (value.kind == bir::Value::Kind::Immediate) {
    const auto immediate = make_scalar_immediate_operand(value);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return PreparedScalarAluRecordError::UnsupportedOperandValue;
  }

  const auto* home = find_prepared_scalar_value_home(names, value_locations, value);
  if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
    return PreparedScalarAluRecordError::MissingOperandValueHome;
  }
  const auto* storage = find_prepared_scalar_storage(storage_plan, home->value_id);
  if (storage == nullptr || storage->value_name != home->value_name) {
    return PreparedScalarAluRecordError::MissingOperandStorage;
  }

  if (storage->encoding == prepare::PreparedStorageEncodingKind::Immediate) {
    if (home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate ||
        !home->immediate_i32.has_value() || !storage->immediate_i32.has_value() ||
        *home->immediate_i32 != *storage->immediate_i32) {
      return PreparedScalarAluRecordError::UnsupportedOperandStorage;
    }
    const auto immediate = make_scalar_immediate_operand(
        bir::Value::immediate_i32(static_cast<std::int32_t>(*storage->immediate_i32)),
        home->value_id,
        home->value_name);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }

  if (home->kind != prepare::PreparedValueHomeKind::Register ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!storage->register_placement.has_value() &&
       (!home->register_name.has_value() || !storage->register_name.has_value() ||
        *home->register_name != *storage->register_name))) {
    return PreparedScalarAluRecordError::UnsupportedOperandStorage;
  }

  const auto reg = make_prepared_scalar_register_operand(
      *home, *storage, value.type, RegisterOperandRole::StoragePlan);
  if (!reg.has_value()) {
    return PreparedScalarAluRecordError::RegisterConversionFailed;
  }
  out = make_register_operand(*reg);
  return PreparedScalarAluRecordError::None;
}

PreparedScalarAluRecordError make_prepared_scalar_result_register_operand(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& result,
    RegisterOperand& out) {
  if (result.kind != bir::Value::Kind::Named || result.name.empty()) {
    return PreparedScalarAluRecordError::UnsupportedResultValue;
  }

  const auto* result_home = find_prepared_scalar_value_home(names, value_locations, result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return PreparedScalarAluRecordError::MissingResultValueHome;
  }
  const auto* result_storage = find_prepared_scalar_storage(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return PreparedScalarAluRecordError::MissingResultStorage;
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() ||
        !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return PreparedScalarAluRecordError::UnsupportedResultStorage;
  }
  const auto result_register = make_prepared_scalar_register_operand(
      *result_home, *result_storage, result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return PreparedScalarAluRecordError::RegisterConversionFailed;
  }
  out = *result_register;
  return PreparedScalarAluRecordError::None;
}

ScalarInstructionRecord make_scalar_alu_instruction_record(ScalarAluRecord alu) {
  return ScalarInstructionRecord{
      .result_value_id = alu.result_value_id,
      .result_value_name = alu.result_value_name,
      .result_type = alu.result_type,
      .result_register = alu.result_register,
      .inputs = {alu.lhs, alu.rhs},
      .source_binary_opcode = alu.source_binary_opcode,
      .scalar_alu = alu,
  };
}

PreparedScalarAluRecordResult make_prepared_scalar_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::InvalidFunction);
  }
  const bool is_integer_operation =
      scalar_register_view(binary.operand_type).has_value() &&
      scalar_register_view(binary.result.type).has_value() &&
      is_scalar_alu_integer_opcode(binary.opcode);
  if (!is_integer_operation && is_prepared_scalar_float_alu_operation(binary)) {
    return make_prepared_scalar_float_alu_record(
        names, value_locations, storage_plan, binary);
  }
  const bool may_be_unsigned_reduction =
      is_unsigned_power_of_two_reduction_opcode(binary.opcode) &&
      scalar_register_view(binary.operand_type).has_value() &&
      scalar_register_view(binary.result.type).has_value() &&
      binary.operand_type == binary.result.type;
  if (!is_integer_operation && !may_be_unsigned_reduction) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedResultValue);
  }
  if (!scalar_storage_register_view(binary.result.type).has_value() ||
      !scalar_storage_register_view(binary.operand_type).has_value()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOperandType);
  }

  RegisterOperand result_register;
  if (const auto error = make_prepared_scalar_result_register_operand(
          names, value_locations, storage_plan, binary.result, result_register);
      error != PreparedScalarAluRecordError::None) {
    return scalar_alu_record_error(error);
  }

  OperandRecord lhs;
  OperandRecord rhs;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.lhs, lhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_alu_record_error(error);
  }
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.rhs, rhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_alu_record_error(error);
  }

  ScalarAluOperationKind operation = scalar_alu_operation_from_binary_opcode(binary.opcode);
  bool supported_integer_operation = is_integer_operation;
  std::optional<unsigned> post_zero_extend_result_bits;
  std::optional<unsigned> post_sign_extend_result_bits =
      scalar_alu_post_sign_extend_bits(binary.opcode, binary.operand_type, binary.result.type);
  if (!supported_integer_operation &&
      scalar_register_view(binary.operand_type).has_value() &&
      scalar_register_view(binary.result.type).has_value() &&
      binary.operand_type == binary.result.type &&
      is_unsigned_power_of_two_reduction_opcode(binary.opcode)) {
    const auto reduction_operation =
        unsigned_reduction_operation(binary.opcode, binary.operand_type, rhs);
    if (!reduction_operation.has_value()) {
      return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
    }
    const auto* divisor = std::get_if<ImmediateOperand>(&rhs.payload);
    const auto replacement =
        unsigned_reduction_replacement_immediate(binary.opcode, binary.operand_type, *divisor);
    rhs = make_immediate_operand(*replacement);
    operation = *reduction_operation;
    supported_integer_operation = true;
    post_zero_extend_result_bits =
        unsigned_reduction_post_zero_extend_bits(binary.result.type);
  }

  return PreparedScalarAluRecordResult{
      .record =
          ScalarAluRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = operation,
              .source_binary_opcode = binary.opcode,
              .operand_type = binary.operand_type,
              .result_value_id = result_register.value_id,
              .result_value_name = result_register.value_name,
              .result_type = binary.result.type,
              .result_register = result_register,
              .lhs = lhs,
              .rhs = rhs,
              .post_zero_extend_result_bits = post_zero_extend_result_bits,
              .post_sign_extend_result_bits = post_sign_extend_result_bits,
              .supported_integer_operation = supported_integer_operation,
              .supported_floating_operation = false,
          },
      .error = PreparedScalarAluRecordError::None,
  };
}

PreparedScalarInstructionRecordResult make_prepared_scalar_alu_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary) {
  const auto result = make_prepared_scalar_alu_record(names, value_locations, storage_plan, binary);
  if (!result.record.has_value()) {
    return scalar_instruction_record_error(result.error);
  }
  return PreparedScalarInstructionRecordResult{
      .record = make_scalar_alu_instruction_record(*result.record),
      .error = PreparedScalarAluRecordError::None,
  };
}

ScalarInstructionRecord make_scalar_unary_instruction_record(ScalarUnaryRecord unary) {
  return ScalarInstructionRecord{
      .result_value_id = unary.result_value_id,
      .result_value_name = unary.result_value_name,
      .result_type = unary.result_type,
      .result_register = unary.result_register,
      .inputs = {unary.operand},
      .scalar_unary = unary,
  };
}

PreparedScalarUnaryRecordResult make_prepared_scalar_unary_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    ScalarUnaryOperationKind operation,
    const bir::Value& result,
    const bir::Value& operand) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::InvalidFunction);
  }
  if (result.kind != bir::Value::Kind::Named) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::UnsupportedResultValue);
  }
  if (result.type != operand.type ||
      !is_scalar_unary_integer_operation(operation, operand.type)) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::UnsupportedOperandType);
  }

  const auto* result_home = find_prepared_scalar_value_home(names, value_locations, result);
  if (result_home == nullptr) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::MissingResultValueHome);
  }
  const auto* result_storage =
      find_prepared_scalar_storage(storage_plan, result_home->value_id);
  if (result_storage == nullptr) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() ||
        !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::UnsupportedResultStorage);
  }
  const auto result_register = make_prepared_scalar_register_operand(
      *result_home, *result_storage, result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return scalar_unary_record_error(PreparedScalarAluRecordError::RegisterConversionFailed);
  }

  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, operand, source);
      error != PreparedScalarAluRecordError::None) {
    return scalar_unary_record_error(error);
  }

  return PreparedScalarUnaryRecordResult{
      .record =
          ScalarUnaryRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = operation,
              .operand_type = operand.type,
              .result_value_id = result_home->value_id,
              .result_value_name = result_home->value_name,
              .result_type = result.type,
              .result_register = result_register,
              .operand = source,
              .supported_integer_operation = true,
          },
      .error = PreparedScalarAluRecordError::None,
  };
}

PreparedScalarInstructionRecordResult make_prepared_scalar_unary_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    ScalarUnaryOperationKind operation,
    const bir::Value& result,
    const bir::Value& operand) {
  const auto prepared =
      make_prepared_scalar_unary_record(names, value_locations, storage_plan, operation, result, operand);
  if (!prepared.record.has_value()) {
    return scalar_instruction_record_error(prepared.error);
  }
  return PreparedScalarInstructionRecordResult{
      .record = make_scalar_unary_instruction_record(*prepared.record),
      .error = PreparedScalarAluRecordError::None,
  };
}

std::optional<module::MachineInstruction> lower_scalar_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr) {
    return std::nullopt;
  }

  std::optional<ScalarInstructionRecord> scalar_record;
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    const auto prepared = make_prepared_scalar_cast_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *cast);
    scalar_record = prepared.record;
  } else if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    if (is_prepared_scalar_float_alu_operation(*binary)) {
      return std::nullopt;
    }
    const auto prepared =
        make_prepared_scalar_alu_instruction_record(
            context.function.prepared->names,
            *context.function.value_locations,
            *context.function.storage_plan,
            *binary);
    scalar_record = prepared.record;
    if (!scalar_record.has_value()) {
      const auto* result_home = find_named_value_home(binary->result, context.function);
      auto result_register =
          result_home == nullptr
              ? std::optional<RegisterOperand>{}
              : find_return_abi_register(context,
                                         result_home->value_id,
                                         result_home->value_name,
                                         binary->result.type);
      if (!result_register.has_value() && result_home != nullptr) {
        result_register = find_return_chain_register(
            context, instruction_index, *result_home, binary->result.type);
      }
      const auto lhs =
          make_scalar_fallback_operand(binary->lhs, context, scalar_state, diagnostics);
      const auto rhs =
          make_scalar_fallback_operand(binary->rhs, context, scalar_state, diagnostics);
      if (result_home == nullptr || !result_register.has_value() || !lhs.has_value() ||
          !rhs.has_value() || !is_scalar_alu_integer_opcode(binary->opcode)) {
        return std::nullopt;
      }
      scalar_record = make_scalar_alu_instruction_record(ScalarAluRecord{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .operation = scalar_alu_operation_from_binary_opcode(binary->opcode),
          .source_binary_opcode = binary->opcode,
          .operand_type = binary->operand_type,
          .result_value_id = result_home->value_id,
          .result_value_name = result_home->value_name,
          .result_type = binary->result.type,
          .result_register = *result_register,
          .lhs = *lhs,
          .rhs = *rhs,
          .supported_integer_operation = true,
      });
    }
  } else {
    return std::nullopt;
  }
  if (!scalar_record.has_value()) {
    return std::nullopt;
  }

  InstructionRecord target = make_scalar_instruction(*scalar_record);
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    return std::nullopt;
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (scalar_record->result_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   scalar_record->result_value_name,
                                   *scalar_record->result_register);
  }

  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

}  // namespace c4c::backend::aarch64::codegen
