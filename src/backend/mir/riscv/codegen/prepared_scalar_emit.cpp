#include "prepared_scalar_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"

#include "../../../prealloc/addressing.hpp"

#include <algorithm>

namespace c4c::backend::riscv::codegen {

std::optional<std::int64_t> simple_integer_immediate(
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
      return value.immediate;
    default:
      return std::nullopt;
  }
}

namespace {

std::optional<std::string> riscv_branch_mnemonic(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq: return std::string{"beq"};
    case c4c::backend::bir::BinaryOpcode::Ne: return std::string{"bne"};
    case c4c::backend::bir::BinaryOpcode::Slt: return std::string{"blt"};
    case c4c::backend::bir::BinaryOpcode::Sge: return std::string{"bge"};
    case c4c::backend::bir::BinaryOpcode::Ult: return std::string{"bltu"};
    case c4c::backend::bir::BinaryOpcode::Uge: return std::string{"bgeu"};
    default:
      return std::nullopt;
  }
}

void normalize_riscv_compare_branch(c4c::backend::bir::BinaryOpcode& opcode,
                                    const c4c::backend::bir::Value*& lhs,
                                    const c4c::backend::bir::Value*& rhs) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Sle:
      opcode = c4c::backend::bir::BinaryOpcode::Sge;
      std::swap(lhs, rhs);
      break;
    case c4c::backend::bir::BinaryOpcode::Ule:
      opcode = c4c::backend::bir::BinaryOpcode::Uge;
      std::swap(lhs, rhs);
      break;
    case c4c::backend::bir::BinaryOpcode::Sgt:
      opcode = c4c::backend::bir::BinaryOpcode::Slt;
      std::swap(lhs, rhs);
      break;
    case c4c::backend::bir::BinaryOpcode::Ugt:
      opcode = c4c::backend::bir::BinaryOpcode::Ult;
      std::swap(lhs, rhs);
      break;
    default:
      break;
  }
}

std::optional<std::int64_t> prepared_immediate_i32_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind != c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate ||
      !home->immediate_i32.has_value()) {
    return std::nullopt;
  }
  return home->immediate_i32;
}

std::optional<std::int64_t> simple_or_prepared_integer_immediate(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto immediate = simple_integer_immediate(value);
  if (immediate.has_value()) {
    return immediate;
  }
  return prepared_immediate_i32_for_value(names, lookups, value);
}

bool emit_move_to_i32_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value) {
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      destination_home.register_name.has_value()) {
    return emit_move_to_register(
        out,
        *destination_home.register_name,
        context.names,
        context.lookups,
        value);
  }
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      destination_home.size_bytes == std::optional<std::size_t>{4} &&
      fits_signed_12_bit_load_offset(*destination_home.offset_bytes)) {
    if (!emit_move_to_register(out, "t3", context.names, context.lookups, value)) {
      return false;
    }
    out += "    sw t3, ";
    out += std::to_string(*destination_home.offset_bytes);
    out += "(sp)\n";
    return true;
  }
  return false;
}

}  // namespace

std::optional<std::string> emit_riscv_simple_compare_branch(
    const SimpleCompare& compare,
    std::string_view true_label,
    std::string_view false_label) {
  const auto* lhs_value = &compare.lhs;
  const auto* rhs_value = &compare.rhs;
  c4c::backend::bir::BinaryOpcode opcode = compare.opcode;
  normalize_riscv_compare_branch(opcode, lhs_value, rhs_value);

  const auto lhs_imm = simple_integer_immediate(*lhs_value);
  const auto rhs_imm = simple_integer_immediate(*rhs_value);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  std::int64_t lhs = *lhs_imm;
  std::int64_t rhs = *rhs_imm;

  const auto mnemonic = riscv_branch_mnemonic(opcode);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  out += "    li t0, " + std::to_string(lhs) + "\n";
  out += "    li t1, " + std::to_string(rhs) + "\n";
  out += "    " + *mnemonic + " t0, t1, " + std::string(true_label) + "\n";
  out += "    j " + std::string(false_label) + "\n";
  return out;
}

std::optional<std::string> emit_riscv_prepared_fused_compare_branch(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view true_label,
    std::string_view false_label) {
  if (branch_condition.kind !=
          c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  const c4c::backend::bir::Value* lhs_value = &*branch_condition.lhs;
  const c4c::backend::bir::Value* rhs_value = &*branch_condition.rhs;
  c4c::backend::bir::BinaryOpcode opcode = *branch_condition.predicate;
  normalize_riscv_compare_branch(opcode, lhs_value, rhs_value);

  const auto mnemonic = riscv_branch_mnemonic(opcode);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", names, lookups, *lhs_value) ||
      !emit_move_to_register(out, "t4", names, lookups, *rhs_value)) {
    return std::nullopt;
  }
  out += "    " + *mnemonic + " t3, t4, " + std::string(true_label) + "\n";
  out += "    j " + std::string(false_label) + "\n";
  return out;
}

bool emit_move_to_register(std::string& out,
                           std::string_view destination_register,
                           const c4c::backend::prepare::PreparedNameTables& names,
                           const c4c::backend::prepare::PreparedFunctionLookups* lookups,
                           const c4c::backend::bir::Value& value) {
  const auto immediate = simple_or_prepared_integer_immediate(names, lookups, value);
  if (immediate.has_value()) {
    out += "    li ";
    out += destination_register;
    out += ", ";
    out += std::to_string(*immediate);
    out += "\n";
    return true;
  }

  const auto source_register = prepared_register_for_value(names, lookups, value);
  if (source_register.has_value()) {
    if (*source_register != destination_register) {
      out += "    mv ";
      out += destination_register;
      out += ", ";
      out += *source_register;
      out += "\n";
    }
    return true;
  }

  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home != nullptr &&
      value.type == c4c::backend::bir::TypeKind::I32 &&
      home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value() &&
      home->size_bytes == std::optional<std::size_t>{4} &&
      fits_signed_12_bit_load_offset(*home->offset_bytes)) {
    const auto loaded =
        emit_i32_load_from_stack_offset(destination_register,
                                        static_cast<std::int64_t>(*home->offset_bytes));
    if (!loaded.has_value()) {
      return false;
    }
    out += *loaded;
    return true;
  }

  return false;
}

std::optional<std::string> emit_riscv_simple_cast(
    const c4c::backend::bir::CastInst& cast,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if ((cast.opcode != c4c::backend::bir::CastOpcode::SExt &&
       cast.opcode != c4c::backend::bir::CastOpcode::ZExt) ||
      cast.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(names, lookups, cast.result);
  if (!destination_register.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, *destination_register, names, lookups, cast.operand)) {
    return std::nullopt;
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I32 &&
      cast.result.type == c4c::backend::bir::TypeKind::I64) {
    out += "    slli " + *destination_register + ", " + *destination_register + ", 32\n";
    out += "    srli " + *destination_register + ", " + *destination_register + ", 32\n";
  }
  return out;
}

std::optional<std::string> emit_riscv_simple_select(
    const c4c::backend::bir::SelectInst& select,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context) {
  if (select.result.kind != c4c::backend::bir::Value::Kind::Named ||
      select.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(context, select.result);
  if (destination_home == nullptr) {
    return std::nullopt;
  }
  const auto mnemonic = riscv_branch_mnemonic(select.predicate);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", context.names, context.lookups, select.lhs) ||
      !emit_move_to_register(out, "t4", context.names, context.lookups, select.rhs)) {
    return std::nullopt;
  }

  const std::string label_base = ".L" + std::string(function_name) + "_select_" +
                                 std::to_string(context.instruction_index);
  const std::string true_label = label_base + "_true";
  const std::string done_label = label_base + "_done";
  out += "    " + *mnemonic + " t3, t4, " + true_label + "\n";
  if (!emit_move_to_i32_location(out, *destination_home, context, select.false_value)) {
    return std::nullopt;
  }
  out += "    j " + done_label + "\n";
  out += true_label + ":\n";
  if (!emit_move_to_i32_location(out, *destination_home, context, select.true_value)) {
    return std::nullopt;
  }
  out += done_label + ":\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_prepared_pointer_add(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedCurrentInstructionContext& context) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (binary.opcode != c4c::backend::bir::BinaryOpcode::Add ||
      binary.result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary.result.type != c4c::backend::bir::TypeKind::Ptr ||
      prepared_pointer_register_for_value(context, binary.result).has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for(context, binary.result);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  if (has_frame_slot_address_materialization_at(context)) {
    return std::string{};
  }
  if (!home->offset_bytes.has_value() ||
      home->size_bytes != std::optional<std::size_t>{8} ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(*home->offset_bytes))) {
    return std::nullopt;
  }

  const bir::Value* base = nullptr;
  const bir::Value* offset = nullptr;
  if (binary.lhs.type == bir::TypeKind::Ptr &&
      binary.rhs.type != bir::TypeKind::Ptr) {
    base = &binary.lhs;
    offset = &binary.rhs;
  } else if (binary.rhs.type == bir::TypeKind::Ptr &&
             binary.lhs.type != bir::TypeKind::Ptr) {
    base = &binary.rhs;
    offset = &binary.lhs;
  } else {
    return std::nullopt;
  }

  const auto base_register = prepared_pointer_register_for_value(context, *base);
  if (!base_register.has_value()) {
    return std::nullopt;
  }

  std::string out;
  const auto offset_immediate =
      simple_or_prepared_integer_immediate(context.names, context.lookups, *offset);
  if (offset_immediate.has_value()) {
    if (!fits_signed_12_bit_immediate(*offset_immediate)) {
      return std::nullopt;
    }
    out += "    addi t3, " + *base_register + ", " +
           std::to_string(*offset_immediate) + "\n";
  } else {
    const auto offset_register = prepared_register_for_value(context, *offset);
    if (!offset_register.has_value()) {
      return std::nullopt;
    }
    out += "    add t3, " + *base_register + ", " + *offset_register + "\n";
  }
  out += "    sd t3, " + std::to_string(*home->offset_bytes) + "(sp)\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_binary(
    const c4c::backend::bir::BinaryInst& binary,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if ((binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Mul) ||
      binary.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(names, lookups, binary.result);
  if (!destination_register.has_value()) {
    const auto result_immediate = prepared_immediate_i32_for_value(names, lookups, binary.result);
    return result_immediate.has_value() ? std::optional<std::string>{std::string{}} : std::nullopt;
  }

  const auto lhs_imm = simple_or_prepared_integer_immediate(names, lookups, binary.lhs);
  const auto rhs_imm = simple_or_prepared_integer_immediate(names, lookups, binary.rhs);
  const auto lhs_register = prepared_register_for_value(names, lookups, binary.lhs);
  const auto rhs_register = prepared_register_for_value(names, lookups, binary.rhs);

  std::string out;
  if (lhs_imm.has_value() && rhs_imm.has_value()) {
    std::int64_t result = 0;
    switch (binary.opcode) {
      case c4c::backend::bir::BinaryOpcode::Add:
        result = *lhs_imm + *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::Sub:
        result = *lhs_imm - *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::Mul:
        result = *lhs_imm * *rhs_imm;
        break;
      default:
        return std::nullopt;
    }
    out += "    li " + *destination_register + ", " +
           std::to_string(result) + "\n";
    return out;
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    std::string lhs_register_name;
    if (lhs_register.has_value()) {
      lhs_register_name = *lhs_register;
    } else if (lhs_imm.has_value()) {
      lhs_register_name = "t3";
      out += "    li " + lhs_register_name + ", " + std::to_string(*lhs_imm) + "\n";
    } else {
      return std::nullopt;
    }

    std::string rhs_register_name;
    if (rhs_register.has_value()) {
      rhs_register_name = *rhs_register;
    } else if (rhs_imm.has_value()) {
      rhs_register_name = "t4";
      out += "    li " + rhs_register_name + ", " + std::to_string(*rhs_imm) + "\n";
    } else {
      return std::nullopt;
    }

    out += "    mul " + *destination_register + ", " + lhs_register_name + ", " +
           rhs_register_name + "\n";
    return out;
  }
  if (lhs_register.has_value() && rhs_imm.has_value()) {
    const auto immediate = binary.opcode == c4c::backend::bir::BinaryOpcode::Add
                               ? *rhs_imm
                               : -*rhs_imm;
    if (fits_signed_12_bit_immediate(immediate)) {
      out += "    addi " + *destination_register + ", " + *lhs_register + ", " +
             std::to_string(immediate) + "\n";
      return out;
    }
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add &&
      rhs_register.has_value() && lhs_imm.has_value() &&
      fits_signed_12_bit_immediate(*lhs_imm)) {
    out += "    addi " + *destination_register + ", " + *rhs_register + ", " +
           std::to_string(*lhs_imm) + "\n";
    return out;
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub &&
      rhs_register.has_value() && lhs_imm.has_value()) {
    const std::string lhs_register_name =
        *destination_register == *rhs_register ? "t0" : *destination_register;
    out += "    li " + lhs_register_name + ", " + std::to_string(*lhs_imm) + "\n";
    out += "    sub " + *destination_register + ", " + lhs_register_name + ", " +
           *rhs_register + "\n";
    return out;
  }
  if (lhs_register.has_value() && rhs_register.has_value()) {
    out += std::string{"    "} +
           (binary.opcode == c4c::backend::bir::BinaryOpcode::Add ? "add " : "sub ") +
           *destination_register + ", " + *lhs_register + ", " +
           *rhs_register + "\n";
    return out;
  }
  return std::nullopt;
}

std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::optional<std::size_t> return_address_stack_offset,
    std::size_t stack_frame_bytes) {
  auto append_epilogue = [return_address_stack_offset, stack_frame_bytes](std::string& out) {
    if (return_address_stack_offset.has_value()) {
      out += "    ld ra, " + std::to_string(*return_address_stack_offset) + "(sp)\n";
    }
    if (stack_frame_bytes > 0) {
      out += "    addi sp, sp, " + std::to_string(stack_frame_bytes) + "\n";
    }
    out += "    ret\n";
  };

  if (!terminator.value.has_value()) {
    std::string out;
    append_epilogue(out);
    return out;
  }
  const auto immediate = simple_integer_immediate(*terminator.value);
  if (!immediate.has_value()) {
    std::string out;
    if (!emit_move_to_register(out, "a0", names, lookups, *terminator.value)) {
      return std::nullopt;
    }
    append_epilogue(out);
    return out;
  }
  std::string out = std::string{"    li a0, "} + std::to_string(*immediate) + "\n";
  append_epilogue(out);
  return out;
}

}  // namespace c4c::backend::riscv::codegen
