#include "alu.hpp"
#include "cast_ops.hpp"
#include "dispatch.hpp"

#include "constant_materialization.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "float_ops.hpp"
#include "frame_slot_address.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "select_materialization.hpp"

#include "../../../prealloc/prepared_lookups.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] bool registers_alias(const RegisterOperand& lhs,
                                   const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}

[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs) {
  return registers_alias(lhs, rhs);
}

[[nodiscard]] std::optional<unsigned> value_power_of_two_shift(
    const bir::Value& value,
    std::optional<std::int64_t> same_block_integer_constant) {
  if (value.kind == bir::Value::Kind::Immediate) {
    if (value.immediate < 0) {
      return std::nullopt;
    }
    return power_of_two_shift(static_cast<std::uint64_t>(value.immediate));
  }
  if (!same_block_integer_constant.has_value() ||
      *same_block_integer_constant < 0) {
    return std::nullopt;
  }
  return power_of_two_shift(
      static_cast<std::uint64_t>(*same_block_integer_constant));
}

[[nodiscard]] bool value_publication_may_write_scratch_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    SameBlockIntegerConstantLookup same_block_integer_constant,
    SameBlockScalarProducerLookup same_block_scalar_producer,
    unsigned depth) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  if (same_block_integer_constant == nullptr ||
      same_block_scalar_producer == nullptr) {
    return false;
  }
  if (same_block_integer_constant(context, value, before_instruction_index)
          .has_value()) {
    return false;
  }
  if (const auto value_view = scalar_view_for_type(value.type);
      value_view.has_value()) {
    if (current_block_entry_publication_register(context, value, *value_view)) {
      return false;
    }
  }

  const auto producer_context =
      same_block_scalar_producer(context, value, before_instruction_index);
  if (!producer_context.has_value() || producer_context->instruction == nullptr) {
    return false;
  }
  const auto* producer = producer_context->instruction;
  const auto producer_index = producer_context->instruction_index;

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return value_publication_may_write_scratch_register(
        context,
        operand,
        producer_index,
        same_block_integer_constant,
        same_block_scalar_producer,
        depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer); binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (binary->opcode == bir::BinaryOpcode::Mul &&
        value_power_of_two_shift(
            rhs,
            same_block_integer_constant(context, rhs, producer_index))
            .has_value()) {
      return value_publication_may_write_scratch_register(
          context,
          lhs,
          producer_index,
          same_block_integer_constant,
          same_block_scalar_producer,
          depth + 1);
    }
    return true;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr ||
      std::get_if<bir::LoadLocalInst>(producer) != nullptr ||
      std::get_if<bir::SelectInst>(producer) != nullptr) {
    return true;
  }
  return false;
}

namespace {

namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

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

[[nodiscard]] bool is_plain_logical_immediate(const ImmediateOperand& operand) {
  (void)operand;
  return false;
}

[[nodiscard]] bool is_plain_mov_immediate(const ImmediateOperand& operand) {
  return operand.kind == ImmediateKind::SignedInteger && operand.signed_value >= -65536 &&
         operand.signed_value <= 65535;
}

[[nodiscard]] bool scalar_alu_operation_accepts_immediate(
    ScalarAluOperationKind operation,
    const ImmediateOperand& operand) {
  switch (operation) {
    case ScalarAluOperationKind::Add:
    case ScalarAluOperationKind::Sub:
      return is_plain_add_sub_immediate(operand);
    case ScalarAluOperationKind::And:
    case ScalarAluOperationKind::Or:
    case ScalarAluOperationKind::Xor:
      return is_plain_logical_immediate(operand);
    case ScalarAluOperationKind::Mul:
    case ScalarAluOperationKind::Div:
    case ScalarAluOperationKind::LogicalShiftRight:
    case ScalarAluOperationKind::Deferred:
      return false;
  }
  return false;
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

[[nodiscard]] std::optional<std::size_t> scalar_type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
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

[[nodiscard]] std::optional<std::string> scalar_alu_stack_publication_line(
    const ScalarAluRecord& alu,
    std::string_view result) {
  if (!alu.result_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  if (*alu.result_stack_offset_bytes < 0) {
    return std::string{};
  }
  std::string_view mnemonic = "str";
  if (alu.result_type == bir::TypeKind::I1 || alu.result_type == bir::TypeKind::I8) {
    mnemonic = "strb";
  } else if (alu.result_type == bir::TypeKind::I16) {
    mnemonic = "strh";
  }
  std::ostringstream store;
  store << mnemonic << " " << result << ", ["
        << (alu.result_uses_frame_pointer_base ? "x29" : "sp");
  if (*alu.result_stack_offset_bytes != 0) {
    store << ", #" << *alu.result_stack_offset_bytes;
  }
  store << "]";
  return store.str();
}

[[nodiscard]] std::optional<std::size_t> scalar_publication_width_bytes(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1U;
    case bir::TypeKind::I16:
      return 2U;
    case bir::TypeKind::I32:
      return 4U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 8U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] bool stack_publication_direct_offset_is_encodable(std::int64_t offset,
                                                                std::size_t width_bytes) {
  if (offset < 0 || width_bytes == 0) {
    return false;
  }
  const auto unsigned_offset = static_cast<std::uint64_t>(offset);
  return unsigned_offset % width_bytes == 0 &&
         unsigned_offset / width_bytes <= 4095U;
}

[[nodiscard]] bool scalar_result_uses_scratch(std::string_view result,
                                              abi::RegisterReference scratch) {
  return result == abi::register_name(abi::x_register(scratch.index)) ||
         result == abi::register_name(abi::w_register(scratch.index));
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_publication_address_scratch(
    std::string_view result) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (!scalar_result_uses_scratch(result, scratch)) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::vector<std::string>> scalar_alu_stack_publication_lines(
    const ScalarAluRecord& alu,
    std::string_view result) {
  if (!alu.result_stack_offset_bytes.has_value()) {
    return std::vector<std::string>{};
  }
  const auto width = scalar_publication_width_bytes(alu.result_type);
  if (!width.has_value() || *alu.result_stack_offset_bytes < 0) {
    return std::nullopt;
  }
  if (stack_publication_direct_offset_is_encodable(*alu.result_stack_offset_bytes,
                                                   *width)) {
    const auto line = scalar_alu_stack_publication_line(alu, result);
    if (!line.has_value() || line->empty()) {
      return std::nullopt;
    }
    return std::vector<std::string>{*line};
  }

  const auto scratch = scalar_publication_address_scratch(result);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  const auto offset = static_cast<std::uint64_t>(*alu.result_stack_offset_bytes);
  const std::string scratch_name = abi::register_name(*scratch);
  const std::string_view base = alu.result_uses_frame_pointer_base ? "x29" : "sp";
  std::vector<std::string> lines;
  if (offset <= 4095U) {
    lines.push_back("add " + scratch_name + ", " + std::string{base} + ", #" +
                    std::to_string(offset));
  } else {
    lines = materialize_integer_constant_lines(*scratch, offset, 64);
    if (lines.empty()) {
      return std::nullopt;
    }
    lines.push_back("add " + scratch_name + ", " + std::string{base} + ", " +
                    scratch_name);
  }
  std::string_view mnemonic = "str";
  if (alu.result_type == bir::TypeKind::I1 || alu.result_type == bir::TypeKind::I8) {
    mnemonic = "strb";
  } else if (alu.result_type == bir::TypeKind::I16) {
    mnemonic = "strh";
  }
  lines.push_back(std::string{mnemonic} + " " + std::string{result} + ", [" +
                  scratch_name + "]");
  return lines;
}

[[nodiscard]] ScalarAluPrintResult append_scalar_alu_stack_publication(
    std::vector<std::string>& lines,
    const ScalarAluRecord& alu,
    std::string_view result) {
  const auto publication = scalar_alu_stack_publication_lines(alu, result);
  if (!publication.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar ALU stack publication offset is not printable"};
  }
  lines.insert(lines.end(), publication->begin(), publication->end());
  return {.lines = std::move(lines), .diagnostic = {}};
}

[[nodiscard]] bool is_unsigned_power_of_two_reduction_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::UDiv || opcode == bir::BinaryOpcode::URem;
}

[[nodiscard]] bool is_scalar_compare_opcode(bir::BinaryOpcode opcode) {
  return is_compare_predicate(opcode);
}

[[nodiscard]] std::optional<std::string_view> scalar_compare_condition(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
      return "eq";
    case bir::BinaryOpcode::Ne:
      return "ne";
    case bir::BinaryOpcode::Slt:
      return "lt";
    case bir::BinaryOpcode::Sle:
      return "le";
    case bir::BinaryOpcode::Sgt:
      return "gt";
    case bir::BinaryOpcode::Sge:
      return "ge";
    case bir::BinaryOpcode::Ult:
      return "lo";
    case bir::BinaryOpcode::Ule:
      return "ls";
    case bir::BinaryOpcode::Ugt:
      return "hi";
    case bir::BinaryOpcode::Uge:
      return "hs";
    default:
      return std::nullopt;
  }
}

struct ScalarAluOpcodeSemanticFacts {
  bool integer_alu_opcode = false;
  bool division_opcode = false;
  bool remainder_opcode = false;
  bool publication_opcode = false;
  ScalarAluOperationKind publication_operation = ScalarAluOperationKind::Deferred;
};

[[nodiscard]] ScalarAluOpcodeSemanticFacts scalar_alu_opcode_semantic_facts(
    bir::BinaryOpcode opcode) {
  const bool integer_alu_opcode = is_scalar_alu_integer_opcode(opcode);
  const bool division_opcode =
      opcode == bir::BinaryOpcode::SDiv || opcode == bir::BinaryOpcode::UDiv;
  const bool remainder_opcode =
      opcode == bir::BinaryOpcode::SRem || opcode == bir::BinaryOpcode::URem;
  const bool publication_opcode = integer_alu_opcode ||
                                  opcode == bir::BinaryOpcode::Mul ||
                                  division_opcode ||
                                  remainder_opcode;
  return ScalarAluOpcodeSemanticFacts{
      .integer_alu_opcode = integer_alu_opcode,
      .division_opcode = division_opcode,
      .remainder_opcode = remainder_opcode,
      .publication_opcode = publication_opcode,
      .publication_operation = remainder_opcode
                                   ? ScalarAluOperationKind::Div
                                   : scalar_alu_operation_from_binary_opcode(opcode),
  };
}

[[nodiscard]] bool is_scalar_alu_publication_opcode(bir::BinaryOpcode opcode) {
  const auto facts = scalar_alu_opcode_semantic_facts(opcode);
  return facts.publication_opcode;
}

[[nodiscard]] ScalarAluOperationKind scalar_alu_publication_operation(
    bir::BinaryOpcode opcode) {
  const auto facts = scalar_alu_opcode_semantic_facts(opcode);
  return facts.publication_operation;
}

[[nodiscard]] std::optional<unsigned> scalar_alu_post_sign_extend_bits(
    bir::BinaryOpcode opcode,
    bir::TypeKind operand_type,
    bir::TypeKind result_type);

struct ScalarAluSemanticFacts {
  ScalarAluOpcodeSemanticFacts opcode;
  bool scalar_operand_type = false;
  bool scalar_result_type = false;
  bool same_scalar_integer_type = false;
  bool integer_operation = false;
  bool unsigned_reduction_candidate = false;
  bool unsigned_div_rem_publication = false;
  std::optional<unsigned> post_sign_extend_result_bits;
};

[[nodiscard]] ScalarAluSemanticFacts scalar_alu_semantic_facts(
    const bir::BinaryInst& binary) {
  const auto opcode = scalar_alu_opcode_semantic_facts(binary.opcode);
  const bool scalar_operand_type = scalar_register_view(binary.operand_type).has_value();
  const bool scalar_result_type = scalar_register_view(binary.result.type).has_value();
  const bool same_scalar_integer_type =
      scalar_operand_type && scalar_result_type &&
      binary.operand_type == binary.result.type;
  return ScalarAluSemanticFacts{
      .opcode = opcode,
      .scalar_operand_type = scalar_operand_type,
      .scalar_result_type = scalar_result_type,
      .same_scalar_integer_type = same_scalar_integer_type,
      .integer_operation = scalar_operand_type && scalar_result_type &&
                           opcode.integer_alu_opcode,
      .unsigned_reduction_candidate =
          same_scalar_integer_type &&
          is_unsigned_power_of_two_reduction_opcode(binary.opcode),
      .unsigned_div_rem_publication =
          same_scalar_integer_type &&
          (binary.opcode == bir::BinaryOpcode::UDiv ||
           binary.opcode == bir::BinaryOpcode::URem),
      .post_sign_extend_result_bits = scalar_alu_post_sign_extend_bits(
          binary.opcode, binary.operand_type, binary.result.type),
  };
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

[[nodiscard]] bool scalar_registers_alias(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}

[[nodiscard]] std::optional<RegisterOperand> scalar_gp_scratch_register(
    abi::RegisterView view,
    const std::vector<const RegisterOperand*>& occupied) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    const auto viewed = abi::gp_register(scratch.index, view);
    if (!viewed.has_value()) {
      continue;
    }
    bool aliases_occupied = false;
    for (const auto* reg : occupied) {
      if (reg != nullptr && reg->reg.bank == viewed->bank &&
          reg->reg.index == viewed->index) {
        aliases_occupied = true;
        break;
      }
    }
    if (!aliases_occupied) {
      return RegisterOperand{
          .reg = *viewed,
          .role = RegisterOperandRole::ReservedMirScratch,
          .expected_view = view,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> scalar_gp_scratch_name(
    abi::RegisterView view,
    const std::vector<const RegisterOperand*>& occupied) {
  const auto scratch = scalar_gp_scratch_register(view, occupied);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(scratch->reg);
}

[[nodiscard]] bool scalar_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      memory.size_bytes == 0) {
    return false;
  }
  if (memory.size_bytes != 1 && memory.size_bytes != 2 && memory.size_bytes != 4 &&
      memory.size_bytes != 8) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % memory.size_bytes == 0 && offset / memory.size_bytes <= 4095U;
}

[[nodiscard]] std::vector<std::string> scalar_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  const std::string scratch_name = abi::register_name(scratch);
  if (offset <= 4095U) {
    return {"add " + scratch_name + ", sp, #" + std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(scratch, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + scratch_name + ", sp, " + scratch_name);
  return lines;
}

[[nodiscard]] std::optional<std::string_view> scalar_frame_slot_load_mnemonic(
    std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> scalar_frame_slot_load_target(
    std::string_view target,
    std::size_t size_bytes) {
  if (target.empty()) {
    return std::nullopt;
  }
  std::string result{target};
  if (size_bytes == 1 || size_bytes == 2 || size_bytes == 4) {
    if (result.front() == 'x') {
      result.front() = 'w';
    }
    return result;
  }
  if (size_bytes == 8) {
    return result;
  }
  return std::nullopt;
}

[[nodiscard]] bool append_scalar_frame_slot_load(
    std::vector<std::string>& lines,
    const MemoryOperand& memory,
    std::string_view target,
    abi::RegisterReference address_scratch) {
  const auto mnemonic = scalar_frame_slot_load_mnemonic(memory.size_bytes);
  const auto load_target = scalar_frame_slot_load_target(target, memory.size_bytes);
  if (!mnemonic.has_value() || !load_target.has_value()) {
    return false;
  }
  std::string address;
  if (scalar_frame_slot_direct_offset_is_encodable(memory)) {
    address = memory_address(memory);
  } else {
    auto address_lines = scalar_frame_slot_address_lines(address_scratch, memory);
    if (address_lines.empty()) {
      return false;
    }
    lines.insert(lines.end(), address_lines.begin(), address_lines.end());
    address = "[" + std::string{abi::register_name(address_scratch)} + "]";
  }
  if (address.empty()) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " + address);
  return true;
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
    if (!converted.has_value() && converted.error.has_value() &&
        converted.error->kind ==
            abi::PreparedRegisterConversionErrorKind::RegisterViewMismatch &&
        storage.bank == prepare::PreparedRegisterBank::Gpr &&
        (*expected_view == abi::RegisterView::W ||
         *expected_view == abi::RegisterView::X)) {
      const auto parsed = abi::parse_aarch64_register_name(*storage.register_name);
      if (parsed.has_value() &&
          parsed->bank == abi::RegisterBank::GeneralPurpose) {
        const auto retargeted = abi::gp_register(parsed->index, *expected_view);
        if (retargeted.has_value()) {
          converted = abi::convert_prepared_register(
              std::string(abi::register_name(*retargeted)),
              storage.bank,
              prepared_class,
              expected_view);
        }
      }
    }
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

[[nodiscard]] std::optional<RegisterOperand> make_scalar_spill_scratch_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      storage.encoding != prepare::PreparedStorageEncodingKind::FrameSlot) {
    return std::nullopt;
  }
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  auto scratch = abi::gp_register(scratches.front().index, *expected_view);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *scratch,
      .role = RegisterOperandRole::SpillAuthority,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = register_class_from_bank(storage.bank),
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = occupied_register_references(*scratch),
      .occupied_registers = occupied_register_views(*scratch),
  };
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_scalar_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const bir::Value& value,
    const module::FunctionLoweringContext& context);

[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context) {
  return context.frame_plan != nullptr &&
         context.frame_plan->uses_frame_pointer_for_fixed_slots;
}

[[nodiscard]] std::optional<MemoryOperand> make_prepared_scalar_load_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home,
    const prepare::PreparedMemoryAccess& source_access) {
  if (context.function.prepared == nullptr || home.value_name == c4c::kInvalidValueName ||
      source_access.result_value_name != std::optional<c4c::ValueNameId>{home.value_name}) {
    return std::nullopt;
  }

  if (source_access.address.base_kind == prepare::PreparedAddressBaseKind::GlobalSymbol) {
    if (!source_access.address.symbol_name.has_value()) {
      return std::nullopt;
    }
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = source_access.function_name,
        .block_label = source_access.block_label,
        .instruction_index = source_access.inst_index,
        .result_value_id = home.value_id,
        .result_value_name = home.value_name,
        .base_kind = MemoryBaseKind::Symbol,
        .symbol_name = source_access.address.symbol_name,
        .symbol_label = std::string{
            prepare::prepared_link_name(context.function.prepared->names,
                                        *source_access.address.symbol_name)},
        .byte_offset = source_access.address.byte_offset,
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_access.address.size_bytes,
        .align_bytes = source_access.address.align_bytes,
        .address_space = source_access.address_space,
        .is_volatile = source_access.is_volatile,
        .can_use_base_plus_offset = source_access.address.can_use_base_plus_offset,
    };
  }

  if (source_access.address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !source_access.address.frame_slot_id.has_value()) {
    return std::nullopt;
  }

  const auto* slot = find_scalar_frame_slot_by_id(
      context.function.prepared->stack_layout,
      *source_access.address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }

  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = source_access.function_name,
      .block_label = source_access.block_label,
      .instruction_index = source_access.inst_index,
      .result_value_id = home.value_id,
      .result_value_name = home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = source_access.address.frame_slot_id,
      .byte_offset = static_cast<std::int64_t>(slot->offset_bytes) +
                     source_access.address.byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = home.size_bytes.value_or(source_access.address.size_bytes),
      .align_bytes = home.align_bytes.value_or(source_access.address.align_bytes),
      .address_space = source_access.address_space,
      .is_volatile = source_access.is_volatile,
      .can_use_base_plus_offset = true,
      .uses_frame_pointer_base = fixed_slots_use_frame_pointer(context.function),
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_prepared_scalar_load_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr ||
      home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  const auto* source_access =
      prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
          &context.function.prepared_lookups->memory_accesses, home.value_id);
  if (source_access == nullptr) {
    return std::nullopt;
  }

  return make_prepared_scalar_load_source(context, home, *source_access);
}

[[nodiscard]] std::optional<MemoryOperand> make_prepared_stack_home_load_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  std::optional<std::int64_t> offset;
  if (context.function.storage_plan != nullptr) {
    if (const auto* storage =
            find_prepared_scalar_storage(*context.function.storage_plan, home.value_id);
        storage != nullptr &&
        storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        storage->stack_offset_bytes.has_value()) {
      offset = static_cast<std::int64_t>(*storage->stack_offset_bytes);
    }
  }
  if (!offset.has_value()) {
    if (!home.offset_bytes.has_value()) {
      return std::nullopt;
    }
    offset = static_cast<std::int64_t>(*home.offset_bytes);
  }

  const auto size_bytes = home.size_bytes.value_or(4);
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .result_value_id = home.value_id,
      .result_value_name = home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = home.slot_id,
      .byte_offset = *offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = home.align_bytes.value_or(size_bytes),
      .can_use_base_plus_offset = true,
      .uses_frame_pointer_base = fixed_slots_use_frame_pointer(context.function),
  };
}

[[nodiscard]] std::optional<prepare::PreparedScalarLoadLocalSourceProducer>
find_prepared_load_local_source_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      context.control_flow_block == nullptr ||
      context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr) {
    return std::nullopt;
  }
  const auto source =
      prepare::find_prepared_same_block_load_local_source_producer(
          context.function.prepared->names,
          context.function.prepared->stack_layout,
          &context.function.prepared_lookups->memory_accesses,
          &context.function.prepared_lookups->edge_publication_source_producers,
          context.control_flow_block->block_label,
          context.bir_block,
          value,
          before_instruction_index);
  return source;
}

[[nodiscard]] bool load_local_home_needs_consumer_publication(
    const prepare::PreparedValueHome& home) {
  if (home.kind == prepare::PreparedValueHomeKind::StackSlot) {
    return true;
  }
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return false;
  }
  const auto reg = abi::parse_aarch64_register_name(*home.register_name);
  return reg.has_value() && abi::is_gp_register(*reg);
}

[[nodiscard]] std::optional<MemoryOperand> make_unpublished_load_local_source_operand(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto source =
      find_prepared_load_local_source_producer(context, value, before_instruction_index);
  if (!source.has_value() || source->source_access == nullptr ||
      context.bir_block == nullptr) {
    return std::nullopt;
  }
  const auto* load =
      source->producer != nullptr ? source->producer->load_local : nullptr;
  if (load == nullptr &&
      source->source_access->inst_index < context.bir_block->insts.size()) {
    load = std::get_if<bir::LoadLocalInst>(
        &context.bir_block->insts[source->source_access->inst_index]);
  }
  if (load == nullptr) {
    return std::nullopt;
  }
  const auto* home = find_named_value_home(load->result, context.function);
  if (home == nullptr || !load_local_home_needs_consumer_publication(*home)) {
    return std::nullopt;
  }
  auto source_operand =
      make_prepared_scalar_load_source(context, *home, *source->source_access);
  if (!source_operand.has_value() ||
      source_operand->base_kind != MemoryBaseKind::FrameSlot ||
      source_operand->support != MemoryOperandSupportKind::Prepared ||
      !source_operand->can_use_base_plus_offset ||
      !source_operand->byte_offset_is_prepared_snapshot) {
    return std::nullopt;
  }
  if (const auto loaded_size = scalar_type_size_bytes(load->result.type)) {
    source_operand->size_bytes = *loaded_size;
    source_operand->align_bytes = *loaded_size;
  }
  return source_operand;
}

[[nodiscard]] std::string relocation_operand(std::string_view label,
                                             std::int64_t offset) {
  std::string text{label};
  if (offset > 0) {
    text += "+";
    text += std::to_string(offset);
  } else if (offset < 0) {
    text += std::to_string(offset);
  }
  return text;
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
    case bir::TypeKind::Ptr:
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

  if (const auto* memory = std::get_if<mir::Memory>(&resolved.operand.payload);
      memory != nullptr && resolved.frame_slot_id.has_value()) {
    const auto size_bytes = scalar_type_size_bytes(value.type);
    if (!size_bytes.has_value()) {
      return std::nullopt;
    }
    return make_memory_operand(MemoryOperand{
        .surface = RecordSurfaceKind::RecordOnly,
        .support = MemoryOperandSupportKind::Prepared,
        .result_value_id = resolved.value_id,
        .result_value_name = resolved.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = resolved.frame_slot_id,
        .byte_offset = memory->displacement,
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *size_bytes,
        .align_bytes = *size_bytes,
        .can_use_base_plus_offset = true,
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
  const auto expected_view = scalar_storage_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto destination_bank = scalar_fp_register_view(type).has_value()
                                    ? prepare::PreparedRegisterBank::Fpr
                                    : prepare::PreparedRegisterBank::Gpr;
  const auto* move =
      prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          context.function.move_bundle_lookups,
          context.function.value_locations,
          context.block_index,
          value_id,
          destination_bank);
  if (move == nullptr || !move->destination_register_placement.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *move->destination_register_placement, std::nullopt, expected_view);
  if (!converted.reg.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = value_id,
      .value_name = value_name,
      .expected_view = expected_view,
  };
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

[[nodiscard]] std::optional<RegisterOperand> find_return_chain_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedValueHome& result_home,
    bir::TypeKind result_type) {
  if (context.function.prepared_lookups == nullptr ||
      context.bir_block == nullptr ||
      context.function.value_locations == nullptr) {
    return std::nullopt;
  }

  const auto terminal_value_name =
      prepare::find_prepared_return_chain_terminal_value(
          &context.function.prepared_lookups->return_chains,
          context.block_index,
          instruction_index,
          result_home.value_name);
  if (terminal_value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* terminal_home =
      prepare::find_indexed_prepared_value_home(
          context.function.value_home_lookups,
          nullptr,
          context.function.value_locations,
          terminal_value_name);
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

struct ScalarFallbackOperandSelector {
  const module::BlockLoweringContext& context;
  std::size_t instruction_index;
  const BlockScalarLoweringState& scalar_state;
  module::ModuleLoweringDiagnostics& diagnostics;

  [[nodiscard]] std::optional<OperandRecord> select(const bir::Value& value) const {
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
    auto load_source =
        make_unpublished_load_local_source_operand(context, value, instruction_index);
    if (load_source.has_value()) {
      return make_memory_operand(*load_source);
    }
    if (home != nullptr) {
      auto value_home_operand = make_named_scalar_operand(value, context, diagnostics);
      if (value_home_operand.has_value()) {
        return value_home_operand;
      }
      auto source = make_prepared_scalar_load_source(context, *home);
      if (source.has_value()) {
        return make_memory_operand(*source);
      }
    }
    return make_named_scalar_operand(value, context, diagnostics);
  }
};

[[nodiscard]] std::optional<OperandRecord> make_scalar_fallback_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  return ScalarFallbackOperandSelector{
      .context = context,
      .instruction_index = instruction_index,
      .scalar_state = scalar_state,
      .diagnostics = diagnostics,
  }.select(value);
}

[[nodiscard]] std::optional<OperandRecord> make_control_publication_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    bool allow_prepared_load_source) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return make_immediate_scalar_operand(value);
  }
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, home->value_name);
  if (emitted.has_value() && !abi::is_reserved_mir_scratch(emitted->reg)) {
    return make_register_operand(*emitted);
  }
  if (allow_prepared_load_source) {
    auto source = make_prepared_scalar_load_source(context, *home);
    if (source.has_value()) {
      return make_memory_operand(*source);
    }
    source = make_prepared_stack_home_load_source(context, *home);
    if (source.has_value()) {
      return make_memory_operand(*source);
    }
  }
  if (emitted.has_value()) {
    return make_register_operand(*emitted);
  }
  if (home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    auto source = make_prepared_scalar_load_source(context, *home);
    if (source.has_value() && context.control_flow_block != nullptr &&
        source->block_label == context.control_flow_block->block_label) {
      return make_memory_operand(*source);
    }
    source = make_prepared_stack_home_load_source(context, *home);
    if (source.has_value()) {
      return make_memory_operand(*source);
    }
  }
  return std::nullopt;
}

[[nodiscard]] PreparedScalarAluRecordError control_prepared_scalar_result_operand(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& result,
    RegisterOperand& out,
    std::optional<std::int64_t>& result_stack_offset_bytes) {
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
  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::Register) {
    const auto result_register = make_prepared_scalar_register_operand(
        *result_home, *result_storage, result.type, RegisterOperandRole::StoragePlan);
    if (!result_register.has_value()) {
      return PreparedScalarAluRecordError::RegisterConversionFailed;
    }
    out = *result_register;
    return PreparedScalarAluRecordError::None;
  }
  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      result_storage->stack_offset_bytes.has_value()) {
    const auto scratch =
        make_scalar_spill_scratch_operand(*result_home, *result_storage, result.type);
    if (!scratch.has_value()) {
      return PreparedScalarAluRecordError::RegisterConversionFailed;
    }
    out = *scratch;
    result_stack_offset_bytes =
        static_cast<std::int64_t>(*result_storage->stack_offset_bytes);
    return PreparedScalarAluRecordError::None;
  }
  return PreparedScalarAluRecordError::UnsupportedResultStorage;
}

struct MaterializedControlSource {
  std::string name;
  std::optional<RegisterOperand> occupied_register;
};

[[nodiscard]] std::optional<MaterializedControlSource> materialize_control_register_source(
    const OperandRecord& operand,
    abi::RegisterView view,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (operand.kind == OperandKind::Register) {
    const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
    if (reg == nullptr) {
      return std::nullopt;
    }
    const auto name = scalar_gp_register_name_with_view(*reg, view);
    if (!name.has_value()) {
      return std::nullopt;
    }
    return MaterializedControlSource{.name = *name, .occupied_register = *reg};
  }
  if (operand.kind == OperandKind::Memory) {
    const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
    if (memory == nullptr || memory->support != MemoryOperandSupportKind::Prepared ||
        !memory->can_use_base_plus_offset ||
        !memory->byte_offset_is_prepared_snapshot) {
      return std::nullopt;
    }
    const auto scratch = scalar_gp_scratch_register(view, occupied);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    const std::string name = abi::register_name(scratch->reg);
    if (memory->base_kind == MemoryBaseKind::FrameSlot) {
      if (!append_scalar_frame_slot_load(lines,
                                         *memory,
                                         name,
                                         abi::x_register(scratch->reg.index))) {
        return std::nullopt;
      }
    } else if (memory->base_kind == MemoryBaseKind::Symbol) {
      if (memory->symbol_label.empty()) {
        return std::nullopt;
      }
      const auto address_name =
          scalar_gp_register_name_with_view(*scratch, abi::RegisterView::X);
      if (!address_name.has_value()) {
        return std::nullopt;
      }
      const std::string reloc =
          relocation_operand(memory->symbol_label, memory->byte_offset);
      std::ostringstream page;
      page << "adrp " << *address_name << ", " << reloc;
      lines.push_back(page.str());
      std::ostringstream low;
      low << "add " << *address_name << ", " << *address_name << ", :lo12:" << reloc;
      lines.push_back(low.str());
      std::ostringstream load;
      load << "ldr " << name << ", [" << *address_name << "]";
      lines.push_back(load.str());
    } else {
      return std::nullopt;
    }
    return MaterializedControlSource{.name = name, .occupied_register = *scratch};
  }
  if (operand.kind == OperandKind::Immediate) {
    const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
    if (immediate == nullptr) {
      return std::nullopt;
    }
    const auto scratch = scalar_gp_scratch_register(view, occupied);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    const std::string name = abi::register_name(scratch->reg);
    std::ostringstream move;
    move << "mov " << name << ", " << scalar_immediate_name(*immediate);
    lines.push_back(move.str());
    return MaterializedControlSource{.name = name, .occupied_register = *scratch};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<MaterializedControlSource> materialize_control_compare_rhs(
    const OperandRecord& operand,
    abi::RegisterView view,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (operand.kind == OperandKind::Immediate) {
    const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
    if (immediate == nullptr || immediate->signed_value < 0 ||
        immediate->signed_value > 4095) {
      return std::nullopt;
    }
    return MaterializedControlSource{
        .name = scalar_immediate_name(*immediate),
        .occupied_register = std::nullopt,
    };
  }
  return materialize_control_register_source(operand, view, lines, std::move(occupied));
}

[[nodiscard]] std::optional<MaterializedControlSource>
materialize_control_binary_result_source(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterView view,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied);

[[nodiscard]] bool append_control_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterView view,
    std::string_view target,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied);

[[nodiscard]] std::optional<prepare::PreparedSameBlockScalarProducer>
find_prepared_control_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      (context.function.prepared_lookups == nullptr &&
       context.function.control_flow == nullptr) ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto generated_lookups =
      context.function.prepared_lookups == nullptr
          ? std::optional<prepare::PreparedFunctionLookups>{
                prepare::make_prepared_function_lookups(
                    *context.function.prepared, *context.function.control_flow)}
          : std::nullopt;
  const auto* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : &generated_lookups->edge_publication_source_producers;
  return prepare::find_prepared_same_block_scalar_producer(
      context.function.prepared->names,
      source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] bool append_move_control_value_to_register(
    const OperandRecord& operand,
    abi::RegisterView view,
    std::string_view target,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (operand.kind == OperandKind::Memory) {
    const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
    if (memory == nullptr || memory->support != MemoryOperandSupportKind::Prepared ||
        !memory->can_use_base_plus_offset ||
        !memory->byte_offset_is_prepared_snapshot) {
      return false;
    }
    if (memory->base_kind == MemoryBaseKind::FrameSlot) {
      if (scalar_frame_slot_direct_offset_is_encodable(*memory)) {
        return append_scalar_frame_slot_load(
            lines, *memory, target, abi::reserved_mir_scratch_gp_registers().front());
      }
      const auto address_scratch =
          scalar_gp_scratch_register(abi::RegisterView::X, occupied);
      if (!address_scratch.has_value() ||
          !append_scalar_frame_slot_load(lines,
                                         *memory,
                                         target,
                                         abi::x_register(address_scratch->reg.index))) {
        return false;
      }
      return true;
    }
    if (memory->base_kind == MemoryBaseKind::Symbol) {
      if (memory->symbol_label.empty()) {
        return false;
      }
      const auto address = scalar_gp_scratch_register(abi::RegisterView::X, occupied);
      if (!address.has_value()) {
        return false;
      }
      const std::string address_name = abi::register_name(address->reg);
      const std::string reloc =
          relocation_operand(memory->symbol_label, memory->byte_offset);
      std::ostringstream page;
      page << "adrp " << address_name << ", " << reloc;
      lines.push_back(page.str());
      std::ostringstream low;
      low << "add " << address_name << ", " << address_name << ", :lo12:" << reloc;
      lines.push_back(low.str());
      std::ostringstream load;
      load << "ldr " << target << ", [" << address_name << "]";
      lines.push_back(load.str());
      return true;
    }
    return false;
  }
  if (operand.kind == OperandKind::Immediate) {
    const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
    if (immediate == nullptr) {
      return false;
    }
    std::ostringstream move;
    move << "mov " << target << ", " << scalar_immediate_name(*immediate);
    lines.push_back(move.str());
    return true;
  }
  const auto source =
      materialize_control_register_source(operand, view, lines, std::move(occupied));
  if (!source.has_value()) {
    return false;
  }
  if (source->name == target) {
    return true;
  }
  std::ostringstream move;
  move << "mov " << target << ", " << source->name;
  lines.push_back(move.str());
  return true;
}

[[nodiscard]] bool append_control_binary_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    abi::RegisterView view,
    std::string_view target,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (binary.operand_type != binary.result.type ||
      scalar_register_view(binary.operand_type) != std::optional<abi::RegisterView>{view}) {
    return false;
  }
  auto lhs_operand = make_control_publication_operand(
      binary.lhs, context, scalar_state, true);
  auto rhs_operand = make_control_publication_operand(
      binary.rhs, context, scalar_state, true);
  if (!lhs_operand.has_value() || !rhs_operand.has_value()) {
    return false;
  }
  const auto lhs =
      materialize_control_register_source(*lhs_operand, view, lines, occupied);
  if (!lhs.has_value()) {
    return false;
  }
  const auto* rhs_register = std::get_if<RegisterOperand>(&rhs_operand->payload);
  std::vector<const RegisterOperand*> rhs_scratch_occupied = occupied;
  if (lhs->occupied_register.has_value()) {
    rhs_scratch_occupied.push_back(&*lhs->occupied_register);
  }
  std::optional<std::string> rhs;
  if (rhs_operand->kind == OperandKind::Immediate) {
    const auto* immediate = std::get_if<ImmediateOperand>(&rhs_operand->payload);
    if (immediate == nullptr) {
      return false;
    }
    std::optional<RegisterOperand> scratch;
    std::string rhs_name;
    if (lhs->name != target) {
      rhs_name = std::string{target};
    } else {
      scratch = scalar_gp_scratch_register(view, rhs_scratch_occupied);
      if (!scratch.has_value()) {
        return false;
      }
      rhs_name = abi::register_name(scratch->reg);
    }
    std::ostringstream move;
    move << "mov " << rhs_name << ", " << scalar_immediate_name(*immediate);
    lines.push_back(move.str());
    if (scratch.has_value()) {
      rhs_operand = make_register_operand(*scratch);
    } else {
      rhs = rhs_name;
    }
  }

  std::vector<const RegisterOperand*> rhs_occupied = occupied;
  if (lhs->occupied_register.has_value()) {
    rhs_occupied.push_back(&*lhs->occupied_register);
  }
  if (!rhs.has_value()) {
    const auto materialized_rhs =
        materialize_control_register_source(*rhs_operand, view, lines, std::move(rhs_occupied));
    if (materialized_rhs.has_value()) {
      rhs = materialized_rhs->name;
    }
  }
  if (!rhs.has_value()) {
    return false;
  }

  std::string_view mnemonic;
  switch (binary.opcode) {
    case bir::BinaryOpcode::Add:
      mnemonic = "add";
      break;
    case bir::BinaryOpcode::Sub:
      mnemonic = "sub";
      break;
    case bir::BinaryOpcode::And:
      mnemonic = "and";
      break;
    case bir::BinaryOpcode::Or:
      mnemonic = "orr";
      break;
    case bir::BinaryOpcode::Xor:
      mnemonic = "eor";
      break;
    case bir::BinaryOpcode::Mul:
      mnemonic = "mul";
      break;
    default:
      return false;
  }
  (void)rhs_register;
  std::ostringstream out;
  out << mnemonic << " " << target << ", " << lhs->name << ", " << *rhs;
  lines.push_back(out.str());
  return true;
}

[[nodiscard]] bool append_control_cast_to_register(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t before_instruction_index,
    abi::RegisterView view,
    const RegisterOperand& target_register,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (scalar_register_view(cast.result.type) != std::optional<abi::RegisterView>{view}) {
    return false;
  }
  const auto source_view = scalar_register_view(cast.operand.type);
  const auto source_bits = integer_scalar_bit_width(cast.operand.type);
  const auto result_bits = integer_scalar_bit_width(cast.result.type);
  if (!source_view.has_value() || !source_bits.has_value() ||
      !result_bits.has_value()) {
    return false;
  }
  const auto source_register = abi::gp_register(target_register.reg.index, *source_view);
  if (!source_register.has_value()) {
    return false;
  }
  const std::string source_name = abi::register_name(*source_register);
  if (!append_control_value_to_register(context,
                                        cast.operand,
                                        before_instruction_index,
                                        *source_view,
                                        source_name,
                                        scalar_state,
                                        lines,
                                        std::move(occupied))) {
    return false;
  }
  const std::string target_name = abi::register_name(target_register.reg);
  switch (cast.opcode) {
    case bir::CastOpcode::SExt:
      if (*source_bits >= *result_bits) {
        return false;
      }
      if (*source_bits == 8U) {
        lines.push_back("sxtb " + target_name + ", " + source_name);
      } else if (*source_bits == 16U) {
        lines.push_back("sxth " + target_name + ", " + source_name);
      } else if (*source_bits == 32U && *result_bits == 64U) {
        lines.push_back("sxtw " + target_name + ", " + source_name);
      } else {
        return false;
      }
      return true;
    case bir::CastOpcode::ZExt:
      if (*source_bits < 32U) {
        const auto widened = abi::gp_register(target_register.reg.index,
                                              abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        lines.push_back("ubfx " + abi::register_name(*widened) + ", " +
                        abi::register_name(*widened) + ", #0, #" +
                        std::to_string(*source_bits));
      }
      return *source_bits <= *result_bits;
    case bir::CastOpcode::Trunc:
      return *source_bits > *result_bits;
    default:
      return false;
  }
}

[[nodiscard]] std::optional<MaterializedControlSource>
materialize_control_binary_result_source(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterView view,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto scratch = scalar_gp_scratch_register(view, occupied);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  const std::string target = abi::register_name(scratch->reg);
  const auto producer =
      find_prepared_control_same_block_scalar_producer(
          context, value, before_instruction_index);
  if (producer.has_value() &&
      producer->producer.kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::Binary &&
      producer->producer.binary != nullptr) {
    if (!append_control_binary_to_register(context,
                                           *producer->producer.binary,
                                           view,
                                           target,
                                           scalar_state,
                                           lines,
                                           occupied)) {
      return std::nullopt;
    }
    return MaterializedControlSource{.name = target, .occupied_register = *scratch};
  }
  if (producer.has_value() &&
      producer->producer.kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::Cast &&
      producer->producer.cast != nullptr) {
    if (!append_control_cast_to_register(context,
                                         *producer->producer.cast,
                                         before_instruction_index,
                                         view,
                                         *scratch,
                                         scalar_state,
                                         lines,
                                         std::move(occupied))) {
      return std::nullopt;
    }
    return MaterializedControlSource{.name = target, .occupied_register = *scratch};
  }
  return std::nullopt;
}

[[nodiscard]] bool append_control_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    abi::RegisterView view,
    std::string_view target,
    BlockScalarLoweringState& scalar_state,
    std::vector<std::string>& lines,
    std::vector<const RegisterOperand*> occupied) {
  if (value.kind == bir::Value::Kind::Named) {
    const auto producer =
        find_prepared_control_same_block_scalar_producer(
            context, value, before_instruction_index);
    if (producer.has_value() &&
        producer->producer.kind ==
            prepare::PreparedEdgePublicationSourceProducerKind::Binary &&
        producer->producer.binary != nullptr) {
      const auto original_line_count = lines.size();
      if (append_control_binary_to_register(
              context,
              *producer->producer.binary,
              view,
              target,
              scalar_state,
              lines,
              occupied)) {
        return true;
      }
      lines.resize(original_line_count);
      if (const auto* home = find_named_value_home(value, context.function);
          home != nullptr) {
        if (auto source = make_prepared_stack_home_load_source(context, *home);
            source.has_value()) {
          auto operand = make_memory_operand(*source);
          return append_move_control_value_to_register(
              operand, view, target, lines, std::move(occupied));
        }
      }
    }
    if (producer.has_value() &&
        producer->producer.kind ==
            prepare::PreparedEdgePublicationSourceProducerKind::Cast &&
        producer->producer.cast != nullptr) {
      const auto target_register =
          abi::parse_aarch64_register_name(std::string{target});
      if (!target_register.has_value() || !abi::is_gp_register(*target_register)) {
        return false;
      }
      const auto original_line_count = lines.size();
      if (append_control_cast_to_register(context,
                                          *producer->producer.cast,
                                          before_instruction_index,
                                          view,
                                          RegisterOperand{.reg = *target_register},
                                          scalar_state,
                                          lines,
                                          occupied)) {
        return true;
      }
      lines.resize(original_line_count);
    }
    if (auto source = make_unpublished_load_local_source_operand(
            context, value, before_instruction_index);
        source.has_value()) {
      auto operand = make_memory_operand(*source);
      return append_move_control_value_to_register(
          operand, view, target, lines, std::move(occupied));
    }
  }
  auto operand = make_control_publication_operand(value, context, scalar_state, true);
  return operand.has_value() &&
         append_move_control_value_to_register(
             *operand, view, target, lines, std::move(occupied));
}

[[nodiscard]] module::MachineInstruction make_control_publication_assembler(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .inline_asm_template = std::move(asm_text),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_compare_publication(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    bool allow_prepared_load_source) {
  if (!is_scalar_compare_opcode(binary.opcode) ||
      context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      binary.result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto condition = scalar_compare_condition(binary.opcode);
  const auto operand_view = scalar_register_view(binary.operand_type);
  if (!condition.has_value() || !operand_view.has_value()) {
    return std::nullopt;
  }
  RegisterOperand result_register;
  std::optional<std::int64_t> result_stack_offset_bytes;
  if (control_prepared_scalar_result_operand(context.function.prepared->names,
                                             *context.function.value_locations,
                                             *context.function.storage_plan,
                                             binary.result,
                                             result_register,
                                             result_stack_offset_bytes) !=
      PreparedScalarAluRecordError::None) {
    return std::nullopt;
  }
  const auto result = scalar_gp_register_name_with_view(
      result_register, scalar_register_view(binary.result.type).value_or(*operand_view));
  if (!result.has_value()) {
    return std::nullopt;
  }
  (void)diagnostics;
  auto lhs_operand = make_control_publication_operand(
      binary.lhs, context, scalar_state, allow_prepared_load_source);
  auto rhs_operand = make_control_publication_operand(
      binary.rhs, context, scalar_state, allow_prepared_load_source);
  if (!lhs_operand.has_value() || !rhs_operand.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  const auto lhs = materialize_control_register_source(
      *lhs_operand, *operand_view, lines, {});
  if (!lhs.has_value()) {
    return std::nullopt;
  }
  std::vector<const RegisterOperand*> rhs_occupied{&result_register};
  if (lhs->occupied_register.has_value()) {
    rhs_occupied.push_back(&*lhs->occupied_register);
  }
  const auto rhs = materialize_control_compare_rhs(
      *rhs_operand, *operand_view, lines, std::move(rhs_occupied));
  if (!rhs.has_value()) {
    return std::nullopt;
  }
  std::ostringstream cmp;
  cmp << "cmp " << lhs->name << ", " << rhs->name;
  lines.push_back(cmp.str());
  std::ostringstream cset;
  cset << "cset " << *result << ", " << *condition;
  lines.push_back(cset.str());
  const auto publication = scalar_alu_stack_publication_lines(
      ScalarAluRecord{.result_type = binary.result.type,
                      .result_stack_offset_bytes = result_stack_offset_bytes,
                      .result_uses_frame_pointer_base =
                          context.function.frame_plan != nullptr &&
                          context.function.frame_plan->uses_frame_pointer_for_fixed_slots},
      *result);
  if (!publication.has_value()) {
    return std::nullopt;
  }
  lines.insert(lines.end(), publication->begin(), publication->end());
  record_emitted_scalar_register(scalar_state,
                                 result_register.value_name,
                                 result_register);
  return make_control_publication_assembler(context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_select_publication(
    const module::BlockLoweringContext& context,
    const bir::SelectInst& select,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      select.result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto condition = scalar_compare_condition(select.predicate);
  const auto compare_view = scalar_register_view(select.compare_type);
  const auto result_view = scalar_register_view(select.result.type);
  if (!condition.has_value() || !compare_view.has_value() ||
      !result_view.has_value()) {
    return std::nullopt;
  }
  RegisterOperand result_register;
  std::optional<std::int64_t> result_stack_offset_bytes;
  if (control_prepared_scalar_result_operand(context.function.prepared->names,
                                             *context.function.value_locations,
                                             *context.function.storage_plan,
                                             select.result,
                                             result_register,
                                             result_stack_offset_bytes) !=
      PreparedScalarAluRecordError::None) {
    return std::nullopt;
  }
  const auto result = scalar_gp_register_name_with_view(result_register, *result_view);
  if (!result.has_value()) {
    return std::nullopt;
  }
  const prepare::PreparedEdgePublicationSourceProducerLookups* source_producers =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publication_source_producers
          : nullptr;
  const auto select_chain_materialization =
      context.control_flow_block != nullptr
          ? prepare::find_prepared_scalar_select_chain_materialization(
                context.function.prepared->names,
                source_producers,
                context.control_flow_block->block_label,
                context.bir_block,
                select.result,
                instruction_index + 1U)
          : prepare::PreparedScalarSelectChainMaterialization{};
  if (select_chain_materialization.available &&
      select_chain_materialization.direct_global_dependency
          .contains_direct_global_load &&
      select_chain_materialization.direct_global_dependency
          .root_instruction_index.has_value()) {
    const auto scratch = scalar_gp_scratch_register(*result_view, {&result_register});
    if (scratch.has_value()) {
      std::vector<std::string> lines;
      const std::size_t package_index = instruction_index + 1U;
      std::size_t label_index = 0;
      std::vector<std::string_view> active_values;
      if (emit_select_chain_value_to_register(context,
                                              select.result,
                                              instruction_index + 1U,
                                              result_register.reg.index,
                                              scratch->reg.index,
                                              *select_chain_materialization
                                                   .direct_global_dependency
                                                   .root_instruction_index,
                                              select_chain_materialization.root_value_name,
                                              package_index,
                                              lines,
                                              label_index,
                                              active_values,
                                              true,
                                              &select_chain_materialization
                                                   .direct_global_dependency)) {
        const auto publication = scalar_alu_stack_publication_lines(
            ScalarAluRecord{.result_type = select.result.type,
                            .result_stack_offset_bytes = result_stack_offset_bytes,
                            .result_uses_frame_pointer_base =
                                context.function.frame_plan != nullptr &&
                                context.function.frame_plan->uses_frame_pointer_for_fixed_slots},
            *result);
        if (publication.has_value()) {
          lines.insert(lines.end(), publication->begin(), publication->end());
          (void)diagnostics;
          record_emitted_scalar_register(scalar_state,
                                         result_register.value_name,
                                         result_register);
          return make_control_publication_assembler(
              context, instruction_index, std::move(lines));
        }
      }
    }
  }
  auto lhs_operand = make_control_publication_operand(
      select.lhs, context, scalar_state, true);
  auto rhs_operand = make_control_publication_operand(
      select.rhs, context, scalar_state, true);

  std::vector<std::string> lines;
  const std::vector<const RegisterOperand*> lhs_occupied;
  const auto lhs = lhs_operand.has_value()
                       ? materialize_control_register_source(
                             *lhs_operand, *compare_view, lines, lhs_occupied)
                       : materialize_control_binary_result_source(context,
                                                                  select.lhs,
                                                                  instruction_index,
                                                                  *compare_view,
                                                                  scalar_state,
                                                                  lines,
                                                                  lhs_occupied);
  if (!lhs.has_value()) {
    return std::nullopt;
  }
  std::vector<const RegisterOperand*> compare_occupied{&result_register};
  if (lhs->occupied_register.has_value()) {
    compare_occupied.push_back(&*lhs->occupied_register);
  }
  const auto rhs = rhs_operand.has_value()
                       ? materialize_control_compare_rhs(*rhs_operand,
                                                         *compare_view,
                                                         lines,
                                                         compare_occupied)
                       : materialize_control_binary_result_source(context,
                                                                  select.rhs,
                                                                  instruction_index,
                                                                  *compare_view,
                                                                  scalar_state,
                                                                  lines,
                                                                  compare_occupied);
  if (!rhs.has_value()) {
    return std::nullopt;
  }
  std::ostringstream cmp;
  cmp << "cmp " << lhs->name << ", " << rhs->name;
  lines.push_back(cmp.str());

  const auto true_scratch = scalar_gp_scratch_register(*result_view, {&result_register});
  if (!true_scratch.has_value()) {
    return std::nullopt;
  }
  const std::string true_name = abi::register_name(true_scratch->reg);
  if (!append_control_value_to_register(context,
                                        select.false_value,
                                        instruction_index,
                                        *result_view,
                                        *result,
                                        scalar_state,
                                        lines,
                                        {&result_register, &*true_scratch})) {
    return std::nullopt;
  }
  if (!append_control_value_to_register(context,
                                        select.true_value,
                                        instruction_index,
                                        *result_view,
                                        true_name,
                                        scalar_state,
                                        lines,
                                        {&result_register, &*true_scratch})) {
    return std::nullopt;
  }

  std::ostringstream csel;
  csel << "csel " << *result << ", " << true_name << ", " << *result << ", "
       << *condition;
  lines.push_back(csel.str());
  const auto publication = scalar_alu_stack_publication_lines(
      ScalarAluRecord{.result_type = select.result.type,
                      .result_stack_offset_bytes = result_stack_offset_bytes,
                      .result_uses_frame_pointer_base =
                          context.function.frame_plan != nullptr &&
                          context.function.frame_plan->uses_frame_pointer_for_fixed_slots},
      *result);
  if (!publication.has_value()) {
    return std::nullopt;
  }
  lines.insert(lines.end(), publication->begin(), publication->end());
  (void)diagnostics;
  record_emitted_scalar_register(scalar_state,
                                 result_register.value_name,
                                 result_register);
  return make_control_publication_assembler(context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<ImmediateOperand> authoritative_immediate_storage(
    const bir::Value& value,
    const module::FunctionLoweringContext& context) {
  const auto* home = find_named_value_home(value, context);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate ||
      !home->immediate_i32.has_value() || context.storage_plan == nullptr) {
    return std::nullopt;
  }
  const auto* storage = find_prepared_scalar_storage(*context.storage_plan,
                                                     home->value_id);
  if (storage == nullptr ||
      storage->value_name != home->value_name ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !storage->immediate_i32.has_value() ||
      *storage->immediate_i32 != *home->immediate_i32) {
    return std::nullopt;
  }
  return make_scalar_immediate_operand(
      bir::Value::immediate_i32(static_cast<std::int32_t>(*storage->immediate_i32)),
      home->value_id,
      home->value_name);
}

[[nodiscard]] bool uses_unemitted_authoritative_immediate(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state) {
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr ||
      find_emitted_scalar_register(scalar_state, home->value_name).has_value()) {
    return false;
  }
  return authoritative_immediate_storage(value, context.function).has_value();
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
  struct MaterializedScalarRegisterSource {
    std::string name;
    std::optional<RegisterOperand> scratch;
  };

  auto materialize_scalar_register_source =
      [&](const OperandRecord& operand,
          const RegisterOperand* reg,
          const MemoryOperand* memory,
          const ImmediateOperand* immediate,
          abi::RegisterView view,
          std::vector<const RegisterOperand*> occupied,
          std::vector<std::string>& lines)
      -> std::optional<MaterializedScalarRegisterSource> {
    if (operand.kind == OperandKind::Register && reg != nullptr) {
      const auto name = scalar_gp_register_name_with_view(*reg, view);
      if (!name.has_value()) {
        return std::nullopt;
      }
      return MaterializedScalarRegisterSource{.name = *name};
    }
    if (operand.kind == OperandKind::Memory && memory != nullptr) {
      if (memory->base_kind != MemoryBaseKind::FrameSlot ||
          memory->support != MemoryOperandSupportKind::Prepared ||
          !memory->can_use_base_plus_offset ||
          !memory->byte_offset_is_prepared_snapshot) {
        return std::nullopt;
      }
      const auto scratch = scalar_gp_scratch_register(view, occupied);
      if (!scratch.has_value()) {
        return std::nullopt;
      }
      const std::string scratch_name = abi::register_name(scratch->reg);
      if (!append_scalar_frame_slot_load(lines,
                                         *memory,
                                         scratch_name,
                                         abi::x_register(scratch->reg.index))) {
        return std::nullopt;
      }
      return MaterializedScalarRegisterSource{.name = scratch_name, .scratch = scratch};
    }
    if (operand.kind == OperandKind::Immediate && immediate != nullptr) {
      const auto scratch = scalar_gp_scratch_register(view, occupied);
      if (!scratch.has_value()) {
        return std::nullopt;
      }
      const std::string scratch_name = abi::register_name(scratch->reg);
      std::ostringstream move;
      move << "mov " << scratch_name << ", " << scalar_immediate_name(*immediate);
      lines.push_back(move.str());
      return MaterializedScalarRegisterSource{.name = scratch_name, .scratch = scratch};
    }
    return std::nullopt;
  };

  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_integer_operation &&
      (scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::Mul ||
       scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::SDiv ||
       scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::SRem ||
       ((scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::UDiv ||
         scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::URem) &&
        scalar.scalar_alu->operation == ScalarAluOperationKind::Div))) {
    const auto& alu = *scalar.scalar_alu;
    if (scalar.inputs.size() != 2) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar mul/div/rem node requires two structured operands"};
    }
    const auto result_view = scalar_register_view(alu.result_type);
    const auto operand_view = scalar_register_view(alu.operand_type);
    if (!result_view.has_value() || result_view != operand_view) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar mul/div/rem node requires matching integer widths"};
    }
    const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
    const auto* lhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[0].payload);
    const auto* rhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[1].payload);
    const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
    const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
    const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                                 lhs_register != nullptr;
    const bool lhs_is_memory = scalar.inputs[0].kind == OperandKind::Memory &&
                               lhs_memory != nullptr;
    const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                  lhs_immediate != nullptr;
    const bool rhs_is_register = scalar.inputs[1].kind == OperandKind::Register &&
                                 rhs_register != nullptr;
    const bool rhs_is_memory = scalar.inputs[1].kind == OperandKind::Memory &&
                               rhs_memory != nullptr;
    const bool rhs_is_immediate = scalar.inputs[1].kind == OperandKind::Immediate &&
                                  rhs_immediate != nullptr;
    if ((!lhs_is_register && !lhs_is_memory && !lhs_is_immediate) ||
        (!rhs_is_register && !rhs_is_memory && !rhs_is_immediate) ||
        !scalar.result_register.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar mul/div/rem node requires register/memory/immediate lhs, register/memory/immediate rhs, and result register"};
    }
    const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
    if (!result.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar mul/div/rem node has incomplete printable result register facts"};
    }

    const bool is_remainder_opcode = alu.source_binary_opcode == bir::BinaryOpcode::SRem ||
                                     alu.source_binary_opcode == bir::BinaryOpcode::URem;
    std::vector<std::string> lines;
    std::vector<const RegisterOperand*> lhs_occupied;
    if (lhs_is_register) {
      lhs_occupied.push_back(&*scalar.result_register);
    }
    if (rhs_register != nullptr) {
      lhs_occupied.push_back(rhs_register);
    }
    const auto lhs = materialize_scalar_register_source(
        scalar.inputs[0],
        lhs_register,
        lhs_memory,
        lhs_immediate,
        *operand_view,
        lhs_occupied,
        lines);
    if (!lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar mul/div/rem node has incomplete printable lhs facts"};
    }
    std::optional<std::string> rhs;
    std::optional<RegisterOperand> rhs_scratch;
    if (rhs_is_register || rhs_is_memory) {
      std::vector<const RegisterOperand*> rhs_occupied{&*scalar.result_register};
      if (lhs_register != nullptr) {
        rhs_occupied.push_back(lhs_register);
      }
      if (lhs->scratch.has_value()) {
        rhs_occupied.push_back(&*lhs->scratch);
      }
      const auto rhs_source = materialize_scalar_register_source(
          scalar.inputs[1],
          rhs_register,
          rhs_memory,
          nullptr,
          *operand_view,
          rhs_occupied,
          lines);
      if (!rhs_source.has_value()) {
        return {.lines = std::nullopt,
                .diagnostic =
                    "scalar mul/div/rem node has incomplete printable rhs facts"};
      }
      rhs = rhs_source->name;
      rhs_scratch = rhs_source->scratch;
    } else {
      const bool result_aliases_lhs =
          lhs_register != nullptr && scalar_registers_alias(*scalar.result_register,
                                                           *lhs_register);
      std::vector<const RegisterOperand*> occupied{&*scalar.result_register, lhs_register};
      if (lhs->scratch.has_value()) {
        occupied.push_back(&*lhs->scratch);
      }
      if (is_remainder_opcode || result_aliases_lhs) {
        const auto scratch = scalar_gp_scratch_register(*operand_view, occupied);
        if (!scratch.has_value()) {
          return {.lines = std::nullopt,
                  .diagnostic =
                      "scalar mul/div/rem node needs an available scratch register"};
        }
        rhs_scratch = scratch;
        rhs = abi::register_name(scratch->reg);
      } else {
        rhs = result;
      }
      std::ostringstream move;
      move << "mov " << *rhs << ", " << scalar_immediate_name(*rhs_immediate);
      lines.push_back(move.str());
    }

    if (alu.source_binary_opcode == bir::BinaryOpcode::Mul) {
      std::ostringstream out;
      out << "mul " << *result << ", " << lhs->name << ", " << *rhs;
      lines.push_back(out.str());
      return append_scalar_alu_stack_publication(lines, alu, *result);
    }
    if (alu.source_binary_opcode == bir::BinaryOpcode::SDiv ||
        alu.source_binary_opcode == bir::BinaryOpcode::UDiv) {
      std::ostringstream out;
      out << (alu.source_binary_opcode == bir::BinaryOpcode::UDiv ? "udiv " : "sdiv ")
          << *result << ", " << lhs->name << ", " << *rhs;
      lines.push_back(out.str());
      return append_scalar_alu_stack_publication(lines, alu, *result);
    }
    if (alu.source_binary_opcode == bir::BinaryOpcode::SRem ||
        alu.source_binary_opcode == bir::BinaryOpcode::URem) {
      std::optional<std::string> divisor = rhs;
      std::optional<RegisterOperand> divisor_scratch = rhs_scratch;
      if (rhs_register != nullptr && scalar_registers_alias(*scalar.result_register,
                                                            *rhs_register)) {
        std::vector<const RegisterOperand*> occupied{&*scalar.result_register, lhs_register};
        if (lhs->scratch.has_value()) {
          occupied.push_back(&*lhs->scratch);
        }
        const auto scratch = scalar_gp_scratch_register(*operand_view, occupied);
        if (!scratch.has_value()) {
          return {.lines = std::nullopt,
                  .diagnostic =
                      "scalar rem node needs scratch when result aliases divisor"};
        }
        std::ostringstream move;
        const std::string scratch_name = abi::register_name(scratch->reg);
        move << "mov " << scratch_name << ", " << *rhs;
        lines.push_back(move.str());
        divisor = scratch_name;
        divisor_scratch = scratch;
      }
      std::optional<std::string> quotient = result;
      if (lhs_register != nullptr &&
          scalar_registers_alias(*scalar.result_register, *lhs_register)) {
        std::vector<const RegisterOperand*> occupied{&*scalar.result_register};
        if (rhs_register != nullptr) {
          occupied.push_back(rhs_register);
        }
        if (divisor_scratch.has_value()) {
          occupied.push_back(&*divisor_scratch);
        }
        const auto scratch = scalar_gp_scratch_name(*operand_view, occupied);
        if (!scratch.has_value()) {
          return {.lines = std::nullopt,
                  .diagnostic =
                      "scalar rem node needs scratch when result aliases dividend"};
        }
        quotient = scratch;
      }
      std::ostringstream div;
      div << (alu.source_binary_opcode == bir::BinaryOpcode::URem ? "udiv " : "sdiv ")
          << *quotient << ", " << lhs->name << ", " << *divisor;
      lines.push_back(div.str());
      std::ostringstream msub;
      msub << "msub " << *result << ", " << *quotient << ", " << *divisor << ", "
           << lhs->name;
      lines.push_back(msub.str());
      return append_scalar_alu_stack_publication(lines, alu, *result);
    }
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
    const auto* lhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[0].payload);
    const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
    const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
    const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                                 lhs_register != nullptr;
    const bool lhs_is_memory = scalar.inputs[0].kind == OperandKind::Memory &&
                               lhs_memory != nullptr;
    const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                  lhs_immediate != nullptr;
    if ((!lhs_is_register && !lhs_is_memory && !lhs_is_immediate) ||
        scalar.inputs[1].kind != OperandKind::Immediate || rhs_immediate == nullptr ||
        !scalar.result_register.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node requires register/memory/immediate lhs, immediate reduction, and result register"};
    }
    const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
    if (!result.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node has incomplete printable result register facts"};
    }
    std::vector<std::string> lines;
    const auto lhs = materialize_scalar_register_source(
        scalar.inputs[0],
        lhs_register,
        lhs_memory,
        lhs_immediate,
        *operand_view,
        std::vector<const RegisterOperand*>{&*scalar.result_register},
        lines);
    if (!lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar unsigned reduction node has incomplete printable lhs facts"};
    }
    std::ostringstream out;
    if (alu.operation == ScalarAluOperationKind::LogicalShiftRight &&
        alu.source_binary_opcode == bir::BinaryOpcode::UDiv) {
      out << "lsr " << *result << ", " << lhs->name << ", #" << rhs_immediate->unsigned_value;
      lines.push_back(out.str());
      if (alu.post_zero_extend_result_bits.has_value()) {
        std::ostringstream extend;
        extend << "ubfx " << *result << ", " << *result << ", #0, #"
               << *alu.post_zero_extend_result_bits;
        lines.push_back(extend.str());
      }
      return append_scalar_alu_stack_publication(lines, alu, *result);
    }
    if (alu.operation == ScalarAluOperationKind::And &&
        alu.source_binary_opcode == bir::BinaryOpcode::URem) {
      out << "and " << *result << ", " << lhs->name << ", #" << rhs_immediate->unsigned_value;
      lines.push_back(out.str());
      if (alu.post_zero_extend_result_bits.has_value()) {
        std::ostringstream extend;
        extend << "ubfx " << *result << ", " << *result << ", #0, #"
               << *alu.post_zero_extend_result_bits;
        lines.push_back(extend.str());
      }
      return append_scalar_alu_stack_publication(lines, alu, *result);
    }
    return {.lines = std::nullopt,
            .diagnostic = "scalar unsigned reduction operation is not printable"};
  }
  if (scalar.scalar_alu.has_value() && scalar.scalar_alu->supported_integer_operation &&
      (scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::Shl ||
       scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::LShr ||
       scalar.scalar_alu->source_binary_opcode == bir::BinaryOpcode::AShr)) {
    const auto& alu = *scalar.scalar_alu;
    if (scalar.inputs.size() != 2) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar shift node requires two structured operands"};
    }
    const auto result_view = scalar_register_view(alu.result_type);
    const auto operand_view = scalar_register_view(alu.operand_type);
    const auto bit_width = integer_scalar_bit_width(alu.operand_type);
    if (!result_view.has_value() || !operand_view.has_value() ||
        result_view != operand_view || !bit_width.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar shift node requires matching integer widths"};
    }
    if (!scalar.result_register.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar shift node requires a structured result register"};
    }
    const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
    const auto* lhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[0].payload);
    const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
    const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
    const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                                 lhs_register != nullptr;
    const bool lhs_is_memory = scalar.inputs[0].kind == OperandKind::Memory &&
                               lhs_memory != nullptr;
    const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                  lhs_immediate != nullptr;
    if ((!lhs_is_register && !lhs_is_memory && !lhs_is_immediate) ||
        scalar.inputs[1].kind != OperandKind::Immediate || rhs_immediate == nullptr) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar shift node requires register/memory/immediate lhs and "
                  "immediate shift amount"};
    }
    if (rhs_immediate->unsigned_value >= *bit_width) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar shift amount exceeds operand width"};
    }
    const auto result =
        scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
    if (!result.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar shift node has incomplete printable result register facts"};
    }

    std::vector<std::string> lines;
    const auto lhs = materialize_scalar_register_source(
        scalar.inputs[0],
        lhs_register,
        lhs_memory,
        lhs_immediate,
        *operand_view,
        std::vector<const RegisterOperand*>{&*scalar.result_register},
        lines);
    if (!lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic = "scalar shift node has incomplete printable lhs facts"};
    }
    std::string_view mnemonic = "asr";
    if (alu.source_binary_opcode == bir::BinaryOpcode::Shl) {
      mnemonic = "lsl";
    } else if (alu.source_binary_opcode == bir::BinaryOpcode::LShr) {
      mnemonic = "lsr";
    }
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << lhs->name << ", #"
        << rhs_immediate->unsigned_value;
    lines.push_back(out.str());
    return append_scalar_alu_stack_publication(lines, alu, *result);
  }
  if (instruction.opcode != MachineOpcode::Add &&
      instruction.opcode != MachineOpcode::Sub &&
      instruction.opcode != MachineOpcode::And &&
      instruction.opcode != MachineOpcode::Or &&
      instruction.opcode != MachineOpcode::Xor) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar node opcode is outside the printable add/sub/bitwise subset"};
  }
  if (scalar.inputs.size() != 2) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise node requires exactly two register or immediate operands"};
  }

  const auto* lhs_register = std::get_if<RegisterOperand>(&scalar.inputs[0].payload);
  const auto* rhs_register = std::get_if<RegisterOperand>(&scalar.inputs[1].payload);
  const auto* lhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[0].payload);
  const auto* rhs_memory = std::get_if<MemoryOperand>(&scalar.inputs[1].payload);
  const auto* lhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[0].payload);
  const auto* rhs_immediate = std::get_if<ImmediateOperand>(&scalar.inputs[1].payload);
  const bool lhs_is_register = scalar.inputs[0].kind == OperandKind::Register &&
                               lhs_register != nullptr;
  const bool rhs_is_register = scalar.inputs[1].kind == OperandKind::Register &&
                               rhs_register != nullptr;
  const bool lhs_is_memory = scalar.inputs[0].kind == OperandKind::Memory &&
                             lhs_memory != nullptr;
  const bool rhs_is_memory = scalar.inputs[1].kind == OperandKind::Memory &&
                             rhs_memory != nullptr;
  const bool lhs_is_immediate = scalar.inputs[0].kind == OperandKind::Immediate &&
                                lhs_immediate != nullptr;
  const bool rhs_is_immediate = scalar.inputs[1].kind == OperandKind::Immediate &&
                                rhs_immediate != nullptr;
  if ((!lhs_is_register && !lhs_is_memory && !lhs_is_immediate) ||
      (!rhs_is_register && !rhs_is_memory && !rhs_is_immediate)) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise node requires register, memory, or immediate operands"};
  }
  if ((lhs_is_memory && lhs_memory->base_kind != MemoryBaseKind::FrameSlot) ||
      (rhs_is_memory && rhs_memory->base_kind != MemoryBaseKind::FrameSlot)) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise memory operands require prepared frame-slot sources"};
  }
  const auto& alu = *scalar.scalar_alu;
  std::string_view mnemonic = machine_instruction_primary_printer_mnemonic(instruction);
  if (instruction.opcode == MachineOpcode::And) {
    mnemonic = "and";
  } else if (instruction.opcode == MachineOpcode::Or) {
    mnemonic = "orr";
  } else if (instruction.opcode == MachineOpcode::Xor) {
    mnemonic = "eor";
  }
  if (mnemonic.empty()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub/bitwise mnemonic is not printable"};
  }
  if (alu.post_zero_extend_result_bits.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise node does not support post-zero-extension facts"};
  }
  if (alu.post_sign_extend_result_bits.has_value() &&
      (*alu.post_sign_extend_result_bits != 32U ||
       alu.operand_type != bir::TypeKind::I32 ||
       alu.result_type != bir::TypeKind::I64)) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise node has unsupported post-sign-extension width"};
  }

  std::vector<std::string> lines;
  const auto result_view =
      alu.post_sign_extend_result_bits.has_value() ? scalar_register_view(alu.operand_type)
                                                   : scalar_register_view(alu.result_type);
  if (!result_view.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic = "scalar add/sub/bitwise node has unsupported printable result width"};
  }
  const auto result = scalar_gp_register_name_with_view(*scalar.result_register, *result_view);
  if (!result.has_value()) {
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar add/sub/bitwise node has incomplete printable result register fact"};
  }

  struct MaterializedScalarSource {
    std::string name;
    std::optional<RegisterOperand> scratch;
  };

  const auto append_materialized_integer_immediate =
      [&](abi::RegisterReference destination, const ImmediateOperand& immediate) -> bool {
    const std::string destination_name = abi::register_name(destination);
    if (is_plain_mov_immediate(immediate)) {
      std::ostringstream move;
      move << "mov " << destination_name << ", " << scalar_immediate_name(immediate);
      lines.push_back(move.str());
      return true;
    }
    auto materialized = materialize_integer_constant_lines(
        destination,
        static_cast<std::uint64_t>(immediate.signed_value),
        *result_view == abi::RegisterView::X ? 64U : 32U);
    if (materialized.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialized.begin(), materialized.end());
    return true;
  };

  const auto materialize_source =
      [&](const OperandRecord& operand,
          const RegisterOperand* reg,
          const MemoryOperand* memory,
          const ImmediateOperand* immediate,
          bool allow_plain_immediate,
          std::vector<const RegisterOperand*> occupied)
          -> std::optional<MaterializedScalarSource> {
    if (operand.kind == OperandKind::Register && reg != nullptr) {
      const auto name = scalar_gp_register_name_with_view(*reg, *result_view);
      if (!name.has_value()) {
        return std::nullopt;
      }
      return MaterializedScalarSource{.name = *name};
    }
    if (operand.kind == OperandKind::Memory && memory != nullptr) {
      if (memory->support != MemoryOperandSupportKind::Prepared ||
          !memory->can_use_base_plus_offset ||
          !memory->byte_offset_is_prepared_snapshot) {
        return std::nullopt;
      }
      const auto scratch = scalar_gp_scratch_register(*result_view, occupied);
      if (!scratch.has_value()) {
        return std::nullopt;
      }
      const std::string scratch_name = abi::register_name(scratch->reg);
      if (memory->base_kind == MemoryBaseKind::FrameSlot) {
        if (!append_scalar_frame_slot_load(lines,
                                           *memory,
                                           scratch_name,
                                           abi::x_register(scratch->reg.index))) {
          return std::nullopt;
        }
      } else {
        const auto address = memory_address(*memory);
        if (address.empty()) {
          return std::nullopt;
        }
        std::ostringstream load;
        load << "ldr " << scratch_name << ", " << address;
        lines.push_back(load.str());
      }
      return MaterializedScalarSource{.name = scratch_name, .scratch = scratch};
    }
    if (operand.kind == OperandKind::Immediate && immediate != nullptr) {
      if (allow_plain_immediate &&
          scalar_alu_operation_accepts_immediate(alu.operation, *immediate)) {
        return MaterializedScalarSource{.name = scalar_immediate_name(*immediate)};
      }
      const auto scratch = scalar_gp_scratch_register(*result_view, occupied);
      if (!scratch.has_value()) {
        return std::nullopt;
      }
      const std::string scratch_name = abi::register_name(scratch->reg);
      if (!append_materialized_integer_immediate(scratch->reg, *immediate)) {
        return std::nullopt;
      }
      return MaterializedScalarSource{.name = scratch_name, .scratch = scratch};
    }
    return std::nullopt;
  };

  const auto materialize_lhs_immediate_into_result =
      [&](const ImmediateOperand& immediate) -> bool {
    const auto destination =
        abi::gp_register(scalar.result_register->reg.index, *result_view);
    if (!destination.has_value()) {
      return false;
    }
    return append_materialized_integer_immediate(*destination, immediate);
  };

  const auto materialize_rhs_immediate_for_existing_lhs =
      [&](const ImmediateOperand& immediate,
          std::string_view lhs_name,
          std::vector<const RegisterOperand*> occupied)
          -> std::optional<MaterializedScalarSource> {
    if (scalar_alu_operation_accepts_immediate(alu.operation, immediate)) {
      return MaterializedScalarSource{.name = scalar_immediate_name(immediate)};
    }
    if (scalar.result_register.has_value()) {
      occupied.push_back(&*scalar.result_register);
    }
    const auto scratch = scalar_gp_scratch_register(*result_view, occupied);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    const std::string scratch_name = abi::register_name(scratch->reg);
    if (scratch_name == lhs_name) {
      return std::nullopt;
    }
    if (!append_materialized_integer_immediate(scratch->reg, immediate)) {
      return std::nullopt;
    }
    return MaterializedScalarSource{.name = scratch_name, .scratch = scratch};
  };

  const auto append_binary_alu =
      [&](std::string_view lhs_name, std::string_view rhs_name) {
    std::ostringstream out;
    out << mnemonic << " " << *result << ", " << lhs_name << ", " << rhs_name;
    lines.push_back(out.str());
  };

  const bool lhs_requires_register_source = lhs_is_register || lhs_is_memory;
  const bool rhs_requires_register_source = rhs_is_register || rhs_is_memory;
  if (lhs_requires_register_source && rhs_requires_register_source) {
    std::vector<const RegisterOperand*> lhs_occupied;
    if (rhs_is_register) {
      lhs_occupied.push_back(rhs_register);
    }
    std::vector<const RegisterOperand*> rhs_occupied;
    if (scalar.result_register.has_value()) {
      rhs_occupied.push_back(&*scalar.result_register);
    }
    if (lhs_is_register) {
      rhs_occupied.push_back(lhs_register);
    }
    const auto lhs = materialize_source(
        scalar.inputs[0], lhs_register, lhs_memory, nullptr,
        false,
        std::move(lhs_occupied));
    if (lhs.has_value() && lhs->scratch.has_value()) {
      rhs_occupied.push_back(&*lhs->scratch);
    }
    const auto rhs = materialize_source(
        scalar.inputs[1],
        rhs_register,
        rhs_memory,
        nullptr,
        false,
        std::move(rhs_occupied));
    if (!lhs.has_value() || !rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub/bitwise node has incomplete printable register or memory operand facts"};
    }
    append_binary_alu(lhs->name, rhs->name);
  } else if ((lhs_is_register || lhs_is_memory) && rhs_is_immediate) {
    const auto lhs = materialize_source(
        scalar.inputs[0],
        lhs_register,
        lhs_memory,
        nullptr,
        false,
        {});
    if (!lhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                "scalar add/sub/bitwise node has incomplete printable lhs operand facts"};
    }
    std::vector<const RegisterOperand*> occupied;
    if (lhs_register != nullptr) {
      occupied.push_back(lhs_register);
    }
    if (lhs->scratch.has_value()) {
      occupied.push_back(&*lhs->scratch);
    }
    const auto rhs =
        materialize_rhs_immediate_for_existing_lhs(*rhs_immediate, lhs->name, std::move(occupied));
    if (!rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub/bitwise node has no scratch register for materialized rhs immediate"};
    }
    append_binary_alu(lhs->name, rhs->name);
  } else if (lhs_is_immediate && (rhs_is_register || rhs_is_memory)) {
    const auto rhs = materialize_source(
        scalar.inputs[1],
        rhs_register,
        rhs_memory,
        nullptr,
        false,
        scalar.result_register.has_value()
            ? std::vector<const RegisterOperand*>{&*scalar.result_register}
            : std::vector<const RegisterOperand*>{});
    if (!rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                "scalar add/sub/bitwise node has incomplete printable rhs operand facts"};
    }
    if (instruction.opcode == MachineOpcode::Add &&
        scalar_alu_operation_accepts_immediate(alu.operation, *lhs_immediate)) {
      append_binary_alu(rhs->name, scalar_immediate_name(*lhs_immediate));
    } else {
      std::vector<const RegisterOperand*> occupied;
      if (scalar.result_register.has_value()) {
        occupied.push_back(&*scalar.result_register);
      }
      if (rhs_register != nullptr) {
        occupied.push_back(rhs_register);
      }
      if (rhs->scratch.has_value()) {
        occupied.push_back(&*rhs->scratch);
      }
      const auto lhs = materialize_source(
          scalar.inputs[0],
          nullptr,
          nullptr,
          lhs_immediate,
          false,
          std::move(occupied));
      if (!lhs.has_value()) {
        return {.lines = std::nullopt,
                .diagnostic =
                    "scalar add/sub/bitwise node has no scratch register for materialized lhs immediate"};
      }
      append_binary_alu(lhs->name, rhs->name);
    }
  } else if (lhs_is_immediate && rhs_is_immediate) {
    materialize_lhs_immediate_into_result(*lhs_immediate);
    const auto result_register = scalar.result_register.has_value()
                                     ? &*scalar.result_register
                                     : nullptr;
    const auto rhs = materialize_rhs_immediate_for_existing_lhs(
        *rhs_immediate,
        *result,
        result_register != nullptr ? std::vector<const RegisterOperand*>{result_register}
                                   : std::vector<const RegisterOperand*>{});
    if (!rhs.has_value()) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar add/sub/bitwise node has no scratch register for materialized immediate pair"};
    }
    append_binary_alu(*result, rhs->name);
  } else {
    if (lhs_is_immediate && (rhs_is_register || rhs_is_memory) &&
        instruction.opcode == MachineOpcode::Sub) {
      return {.lines = std::nullopt,
              .diagnostic =
                  "scalar sub with an immediate lhs and register rhs is not printable"};
    }
    return {.lines = std::nullopt,
            .diagnostic =
                "scalar sub/bitwise with an immediate lhs and register rhs is not printable"};
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
  if (alu.result_stack_offset_bytes.has_value()) {
    return append_scalar_alu_stack_publication(lines, alu, *result);
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
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      return true;
    case bir::BinaryOpcode::Mul:
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
    case bir::BinaryOpcode::Shl:
      return ScalarAluOperationKind::LogicalShiftRight;
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      return ScalarAluOperationKind::LogicalShiftRight;
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

std::optional<abi::RegisterView> scalar_storage_register_view(bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
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
  for (auto it = state.emitted_registers.begin();
       it != state.emitted_registers.end();) {
    if (it->first != value_name && scalar_registers_alias(it->second, reg)) {
      it = state.emitted_registers.erase(it);
    } else {
      ++it;
    }
  }
  state.emitted_registers[value_name] = std::move(reg);
}

void clear_emitted_scalar_register(BlockScalarLoweringState& state,
                                   c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return;
  }
  state.emitted_registers.erase(value_name);
}

void clear_call_clobbered_emitted_scalar_registers(BlockScalarLoweringState& state) {
  for (auto it = state.emitted_registers.begin();
       it != state.emitted_registers.end();) {
    if (abi::is_caller_saved(it->second.reg)) {
      it = state.emitted_registers.erase(it);
    } else {
      ++it;
    }
  }
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

  if (home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      storage->slot_id.has_value() && storage->stack_offset_bytes.has_value()) {
    const auto size_bytes = scalar_type_size_bytes(value.type);
    if (!size_bytes.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_memory_operand(MemoryOperand{
        .surface = RecordSurfaceKind::RecordOnly,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = value_locations.function_name,
        .result_value_id = home->value_id,
        .result_value_name = home->value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = storage->slot_id,
        .byte_offset = static_cast<std::int64_t>(*storage->stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *size_bytes,
        .align_bytes = home->align_bytes.value_or(*size_bytes),
        .can_use_base_plus_offset = true,
    });
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

PreparedScalarAluRecordError make_prepared_scalar_result_operand(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& result,
    RegisterOperand& out,
    std::optional<std::int64_t>& result_stack_offset_bytes) {
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
  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::Register) {
    return make_prepared_scalar_result_register_operand(
        names, value_locations, storage_plan, result, out);
  }
  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      result_storage->encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      result_storage->stack_offset_bytes.has_value()) {
    const auto scratch =
        make_scalar_spill_scratch_operand(*result_home, *result_storage, result.type);
    if (!scratch.has_value()) {
      return PreparedScalarAluRecordError::RegisterConversionFailed;
    }
    out = *scratch;
    result_stack_offset_bytes =
        static_cast<std::int64_t>(*result_storage->stack_offset_bytes);
    return PreparedScalarAluRecordError::None;
  }
  return PreparedScalarAluRecordError::UnsupportedResultStorage;
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
  const auto semantic = scalar_alu_semantic_facts(binary);
  if (!semantic.integer_operation && is_prepared_scalar_float_alu_operation(binary)) {
    return make_prepared_scalar_float_alu_record(
        names, value_locations, storage_plan, binary);
  }
  if (!semantic.integer_operation && !semantic.unsigned_reduction_candidate &&
      !semantic.unsigned_div_rem_publication) {
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
  std::optional<std::int64_t> result_stack_offset_bytes;
  if (const auto error = make_prepared_scalar_result_operand(
          names,
          value_locations,
          storage_plan,
          binary.result,
          result_register,
          result_stack_offset_bytes);
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
  if (binary.opcode == bir::BinaryOpcode::Shl ||
      binary.opcode == bir::BinaryOpcode::LShr ||
      binary.opcode == bir::BinaryOpcode::AShr) {
    const auto* shift_amount = std::get_if<ImmediateOperand>(&rhs.payload);
    const auto bit_width = integer_scalar_bit_width(binary.operand_type);
    if (rhs.kind != OperandKind::Immediate || shift_amount == nullptr ||
        !bit_width.has_value() || shift_amount->unsigned_value >= *bit_width) {
      return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
    }
  }
  if (binary.opcode == bir::BinaryOpcode::UDiv ||
      binary.opcode == bir::BinaryOpcode::URem) {
    const auto* divisor = std::get_if<ImmediateOperand>(&rhs.payload);
    const auto bit_width = integer_scalar_bit_width(binary.operand_type);
    if (rhs.kind == OperandKind::Immediate && divisor != nullptr &&
        bit_width.has_value() && *bit_width < 64U &&
        divisor->unsigned_value >= (std::uint64_t{1} << *bit_width)) {
      return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
    }
  }

  ScalarAluOperationKind operation = semantic.opcode.publication_operation;
  bool supported_integer_operation =
      semantic.integer_operation || semantic.unsigned_div_rem_publication;
  std::optional<unsigned> post_zero_extend_result_bits;
  std::optional<unsigned> post_sign_extend_result_bits =
      semantic.post_sign_extend_result_bits;
  if (semantic.unsigned_reduction_candidate) {
    const auto reduction_operation =
        unsigned_reduction_operation(binary.opcode, binary.operand_type, rhs);
    if (!supported_integer_operation && !reduction_operation.has_value()) {
      return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
    }
    if (reduction_operation.has_value()) {
      const auto* divisor = std::get_if<ImmediateOperand>(&rhs.payload);
      const auto replacement =
          unsigned_reduction_replacement_immediate(binary.opcode, binary.operand_type, *divisor);
      rhs = make_immediate_operand(*replacement);
      operation = *reduction_operation;
      supported_integer_operation = true;
      post_zero_extend_result_bits =
          unsigned_reduction_post_zero_extend_bits(binary.result.type);
    }
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
              .result_stack_offset_bytes = result_stack_offset_bytes,
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

std::optional<module::MachineInstruction> lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->opcode != bir::BinaryOpcode::Mul ||
      binary->result.kind != bir::Value::Kind::Named ||
      binary->result.name.empty()) {
    return std::nullopt;
  }
  if (binary->lhs.kind != bir::Value::Kind::Immediate &&
      binary->rhs.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto result_view = scalar_view_for_type(binary->result.type);
  const auto operand_view = scalar_view_for_type(binary->operand_type);
  if (!result_view.has_value() || !operand_view.has_value() ||
      result_view != operand_view) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, binary->result);
  std::optional<std::size_t> result_stack_offset_bytes;
  if (!result_register.has_value()) {
    const auto value_name = prepared_named_value_id(context, binary->result);
    const auto* home =
        value_name.has_value() && context.function.value_locations != nullptr
            ? prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                        context.function.regalloc,
                                                        context.function.value_locations,
                                                        *value_name)
            : nullptr;
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, *result_view);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = RegisterOperand{
        .reg = *scratch,
        .role = RegisterOperandRole::SpillAuthority,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = result_view,
    };
    result_stack_offset_bytes = *home->offset_bytes;
  }
  if (!abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }
  const auto result_name =
      gp_register_name(result_register->reg.index, *result_view);
  if (!result_name.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> rhs_scratch_index;
  std::optional<std::uint8_t> nested_scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index) {
      rhs_scratch_index = scratch.index;
      break;
    }
  }
  if (!rhs_scratch_index.has_value()) {
    return std::nullopt;
  }
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index &&
        scratch.index != *rhs_scratch_index) {
      nested_scratch_index = scratch.index;
      break;
    }
  }
  if (!nested_scratch_index.has_value()) {
    nested_scratch_index = result_register->reg.index;
  }
  const auto rhs_name = gp_register_name(*rhs_scratch_index, *operand_view);
  if (!rhs_name.has_value()) {
    return std::nullopt;
  }

  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  std::vector<std::string> lines;
  const auto emit_shifted_power_of_two_operand =
      [&](const bir::Value& value, std::uint64_t scale) -> bool {
    const auto shift = power_of_two_shift(scale);
    if (!shift.has_value() ||
        !emit_value_publication_to_register(context,
                                            value,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return false;
    }
    if (*shift != 0U) {
      lines.push_back("lsl " + *result_name + ", " + *result_name +
                      ", #" + std::to_string(*shift));
    }
    return true;
  };
  bool emitted_power_of_two_scale = false;
  if (rhs.kind == bir::Value::Kind::Immediate && rhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(lhs,
                                          static_cast<std::uint64_t>(rhs.immediate));
  } else if (lhs.kind == bir::Value::Kind::Immediate && lhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(rhs,
                                          static_cast<std::uint64_t>(lhs.immediate));
  }
  if (emitted_power_of_two_scale) {
    if (result_stack_offset_bytes.has_value()) {
      lines.push_back("str " + *result_name + ", " +
                      frame_slot_address(context.function, *result_stack_offset_bytes));
    }
    std::string asm_text;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (index != 0) {
        asm_text += '\n';
      }
      asm_text += lines[index];
    }
    InstructionRecord target{
        .family = InstructionFamily::Assembler,
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .opcode = MachineOpcode::Unspecified,
        .selection =
            MachineNodeStatusRecord{
                .status = MachineNodeSelectionStatus::Selected,
            },
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .block_index = context.block_index,
        .instruction_index = instruction_index,
        .payload =
            AssemblerInstructionRecord{
                .has_inline_asm_payload = true,
                .side_effects = false,
                .inline_asm_template = std::move(asm_text),
            },
    };
    record_emitted_scalar_register(scalar_state,
                                   result_register->value_name,
                                   *result_register);
    return module::MachineInstruction{
        .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
        .operands = {},
        .target = std::move(target),
        .origin =
            c4c::backend::mir::MachineOrigin{
                .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
                .function_name = context.function.control_flow != nullptr
                                     ? context.function.control_flow->function_name
                                     : c4c::kInvalidFunctionName,
                .block_label = context.control_flow_block != nullptr
                                   ? context.control_flow_block->block_label
                                   : c4c::kInvalidBlockLabel,
                .instruction_index = instruction_index,
            },
    };
  }
  const bool rhs_reads_result = value_publication_may_read_register_index(
      context, rhs, instruction_index, result_register->reg.index);
  const bool lhs_reads_rhs_scratch = value_publication_may_read_register_index(
      context, lhs, instruction_index, *rhs_scratch_index);

  if (rhs_reads_result && !lhs_reads_rhs_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  }
  lines.push_back("mul " + *result_name + ", " + *result_name + ", " + *rhs_name);
  if (result_stack_offset_bytes.has_value()) {
    lines.push_back("str " + *result_name + ", " +
                    frame_slot_address(context.function, *result_stack_offset_bytes));
  }

  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = false,
              .inline_asm_template = std::move(asm_text),
          },
  };
  record_emitted_scalar_register(scalar_state,
                                 result_register->value_name,
                                 *result_register);
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
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
    const auto prepared_record = make_prepared_scalar_alu_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *binary);
    if (prepared_record.record.has_value()) {
      scalar_record = make_scalar_alu_instruction_record(*prepared_record.record);
    }
    if (scalar_record.has_value()) {
      if (scalar_record->scalar_alu.has_value() && scalar_record->inputs.size() == 2) {
        scalar_record->scalar_alu->result_uses_frame_pointer_base =
            context.function.frame_plan != nullptr &&
            context.function.frame_plan->uses_frame_pointer_for_fixed_slots;
        if (const auto* lhs_home = find_named_value_home(binary->lhs, context.function);
            lhs_home != nullptr &&
            lhs_home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate) {
          if (const auto emitted_lhs =
                  find_emitted_scalar_register(scalar_state, lhs_home->value_name);
              emitted_lhs.has_value()) {
            scalar_record->scalar_alu->lhs = make_register_operand(*emitted_lhs);
            scalar_record->inputs[0] = scalar_record->scalar_alu->lhs;
          } else if (auto load_source = make_unpublished_load_local_source_operand(
                         context, binary->lhs, instruction_index);
                     load_source.has_value()) {
            scalar_record->scalar_alu->lhs = make_memory_operand(*load_source);
            scalar_record->inputs[0] = scalar_record->scalar_alu->lhs;
          }
        }
        if (const auto* rhs_home = find_named_value_home(binary->rhs, context.function);
            rhs_home != nullptr &&
            rhs_home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate) {
          if (const auto emitted_rhs =
                  find_emitted_scalar_register(scalar_state, rhs_home->value_name);
              emitted_rhs.has_value()) {
            scalar_record->scalar_alu->rhs = make_register_operand(*emitted_rhs);
            scalar_record->inputs[1] = scalar_record->scalar_alu->rhs;
          } else if (auto load_source = make_unpublished_load_local_source_operand(
                         context, binary->rhs, instruction_index);
                     load_source.has_value()) {
            scalar_record->scalar_alu->rhs = make_memory_operand(*load_source);
            scalar_record->inputs[1] = scalar_record->scalar_alu->rhs;
          }
        }
      }
    }
    if (!scalar_record.has_value()) {
      const auto* result_home = find_named_value_home(binary->result, context.function);
      auto result_register =
          result_home == nullptr
              ? std::optional<RegisterOperand>{}
              : find_return_abi_register(context,
                                         result_home->value_id,
                                         result_home->value_name,
                                         binary->result.type);
      const auto authoritative_immediate =
          authoritative_immediate_storage(binary->result, context.function);
      if (result_home != nullptr && authoritative_immediate.has_value() &&
          result_register.has_value() &&
          (uses_unemitted_authoritative_immediate(binary->lhs, context, scalar_state) ||
           uses_unemitted_authoritative_immediate(binary->rhs, context, scalar_state))) {
        const auto zero = make_scalar_immediate_operand(
            bir::Value::immediate_i32(0),
            result_home->value_id,
            result_home->value_name);
        if (!zero.has_value()) {
          return std::nullopt;
        }
        scalar_record = make_scalar_alu_instruction_record(ScalarAluRecord{
            .surface = RecordSurfaceKind::MachineInstructionNode,
            .operation = ScalarAluOperationKind::Add,
            .source_binary_opcode = bir::BinaryOpcode::Add,
            .operand_type = binary->result.type,
            .result_value_id = result_home->value_id,
            .result_value_name = result_home->value_name,
            .result_type = binary->result.type,
            .result_register = *result_register,
            .lhs = make_immediate_operand(*authoritative_immediate),
            .rhs = make_immediate_operand(*zero),
            .supported_integer_operation = true,
        });
      }
      if (!scalar_record.has_value()) {
        if (!result_register.has_value() && result_home != nullptr) {
          result_register = find_return_chain_register(
              context, instruction_index, *result_home, binary->result.type);
        }
        if (result_register.has_value() && result_home != nullptr &&
            result_home->kind == prepare::PreparedValueHomeKind::RematerializableImmediate &&
            authoritative_immediate.has_value() &&
            context.function.prepared_lookups != nullptr &&
            context.function.value_locations != nullptr) {
          const auto next_operand_value_name =
              prepare::find_prepared_return_chain_next_operand_value(
                  &context.function.prepared_lookups->return_chains,
                  context.block_index,
                  instruction_index,
                  result_home->value_name);
          const auto* other_home =
              prepare::find_indexed_prepared_value_home(
                  context.function.value_home_lookups,
                  nullptr,
                  context.function.value_locations,
                  next_operand_value_name);
          const auto other_reg =
              other_home != nullptr && other_home->register_name.has_value()
                  ? abi::parse_aarch64_register_name(*other_home->register_name)
                  : std::nullopt;
          if (other_reg.has_value() &&
              other_reg->bank == result_register->reg.bank &&
              other_reg->index == result_register->reg.index) {
            const auto result_view = scalar_register_view(binary->result.type);
            if (!result_view.has_value()) {
              return std::nullopt;
            }
            auto scratch = scalar_gp_scratch_register(*result_view, {&*result_register});
            if (!scratch.has_value()) {
              return std::nullopt;
            }
            scratch->value_id = result_home->value_id;
            scratch->value_name = result_home->value_name;
            scratch->prepared_bank = prepare::PreparedRegisterBank::Gpr;
            result_register = *scratch;
          }
        }
        std::optional<std::int64_t> result_stack_offset_bytes;
        if (!result_register.has_value() && result_home != nullptr &&
            is_scalar_alu_publication_opcode(binary->opcode) &&
            context.function.prepared != nullptr &&
            context.function.value_locations != nullptr &&
            context.function.storage_plan != nullptr) {
          RegisterOperand storage_register;
          if (make_prepared_scalar_result_operand(
                  context.function.prepared->names,
                  *context.function.value_locations,
                  *context.function.storage_plan,
                  binary->result,
                  storage_register,
                  result_stack_offset_bytes) == PreparedScalarAluRecordError::None) {
            result_register = storage_register;
          }
        }
        const auto lhs =
            make_scalar_fallback_operand(
                binary->lhs, context, instruction_index, scalar_state, diagnostics);
        const auto rhs =
            make_scalar_fallback_operand(
                binary->rhs, context, instruction_index, scalar_state, diagnostics);
        const auto operand_is_memory = [](const std::optional<OperandRecord>& operand) {
          if (!operand.has_value()) {
            return false;
          }
          return operand->kind == OperandKind::Memory;
        };
        if ((operand_is_memory(lhs) || operand_is_memory(rhs)) &&
            binary->opcode != bir::BinaryOpcode::Add &&
            binary->opcode != bir::BinaryOpcode::Sub &&
            binary->opcode != bir::BinaryOpcode::And &&
            binary->opcode != bir::BinaryOpcode::Or &&
            binary->opcode != bir::BinaryOpcode::Xor &&
            binary->opcode != bir::BinaryOpcode::Mul &&
            binary->opcode != bir::BinaryOpcode::SDiv &&
            binary->opcode != bir::BinaryOpcode::SRem &&
            binary->opcode != bir::BinaryOpcode::UDiv &&
            binary->opcode != bir::BinaryOpcode::URem) {
          return std::nullopt;
        }
        if (result_home == nullptr || !result_register.has_value() || !lhs.has_value() ||
            !rhs.has_value() || !is_scalar_alu_publication_opcode(binary->opcode)) {
          return std::nullopt;
        }
        scalar_record = make_scalar_alu_instruction_record(ScalarAluRecord{
            .surface = RecordSurfaceKind::MachineInstructionNode,
            .operation = scalar_alu_publication_operation(binary->opcode),
            .source_binary_opcode = binary->opcode,
            .operand_type = binary->operand_type,
            .result_value_id = result_home->value_id,
            .result_value_name = result_home->value_name,
            .result_type = binary->result.type,
            .result_register = *result_register,
            .result_stack_offset_bytes = result_stack_offset_bytes,
            .result_uses_frame_pointer_base =
                context.function.frame_plan != nullptr &&
                context.function.frame_plan->uses_frame_pointer_for_fixed_slots,
            .lhs = *lhs,
            .rhs = *rhs,
            .supported_integer_operation = true,
        });
      }
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

std::optional<module::MachineInstruction> lower_scalar_control_value_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    bool allow_prepared_load_source) {
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return lower_scalar_compare_publication(
        context,
        *binary,
        instruction_index,
        scalar_state,
        diagnostics,
        allow_prepared_load_source);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return lower_scalar_select_publication(
        context, *select, instruction_index, scalar_state, diagnostics);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr && is_scalar_alu_publication_opcode(binary->opcode)) {
    return lower_scalar_instruction(
        context, inst, instruction_index, scalar_state, diagnostics);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
