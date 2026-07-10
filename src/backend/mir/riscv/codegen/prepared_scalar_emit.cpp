#include "prepared_scalar_emit.hpp"

#include "object_emission.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_global_memory_emit.hpp"
#include "rv64_line_assembler.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <limits>

namespace c4c::backend::riscv::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;

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
    case c4c::backend::bir::TypeKind::Ptr:
      if (value.immediate == 0) {
        return 0;
      }
      return std::nullopt;
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
  const auto report =
      c4c::backend::prepare::
          verify_prepared_rematerializable_integer_immediate_contract(home);
  if (report.owner_class !=
      c4c::backend::prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  const auto fact =
      c4c::backend::prepare::as_rematerializable_integer_immediate_fact(*home);
  return fact.has_value() ? std::optional<std::int64_t>{fact->signed_value}
                          : std::nullopt;
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

std::optional<std::string> emit_riscv_simple_compare_value(
    const c4c::backend::bir::BinaryInst& binary,
    std::string_view destination_register,
    const PreparedCurrentInstructionContext& context);

std::optional<std::string> emit_riscv_simple_compare_value(
    const c4c::backend::bir::BinaryInst& binary,
    std::string_view destination_register,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  return emit_riscv_simple_compare_value(
      binary,
      destination_register,
      PreparedCurrentInstructionContext{
          .names = names,
          .lookups = lookups,
      });
}

std::optional<std::string> emit_riscv_simple_compare_value(
    const c4c::backend::bir::BinaryInst& binary,
    std::string_view destination_register,
    const PreparedCurrentInstructionContext& context) {
  if (!c4c::backend::bir::is_compare_opcode(binary.opcode)) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", context, binary.lhs) ||
      !emit_move_to_register(out, "t4", context, binary.rhs)) {
    return std::nullopt;
  }

  const std::string destination{destination_register};
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
      out += "    xor " + destination + ", t3, t4\n";
      out += "    sltiu " + destination + ", " + destination + ", 1\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Ne:
      out += "    xor " + destination + ", t3, t4\n";
      out += "    sltu " + destination + ", zero, " + destination + "\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Slt:
      out += "    slt " + destination + ", t3, t4\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Sgt:
      out += "    slt " + destination + ", t4, t3\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Sle:
      out += "    slt " + destination + ", t4, t3\n";
      out += "    xori " + destination + ", " + destination + ", 1\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Sge:
      out += "    slt " + destination + ", t3, t4\n";
      out += "    xori " + destination + ", " + destination + ", 1\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Ult:
      out += "    sltu " + destination + ", t3, t4\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Ugt:
      out += "    sltu " + destination + ", t4, t3\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Ule:
      out += "    sltu " + destination + ", t4, t3\n";
      out += "    xori " + destination + ", " + destination + ", 1\n";
      return out;
    case c4c::backend::bir::BinaryOpcode::Uge:
      out += "    sltu " + destination + ", t3, t4\n";
      out += "    xori " + destination + ", " + destination + ", 1\n";
      return out;
    default:
      return std::nullopt;
  }
}

bool emit_move_to_i32_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial);

bool emit_move_to_i16_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial);

bool emit_move_to_pointer_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial);

const c4c::backend::bir::SelectInst* find_same_block_select_producer(
    const c4c::backend::bir::Block* block,
    const c4c::backend::bir::Value& value,
    std::size_t before_instruction_index,
    std::size_t* producer_instruction_index) {
  if (block == nullptr ||
      value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const std::size_t limit = std::min(before_instruction_index, block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto& inst = block->insts[index - 1];
    const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst);
    if (select == nullptr ||
        select->result.kind != c4c::backend::bir::Value::Kind::Named ||
        select->result.name != value.name) {
      continue;
    }
    if (producer_instruction_index != nullptr) {
      *producer_instruction_index = index - 1;
    }
    return select;
  }
  return nullptr;
}

const c4c::backend::bir::BinaryInst* find_immediate_compare_producer(
    const c4c::backend::bir::Block* block,
    const c4c::backend::bir::Value& value,
    std::size_t instruction_index) {
  if (block == nullptr ||
      instruction_index == 0 ||
      instruction_index > block->insts.size() ||
      value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.type != c4c::backend::bir::TypeKind::I32 ||
      value.name.empty()) {
    return nullptr;
  }
  const auto& inst = block->insts[instruction_index - 1U];
  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      !c4c::backend::bir::is_compare_opcode(binary->opcode) ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.name != value.name ||
      binary->result.type != c4c::backend::bir::TypeKind::I32) {
    return nullptr;
  }
  return binary;
}

std::optional<std::string> emit_select_to_i32_location(
    const c4c::backend::bir::SelectInst& select,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if (depth > 16U || select.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto mnemonic = riscv_branch_mnemonic(select.predicate);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", context, select.lhs) ||
      !emit_move_to_register(out, "t4", context, select.rhs)) {
    return std::nullopt;
  }

  const std::size_t label_id = label_serial++;
  const std::string label_base = ".L" + std::string(function_name) + "_select_" +
                                 std::to_string(context.block_label) + "_" +
                                 std::to_string(context.instruction_index) + "_" +
                                 std::to_string(label_id);
  const std::string true_label = label_base + "_true";
  const std::string done_label = label_base + "_done";
  out += "    " + *mnemonic + " t3, t4, " + true_label + "\n";
  if (!emit_move_to_i32_location(out,
                                 destination_home,
                                 context,
                                 select.false_value,
                                 function_name,
                                 block,
                                 depth + 1U,
                                 label_serial)) {
    return std::nullopt;
  }
  out += "    j " + done_label + "\n";
  out += true_label + ":\n";
  if (!emit_move_to_i32_location(out,
                                 destination_home,
                                 context,
                                 select.true_value,
                                 function_name,
                                 block,
                                 depth + 1U,
                                 label_serial)) {
    return std::nullopt;
  }
  out += done_label + ":\n";
  return out;
}

std::optional<std::string> emit_select_to_i16_location(
    const c4c::backend::bir::SelectInst& select,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if (depth > 16U || select.result.type != c4c::backend::bir::TypeKind::I16) {
    return std::nullopt;
  }
  const auto mnemonic = riscv_branch_mnemonic(select.predicate);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", context, select.lhs) ||
      !emit_move_to_register(out, "t4", context, select.rhs)) {
    return std::nullopt;
  }

  const std::size_t label_id = label_serial++;
  const std::string label_base = ".L" + std::string(function_name) + "_select_" +
                                 std::to_string(context.block_label) + "_" +
                                 std::to_string(context.instruction_index) + "_" +
                                 std::to_string(label_id);
  const std::string true_label = label_base + "_true";
  const std::string done_label = label_base + "_done";
  out += "    " + *mnemonic + " t3, t4, " + true_label + "\n";
  if (!emit_move_to_i16_location(out,
                                 destination_home,
                                 context,
                                 select.false_value,
                                 function_name,
                                 block,
                                 depth + 1U,
                                 label_serial)) {
    return std::nullopt;
  }
  out += "    j " + done_label + "\n";
  out += true_label + ":\n";
  if (!emit_move_to_i16_location(out,
                                 destination_home,
                                 context,
                                 select.true_value,
                                 function_name,
                                 block,
                                 depth + 1U,
                                 label_serial)) {
    return std::nullopt;
  }
  out += done_label + ":\n";
  return out;
}

std::optional<std::int64_t> prepared_local_frame_address_offset_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    const c4c::backend::bir::Value& value) {
  namespace bir = c4c::backend::bir;

  if (value.kind != bir::Value::Kind::Named ||
      value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return std::nullopt;
  }

  std::optional<std::int64_t> selected_offset;
  for (const auto& object : prepared.stack_layout.objects) {
    if (object.function_name != function_id ||
        c4c::backend::prepare::prepared_stack_object_name(prepared.names, object) !=
            value.name ||
        (!object.address_exposed && !object.permanent_home_slot)) {
      continue;
    }
    const auto* slot =
        c4c::backend::prepare::find_prepared_frame_slot(prepared.stack_layout,
                                                        object.object_id);
    if (slot == nullptr ||
        slot->function_name != function_id ||
        !fits_signed_12_bit_immediate(static_cast<std::int64_t>(slot->offset_bytes))) {
      continue;
    }
    const auto offset = static_cast<std::int64_t>(slot->offset_bytes);
    if (selected_offset.has_value() && *selected_offset != offset) {
      return std::nullopt;
    }
    selected_offset = offset;
  }
  return selected_offset;
}

bool emit_move_to_pointer_register(
    std::string& out,
    std::string_view destination_register,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name) {
  if (value.type != c4c::backend::bir::TypeKind::Ptr) {
    return false;
  }
  if (const auto frame_offset =
          prepared_local_frame_address_offset_for(prepared, function_name, value);
      frame_offset.has_value()) {
    out += "    addi ";
    out += destination_register;
    out += ", sp, ";
    out += std::to_string(*frame_offset);
    out += "\n";
    return true;
  }
  if (emit_move_to_register(out, destination_register, context, value)) {
    return true;
  }
  const auto* home = prepared_value_home_for(context, value);
  if (home != nullptr &&
      home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value() &&
      home->size_bytes == std::optional<std::size_t>{8} &&
      fits_signed_12_bit_immediate(static_cast<std::int64_t>(*home->offset_bytes))) {
    out += "    ld ";
    out += destination_register;
    out += ", ";
    out += std::to_string(*home->offset_bytes);
    out += "(sp)\n";
    return true;
  }
  return false;
}

std::optional<std::string> emit_select_to_pointer_location(
    const c4c::backend::bir::SelectInst& select,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if (depth > 16U || select.result.type != c4c::backend::bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  const auto mnemonic = riscv_branch_mnemonic(select.predicate);
  if (!mnemonic.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", context, select.lhs) ||
      !emit_move_to_register(out, "t4", context, select.rhs)) {
    return std::nullopt;
  }

  const std::size_t label_id = label_serial++;
  const std::string label_base = ".L" + std::string(function_name) + "_select_" +
                                 std::to_string(context.block_label) + "_" +
                                 std::to_string(context.instruction_index) + "_" +
                                 std::to_string(label_id);
  const std::string true_label = label_base + "_true";
  const std::string done_label = label_base + "_done";
  out += "    " + *mnemonic + " t3, t4, " + true_label + "\n";
  if (!emit_move_to_pointer_location(out,
                                     destination_home,
                                     prepared,
                                     context,
                                     select.false_value,
                                     function_name,
                                     block,
                                     depth + 1U,
                                     label_serial)) {
    return std::nullopt;
  }
  out += "    j " + done_label + "\n";
  out += true_label + ":\n";
  if (!emit_move_to_pointer_location(out,
                                     destination_home,
                                     prepared,
                                     context,
                                     select.true_value,
                                     function_name,
                                     block,
                                     depth + 1U,
                                     label_serial)) {
    return std::nullopt;
  }
  out += done_label + ":\n";
  return out;
}

bool emit_move_to_i32_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      destination_home.register_name.has_value()) {
    if (const auto* compare_producer =
            find_immediate_compare_producer(block, value, context.instruction_index);
        compare_producer != nullptr) {
      const auto emitted = emit_riscv_simple_compare_value(
          *compare_producer,
          *destination_home.register_name,
          context.names,
          context.lookups);
      if (emitted.has_value()) {
        out += *emitted;
        return true;
      }
    }
    if (emit_move_to_register(out, *destination_home.register_name, context, value)) {
      return true;
    }
    std::size_t producer_instruction_index = 0;
    const auto* nested_select = find_same_block_select_producer(
        block,
        value,
        context.instruction_index,
        &producer_instruction_index);
    if (nested_select == nullptr) {
      return false;
    }
    const PreparedCurrentInstructionContext nested_context{
        .names = context.names,
        .lookups = context.lookups,
        .block_index = context.block_index,
        .block_label = context.block_label,
        .instruction_index = producer_instruction_index,
    };
    const auto emitted = emit_select_to_i32_location(
        *nested_select,
        function_name,
        nested_context,
        destination_home,
        block,
        depth,
        label_serial);
    if (!emitted.has_value()) {
      return false;
    }
    out += *emitted;
    return true;
  }
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      destination_home.size_bytes == std::optional<std::size_t>{4} &&
      fits_signed_12_bit_load_offset(*destination_home.offset_bytes)) {
    bool materialized = false;
    if (const auto* compare_producer =
            find_immediate_compare_producer(block, value, context.instruction_index);
        compare_producer != nullptr) {
      const auto emitted = emit_riscv_simple_compare_value(
          *compare_producer,
          "t3",
          context.names,
          context.lookups);
      if (emitted.has_value()) {
        out += *emitted;
        materialized = true;
      }
    }
    if (!materialized && !emit_move_to_register(out, "t3", context, value)) {
      std::size_t producer_instruction_index = 0;
      const auto* nested_select = find_same_block_select_producer(
          block,
          value,
          context.instruction_index,
          &producer_instruction_index);
      if (nested_select == nullptr) {
        return false;
      }
      const PreparedCurrentInstructionContext nested_context{
          .names = context.names,
          .lookups = context.lookups,
          .block_index = context.block_index,
          .block_label = context.block_label,
          .instruction_index = producer_instruction_index,
      };
      const c4c::backend::prepare::PreparedValueHome register_home{
          .kind = c4c::backend::prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"t3"},
      };
      const auto emitted = emit_select_to_i32_location(
          *nested_select,
          function_name,
          nested_context,
          register_home,
          block,
          depth,
          label_serial);
      if (!emitted.has_value()) {
        return false;
      }
      out += *emitted;
    }
    out += "    sw t3, ";
    out += std::to_string(*destination_home.offset_bytes);
    out += "(sp)\n";
    return true;
  }
  return false;
}

bool emit_move_to_i16_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      destination_home.register_name.has_value()) {
    if (emit_move_to_register(out, *destination_home.register_name, context, value)) {
      return true;
    }
    std::size_t producer_instruction_index = 0;
    const auto* nested_select = find_same_block_select_producer(
        block,
        value,
        context.instruction_index,
        &producer_instruction_index);
    if (nested_select == nullptr) {
      return false;
    }
    const PreparedCurrentInstructionContext nested_context{
        .names = context.names,
        .lookups = context.lookups,
        .block_index = context.block_index,
        .block_label = context.block_label,
        .instruction_index = producer_instruction_index,
    };
    const auto emitted = emit_select_to_i16_location(
        *nested_select,
        function_name,
        nested_context,
        destination_home,
        block,
        depth,
        label_serial);
    if (!emitted.has_value()) {
      return false;
    }
    out += *emitted;
    return true;
  }
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      destination_home.size_bytes == std::optional<std::size_t>{2} &&
      fits_signed_12_bit_load_offset(*destination_home.offset_bytes)) {
    if (!emit_move_to_register(out, "t3", context, value)) {
      std::size_t producer_instruction_index = 0;
      const auto* nested_select = find_same_block_select_producer(
          block,
          value,
          context.instruction_index,
          &producer_instruction_index);
      if (nested_select == nullptr) {
        return false;
      }
      const PreparedCurrentInstructionContext nested_context{
          .names = context.names,
          .lookups = context.lookups,
          .block_index = context.block_index,
          .block_label = context.block_label,
          .instruction_index = producer_instruction_index,
      };
      const c4c::backend::prepare::PreparedValueHome register_home{
          .kind = c4c::backend::prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"t3"},
      };
      const auto emitted = emit_select_to_i16_location(
          *nested_select,
          function_name,
          nested_context,
          register_home,
          block,
          depth,
          label_serial);
      if (!emitted.has_value()) {
        return false;
      }
      out += *emitted;
    }
    out += "    sh t3, ";
    out += std::to_string(*destination_home.offset_bytes);
    out += "(sp)\n";
    return true;
  }
  return false;
}

bool emit_move_to_pointer_location(
    std::string& out,
    const c4c::backend::prepare::PreparedValueHome& destination_home,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::string_view function_name,
    const c4c::backend::bir::Block* block,
    std::size_t depth,
    std::size_t& label_serial) {
  if ((destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::Register ||
       destination_home.kind ==
           c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset) &&
      destination_home.register_name.has_value()) {
    if (emit_move_to_pointer_register(
            out,
            *destination_home.register_name,
            prepared,
            context,
            value,
            function_name)) {
      return true;
    }
    std::size_t producer_instruction_index = 0;
    const auto* nested_select = find_same_block_select_producer(
        block,
        value,
        context.instruction_index,
        &producer_instruction_index);
    if (nested_select == nullptr) {
      return false;
    }
    const PreparedCurrentInstructionContext nested_context{
        .names = context.names,
        .lookups = context.lookups,
        .block_index = context.block_index,
        .block_label = context.block_label,
        .instruction_index = producer_instruction_index,
    };
    const auto emitted = emit_select_to_pointer_location(
        *nested_select,
        prepared,
        function_name,
        nested_context,
        destination_home,
        block,
        depth,
        label_serial);
    if (!emitted.has_value()) {
      return false;
    }
    out += *emitted;
    return true;
  }
  if (destination_home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      destination_home.size_bytes == std::optional<std::size_t>{8} &&
      fits_signed_12_bit_immediate(static_cast<std::int64_t>(*destination_home.offset_bytes))) {
    if (!emit_move_to_pointer_register(
            out,
            "t3",
            prepared,
            context,
            value,
            function_name)) {
      std::size_t producer_instruction_index = 0;
      const auto* nested_select = find_same_block_select_producer(
          block,
          value,
          context.instruction_index,
          &producer_instruction_index);
      if (nested_select == nullptr) {
        return false;
      }
      const PreparedCurrentInstructionContext nested_context{
          .names = context.names,
          .lookups = context.lookups,
          .block_index = context.block_index,
          .block_label = context.block_label,
          .instruction_index = producer_instruction_index,
      };
      const c4c::backend::prepare::PreparedValueHome register_home{
          .kind = c4c::backend::prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"t3"},
      };
      const auto emitted = emit_select_to_pointer_location(
          *nested_select,
          prepared,
          function_name,
          nested_context,
          register_home,
          block,
          depth,
          label_serial);
      if (!emitted.has_value()) {
        return false;
      }
      out += *emitted;
    }
    out += "    sd t3, ";
    out += std::to_string(*destination_home.offset_bytes);
    out += "(sp)\n";
    return true;
  }
  return false;
}

std::uint32_t encode_i_type(std::uint32_t opcode, std::uint32_t rd,
                            std::uint32_t funct3, std::uint32_t rs1,
                            std::int32_t imm12) {
  return rv64_encode_i_type(opcode, rd, funct3, rs1, imm12);
}


std::uint32_t encode_s_type(std::uint32_t opcode, std::uint32_t funct3,
                            std::uint32_t rs1, std::uint32_t rs2,
                            std::int32_t imm12) {
  return rv64_encode_s_type(opcode, funct3, rs1, rs2, imm12);
}


std::uint32_t encode_r_type(std::uint32_t opcode, std::uint32_t rd,
                            std::uint32_t funct3, std::uint32_t rs1,
                            std::uint32_t rs2, std::uint32_t funct7) {
  return rv64_encode_r_type(opcode, rd, funct3, rs1, rs2, funct7);
}


std::uint32_t encode_b_type(std::uint32_t opcode, std::uint32_t funct3,
                            std::uint32_t rs1, std::uint32_t rs2,
                            std::int32_t imm13) {
  return rv64_encode_b_type(opcode, funct3, rs1, rs2, imm13);
}


std::uint32_t encode_j_type(std::uint32_t opcode, std::uint32_t rd,
                            std::int32_t imm21) {
  return rv64_encode_j_type(opcode, rd, imm21);
}


void append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  rv64_append_le32(bytes, word);
}


std::optional<std::uint32_t> rv64_register_number(std::string_view name) {
  return rv64_prepared_register_number(name);
}


std::optional<std::uint32_t> gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  return rv64_prepared_gpr_register_number_for_home(home);
}


std::optional<std::uint32_t> fpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != c4c::backend::prepare::PreparedRegisterBank::Fpr ||
      identity.register_class !=
          c4c::backend::prepare::PreparedRegisterClass::Float ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}


std::optional<std::uint32_t> rv64_gpr_to_fpr_move_funct7(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x78;  // fmv.w.x
    case c4c::backend::bir::TypeKind::F64:
      return 0x79;  // fmv.d.x
    default:
      return std::nullopt;
  }
}


std::optional<std::int64_t> integer_immediate_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  switch (value.type) {
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
      break;
    default:
      return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(home);
  if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  const auto fact = prepare::as_rematerializable_integer_immediate_fact(*home);
  return fact.has_value() ? std::optional<std::int64_t>{fact->signed_value}
                          : std::nullopt;
}


bool is_rv64_null_pointer_value(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         value.type == c4c::backend::bir::TypeKind::Ptr &&
         value.immediate == 0 && value.immediate_bits == 0;
}


std::optional<std::int64_t> materializable_fpr_immediate_bits(
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  std::uint64_t bits = 0;
  switch (value.type) {
    case c4c::backend::bir::TypeKind::F32:
      bits = value.immediate_bits & 0xffffffffu;
      break;
    case c4c::backend::bir::TypeKind::F64:
      bits = value.immediate_bits;
      break;
    default:
      return std::nullopt;
  }
  if (bits > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  const auto immediate = static_cast<std::int64_t>(bits);
  return fits_signed_12_bit_immediate(immediate) ? std::optional{immediate}
                                                 : std::nullopt;
}


std::optional<std::uint32_t> gpr_register_number_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt : gpr_register_number_for_home(*home);
}


std::optional<std::size_t> rv64_scalar_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return std::size_t{1};
    case c4c::backend::bir::TypeKind::I16:
      return std::size_t{2};
    case c4c::backend::bir::TypeKind::I32:
      return std::size_t{4};
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return std::size_t{8};
    default:
      return std::nullopt;
  }
}


std::optional<unsigned> rv64_integer_type_bits(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return 8U;
    case c4c::backend::bir::TypeKind::I16:
      return 16U;
    case c4c::backend::bir::TypeKind::I32:
      return 32U;
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return 64U;
    default:
      return std::nullopt;
  }
}


bool rv64_fixed_integer_type(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
    case c4c::backend::bir::TypeKind::I64:
      return true;
    default:
      return false;
  }
}


bool rv64_floating_type(c4c::backend::bir::TypeKind type) {
  return type == c4c::backend::bir::TypeKind::F32 ||
         type == c4c::backend::bir::TypeKind::F64;
}


std::optional<std::size_t> prepared_stack_slot_home_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  return rv64_prepared_stack_slot_home_absolute_offset(
      stack_layout, home, stack_frame_bytes, size_bytes);
}


std::optional<std::int32_t> prepared_stack_slot_home_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes = 4) {
  return rv64_prepared_stack_slot_home_offset(
      stack_layout, home, stack_frame_bytes, size_bytes);
}


std::optional<std::size_t> prepared_stack_slot_home_absolute_offset_for_value(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  return prepared_stack_slot_home_absolute_offset(stack_layout,
                                                  *home,
                                                  stack_frame_bytes,
                                                  *size_bytes);
}


void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source) {
  append_rv64_prepared_move(fragment, destination, source);
}


void append_rv64_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate) {
  append_rv64_prepared_load_immediate(fragment, destination, immediate);
}


bool append_rv64_store_register_to_stack_offset(RiscvEncodedFragment& fragment,
                                                std::uint32_t source_register,
                                                std::size_t offset,
                                                std::size_t size_bytes) {
  return append_rv64_prepared_store_register_to_stack_offset(
      fragment, source_register, offset, size_bytes);
}


bool append_rv64_load_stack_offset_to_register(RiscvEncodedFragment& fragment,
                                               std::uint32_t destination_register,
                                               std::size_t offset,
                                               std::size_t size_bytes) {
  return append_rv64_prepared_load_stack_offset_to_register(
      fragment, destination_register, offset, size_bytes);
}


bool append_rv64_load_stack_to_register(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination_register,
                                        std::int32_t offset,
                                        std::size_t size_bytes = 4) {
  return append_rv64_prepared_load_stack_to_register(
      fragment, destination_register, offset, size_bytes);
}


bool append_rv64_stack_pointer_adjustment(RiscvEncodedFragment& fragment,
                                          std::int64_t byte_delta) {
  return append_rv64_prepared_stack_pointer_adjustment(fragment, byte_delta);
}


bool append_rv64_gpr_to_fpr_move(RiscvEncodedFragment& fragment,
                                 std::uint32_t destination,
                                 std::uint32_t source,
                                 c4c::backend::bir::TypeKind type) {
  const auto funct7 = rv64_gpr_to_fpr_move_funct7(type);
  if (!funct7.has_value()) {
    return false;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53, destination, 0, source, 0, *funct7));
  return true;
}


std::optional<std::uint32_t> rv64_branch_funct3(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq: return 0;
    case c4c::backend::bir::BinaryOpcode::Ne: return 1;
    case c4c::backend::bir::BinaryOpcode::Slt: return 4;
    case c4c::backend::bir::BinaryOpcode::Sge: return 5;
    case c4c::backend::bir::BinaryOpcode::Ult: return 6;
    case c4c::backend::bir::BinaryOpcode::Uge: return 7;
    default: return std::nullopt;
  }
}


struct Rv64NormalizedBranchPredicate {
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

std::optional<Rv64NormalizedBranchPredicate> normalize_rv64_branch_predicate(
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  if (lhs.type == c4c::backend::bir::TypeKind::Ptr ||
      rhs.type == c4c::backend::bir::TypeKind::Ptr) {
    if (opcode == c4c::backend::bir::BinaryOpcode::Ne &&
        lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        lhs.type == c4c::backend::bir::TypeKind::Ptr &&
        is_rv64_null_pointer_value(rhs)) {
      return Rv64NormalizedBranchPredicate{
          .opcode = opcode,
          .lhs = lhs,
          .rhs = rhs,
      };
    }
    return std::nullopt;
  }
  if (rv64_branch_funct3(opcode).has_value()) {
    return Rv64NormalizedBranchPredicate{
        .opcode = opcode,
        .lhs = lhs,
        .rhs = rhs,
    };
  }
  const bool matching_scalar_integer_operands =
      lhs.type == rhs.type &&
      lhs.type == c4c::backend::bir::TypeKind::I32;
  if (matching_scalar_integer_operands) {
    switch (opcode) {
      case c4c::backend::bir::BinaryOpcode::Sgt:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Slt,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Sle:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Sge,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Ugt:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Ult,
            .lhs = rhs,
            .rhs = lhs,
        };
      case c4c::backend::bir::BinaryOpcode::Ule:
        return Rv64NormalizedBranchPredicate{
            .opcode = c4c::backend::bir::BinaryOpcode::Uge,
            .lhs = rhs,
            .rhs = lhs,
        };
      default:
        break;
    }
  }
  return std::nullopt;
}


void append_rv64_local_jump(RiscvEncodedFragment& fragment,
                            std::string target_label) {
  const auto offset = fragment.bytes.size();
  append_le32(fragment.bytes, encode_j_type(0x6f, 0, 0));  // jal zero, target
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = offset,
      .kind = RiscvObjectFixupKind::Jal,
      .symbol_name = std::move(target_label),
      .addend = 0,
  });
}


void append_rv64_local_branch(RiscvEncodedFragment& fragment,
                              std::uint32_t funct3,
                              std::uint32_t lhs_register,
                              std::uint32_t rhs_register,
                              std::string target_label) {
  const auto offset = fragment.bytes.size();
  append_le32(fragment.bytes,
              encode_b_type(0x63, funct3, lhs_register, rhs_register, 0));
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = offset,
      .kind = RiscvObjectFixupKind::Branch,
      .symbol_name = std::move(target_label),
      .addend = 0,
  });
}


std::optional<std::size_t> rv64_call_frame_size(std::size_t local_frame_bytes) {
  if (local_frame_bytes > std::numeric_limits<std::size_t>::max() - 16) {
    return std::nullopt;
  }
  return local_frame_bytes + 16;
}


std::optional<std::size_t> rv64_call_frame_ra_offset(
    std::size_t local_frame_bytes) {
  if (local_frame_bytes > std::numeric_limits<std::size_t>::max() - 8) {
    return std::nullopt;
  }
  return local_frame_bytes + 8;
}


bool rv64_is_callee_saved_gpr_register_name(std::string_view name) {
  return name == "s0" || name == "fp" || name == "s1" || name == "s2" ||
         name == "s3" || name == "s4" || name == "s5" || name == "s6" ||
         name == "s7" || name == "s8" || name == "s9" || name == "s10" ||
         name == "s11";
}


std::optional<std::int32_t> rv64_saved_callee_gpr_stack_offset(
    const c4c::backend::prepare::PreparedSavedRegister& saved,
    std::size_t stack_frame_bytes) {
  namespace prepare = c4c::backend::prepare;

  if (saved.bank != prepare::PreparedRegisterBank::Gpr ||
      saved.register_name.empty() ||
      !rv64_is_callee_saved_gpr_register_name(saved.register_name) ||
      !rv64_register_number(saved.register_name).has_value() ||
      saved.contiguous_width != 1 ||
      saved.occupied_register_names.size() != 1 ||
      saved.occupied_register_names.front() != saved.register_name ||
      !saved.placement.has_value() ||
      saved.placement->bank != prepare::PreparedRegisterBank::Gpr ||
      saved.placement->pool != prepare::PreparedRegisterSlotPool::CalleeSaved ||
      saved.placement->contiguous_width != 1 ||
      !saved.slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *saved.slot_placement)) {
    return std::nullopt;
  }

  const auto& slot = *saved.slot_placement;
  if (slot.bank != prepare::PreparedRegisterBank::Gpr ||
      slot.register_name != saved.register_name ||
      slot.contiguous_width != 1 ||
      slot.occupied_register_names.size() != 1 ||
      slot.occupied_register_names.front() != saved.register_name ||
      !slot.register_placement.has_value() ||
      slot.register_placement != saved.placement ||
      !slot.stack_offset_bytes.has_value() ||
      !slot.size_bytes.has_value() ||
      *slot.size_bytes != 8 ||
      slot.stack_offset_bytes > std::optional<std::size_t>{stack_frame_bytes} ||
      stack_frame_bytes - *slot.stack_offset_bytes < *slot.size_bytes ||
      *slot.stack_offset_bytes >
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
      !fits_signed_12_bit_immediate(
          static_cast<std::int64_t>(*slot.stack_offset_bytes))) {
    return std::nullopt;
  }

  return static_cast<std::int32_t>(*slot.stack_offset_bytes);
}


bool append_rv64_saved_callee_gpr_restores(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  if (frame_plan == nullptr) {
    return true;
  }
  for (auto it = frame_plan->saved_callee_registers.rbegin();
       it != frame_plan->saved_callee_registers.rend();
       ++it) {
    const auto destination = rv64_register_number(it->register_name);
    const auto offset = rv64_saved_callee_gpr_stack_offset(*it, stack_frame_bytes);
    if (!destination.has_value() || !offset.has_value() ||
        !append_rv64_load_stack_to_register(fragment, *destination, *offset, 8)) {
      return false;
    }
  }
  return true;
}


bool append_rv64_call_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t local_frame_bytes) {
  return append_rv64_prepared_call_frame_epilogue(
      fragment, frame_plan, local_frame_bytes);
}


bool append_rv64_stack_frame_epilogue(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    std::size_t stack_frame_bytes) {
  return append_rv64_prepared_stack_frame_epilogue(
      fragment, frame_plan, stack_frame_bytes);
}


}  // namespace

bool append_rv64_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (is_rv64_null_pointer_value(value)) {
    append_rv64_load_immediate(fragment, destination, 0);
    return true;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, value);
  if (immediate.has_value()) {
    append_rv64_load_immediate(fragment, destination, *immediate);
    return true;
  }
  const auto source = gpr_register_number_for_value(names, lookups, value);
  if (source.has_value()) {
    append_rv64_move(fragment, destination, *source);
    return true;
  }
  const auto stack_offset =
      prepared_stack_slot_home_absolute_offset_for_value(stack_layout,
                                                         names,
                                                         lookups,
                                                         value,
                                                         stack_frame_bytes);
  if (stack_offset.has_value()) {
    const auto size_bytes = rv64_scalar_memory_size_for_type(value.type);
    if (!size_bytes.has_value()) {
      return false;
    }
    return append_rv64_load_stack_offset_to_register(fragment,
                                                    destination,
                                                    *stack_offset,
                                                    *size_bytes);
  }
  return false;
}

[[nodiscard]] bool append_prior_preserved_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedCallPreservedValue& preserved,
    c4c::backend::bir::TypeKind type) {
  namespace prepare = c4c::backend::prepare;

  if (preserved.route == prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    if (preserved.register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !preserved.register_name.has_value() || preserved.register_name->empty() ||
        preserved.contiguous_width != 1 || preserved.occupied_register_names.empty() ||
        !preserved.register_placement.has_value()) {
      return false;
    }
    const auto source = rv64_prepared_register_number(*preserved.register_name);
    if (!source.has_value()) {
      return false;
    }
    if (*source != destination) {
      append_rv64_move(fragment, destination, *source);
    }
    return true;
  }

  if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
      !preserved.stack_offset_bytes.has_value() ||
      !preserved.stack_size_bytes.has_value()) {
    return false;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(type);
  if (!size_bytes.has_value() || *size_bytes != *preserved.stack_size_bytes) {
    return false;
  }
  return append_rv64_load_stack_offset_to_register(fragment,
                                                  destination,
                                                  *preserved.stack_offset_bytes,
                                                  *size_bytes);
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_return(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::bir::Terminator& terminator,
    std::size_t block_index,
    std::size_t terminator_instruction_index,
    const std::unordered_set<PreparedBeforeReturnStackToRegisterKey,
                             PreparedBeforeReturnStackToRegisterKeyHash>*
        prepared_before_return_stack_to_register_values,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.return_lanes.empty()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  if (!terminator.value.has_value()) {
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  if ((terminator.value->type == c4c::backend::bir::TypeKind::F32 ||
       terminator.value->type == c4c::backend::bir::TypeKind::F64) &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      !terminator.value->name.empty() && lookups != nullptr) {
    const auto value_name = names.value_names.find(terminator.value->name);
    const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
    if (value_name != c4c::kInvalidValueName &&
        value_id_it != lookups->value_homes.value_ids.end() &&
        prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
            &lookups->move_bundles,
            nullptr,
            block_index,
            value_id_it->second,
            prepare::PreparedRegisterBank::Fpr) != nullptr) {
      if (restore_return_address) {
        if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
          return std::nullopt;
        }
      } else if (!append_rv64_stack_frame_epilogue(
                     fragment,
                     frame_plan,
                     stack_frame_bytes)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
      return fragment;
    }
  }
  if (!rv64_floating_type(terminator.value->type) &&
      terminator.value->type != c4c::backend::bir::TypeKind::Ptr &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      !terminator.value->name.empty() && lookups != nullptr) {
    const auto terminator_value_name =
        names.value_names.find(terminator.value->name);
    const auto terminator_value_id =
        terminator_value_name == c4c::kInvalidValueName
            ? lookups->value_homes.value_ids.end()
            : lookups->value_homes.value_ids.find(terminator_value_name);
    const bool prepared_return_value_already_loaded =
        terminator_value_name != c4c::kInvalidValueName &&
        terminator_value_id != lookups->value_homes.value_ids.end() &&
        prepared_before_return_stack_to_register_values != nullptr &&
        prepared_before_return_stack_to_register_values->count(
            PreparedBeforeReturnStackToRegisterKey{
                .block_index = block_index,
                .value_id = terminator_value_id->second,
            }) != 0;
    if (prepared_return_value_already_loaded) {
      if (restore_return_address) {
        if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
          return std::nullopt;
        }
      } else if (!append_rv64_stack_frame_epilogue(
                     fragment,
                     frame_plan,
                     stack_frame_bytes)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
      return fragment;
    }
  }
  if (const auto bits = materializable_fpr_immediate_bits(*terminator.value)) {
    constexpr std::uint32_t scratch = 5;  // t0
    constexpr std::uint32_t return_fpr = 10;  // fa0
    append_rv64_load_immediate(fragment, scratch, *bits);
    if (!append_rv64_gpr_to_fpr_move(
            fragment, return_fpr, scratch, terminator.value->type)) {
      return std::nullopt;
    }
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  const bool direct_global_pointer_return_candidate =
      terminator.value->type == c4c::backend::bir::TypeKind::Ptr &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      (terminator.value->pointer_symbol_link_name_id != c4c::kInvalidLinkName ||
       (!terminator.value->name.empty() && terminator.value->name.front() == '@'));
  if (direct_global_pointer_return_candidate) {
    const auto* value_home = prepared_value_home_for(names, lookups, *terminator.value);
    const auto* before_return_move =
        value_home == nullptr
            ? nullptr
            : prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
                  lookups == nullptr ? nullptr : &lookups->move_bundles,
                  nullptr,
                  block_index,
                  value_home->value_id,
                  prepare::PreparedRegisterBank::Gpr);
    const auto authority =
        prepare::plan_prepared_direct_global_return_authority({
            .names = &names,
            .return_value = &*terminator.value,
            .value_home = value_home,
            .before_return_move = before_return_move,
            .block_index = block_index,
            .instruction_index = terminator_instruction_index,
        });
    if (!prepare::prepared_direct_global_return_authority_available(authority) ||
        authority.before_return_move == nullptr || authority.value_home == nullptr) {
      return std::nullopt;
    }
    const auto source = gpr_register_number_for_home(*authority.value_home);
    const auto destination =
        rv64_register_number(*authority.before_return_move->destination_register_name);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    if (restore_return_address) {
      if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
        return std::nullopt;
      }
    } else if (!append_rv64_stack_frame_epilogue(
                   fragment,
                   frame_plan,
                   stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  if (!append_rv64_move_value_to_register(fragment,
                                          10,
                                          stack_layout,
                                          names,
                                          lookups,
                                          *terminator.value,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  if (restore_return_address) {
    if (!append_rv64_call_frame_epilogue(fragment, frame_plan, stack_frame_bytes)) {
      return std::nullopt;
    }
  } else if (!append_rv64_stack_frame_epilogue(
                 fragment,
                 frame_plan,
                 stack_frame_bytes)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
}


void append_rv64_zero_extend_register(RiscvEncodedFragment& fragment,
                                      std::uint32_t destination,
                                      std::uint32_t source,
                                      unsigned source_bits) {
  if (source_bits == 64U) {
    append_rv64_move(fragment, destination, source);
    return;
  }
  if (source_bits == 8U) {
    append_le32(fragment.bytes,
                encode_i_type(0x13, destination, 7, source, 0xff));
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            1,
                            source,
                            static_cast<std::int32_t>(64U - source_bits)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            5,
                            destination,
                            static_cast<std::int32_t>(64U - source_bits)));
}


void append_rv64_sign_extend_register(RiscvEncodedFragment& fragment,
                                      std::uint32_t destination,
                                      std::uint32_t source,
                                      unsigned source_bits) {
  if (source_bits == 64U) {
    append_rv64_move(fragment, destination, source);
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            1,
                            source,
                            static_cast<std::int32_t>(64U - source_bits)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            5,
                            destination,
                            static_cast<std::int32_t>(0x400U | (64U - source_bits))));
}


std::optional<std::uint32_t> rv64_int_to_fp_cast_rs2(
    c4c::backend::bir::CastOpcode opcode,
    c4c::backend::bir::TypeKind source_type) {
  switch (source_type) {
    case c4c::backend::bir::TypeKind::I32:
      if (opcode == c4c::backend::bir::CastOpcode::SIToFP) {
        return 0;  // fcvt.{s,d}.w
      }
      if (opcode == c4c::backend::bir::CastOpcode::UIToFP) {
        return 1;  // fcvt.{s,d}.wu
      }
      return std::nullopt;
    case c4c::backend::bir::TypeKind::I64:
      if (opcode == c4c::backend::bir::CastOpcode::SIToFP) {
        return 2;  // fcvt.{s,d}.l
      }
      if (opcode == c4c::backend::bir::CastOpcode::UIToFP) {
        return 3;  // fcvt.{s,d}.lu
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}


std::optional<std::uint32_t> rv64_int_to_fp_cast_funct7(
    c4c::backend::bir::TypeKind result_type) {
  switch (result_type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x68;
    case c4c::backend::bir::TypeKind::F64:
      return 0x69;
    default:
      return std::nullopt;
  }
}


std::optional<std::uint32_t> rv64_fp_to_int_cast_rs2(
    c4c::backend::bir::CastOpcode opcode,
    c4c::backend::bir::TypeKind result_type) {
  switch (result_type) {
    case c4c::backend::bir::TypeKind::I32:
      if (opcode == c4c::backend::bir::CastOpcode::FPToSI) {
        return 0;  // fcvt.w.{s,d}
      }
      if (opcode == c4c::backend::bir::CastOpcode::FPToUI) {
        return 1;  // fcvt.wu.{s,d}
      }
      return std::nullopt;
    case c4c::backend::bir::TypeKind::I64:
      if (opcode == c4c::backend::bir::CastOpcode::FPToSI) {
        return 2;  // fcvt.l.{s,d}
      }
      if (opcode == c4c::backend::bir::CastOpcode::FPToUI) {
        return 3;  // fcvt.lu.{s,d}
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}


std::optional<std::uint32_t> rv64_fp_to_int_cast_funct7(
    c4c::backend::bir::TypeKind source_type) {
  switch (source_type) {
    case c4c::backend::bir::TypeKind::F32:
      return 0x60;
    case c4c::backend::bir::TypeKind::F64:
      return 0x61;
    default:
      return std::nullopt;
  }
}


std::optional<std::int64_t> rv64_fp_immediate_bits_as_i64(
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      (value.type != c4c::backend::bir::TypeKind::F32 &&
       value.type != c4c::backend::bir::TypeKind::F64)) {
    return std::nullopt;
  }
  std::int64_t bits = 0;
  if (value.type == c4c::backend::bir::TypeKind::F32) {
    std::int32_t f32_bits = 0;
    const auto raw_bits = static_cast<std::uint32_t>(value.immediate_bits);
    std::memcpy(&f32_bits, &raw_bits, sizeof(f32_bits));
    bits = f32_bits;
  } else {
    std::memcpy(&bits, &value.immediate_bits, sizeof(bits));
  }
  return bits;
}


std::optional<std::uint32_t> choose_fp_scratch_fpr(
    std::initializer_list<std::optional<std::uint32_t>> occupied) {
  constexpr std::uint32_t candidates[] = {5, 6, 7, 28, 29, 30, 31};
  for (const auto candidate : candidates) {
    bool available = true;
    for (const auto value : occupied) {
      if (value.has_value() && *value == candidate) {
        available = false;
        break;
      }
    }
    if (available) {
      return candidate;
    }
  }
  return std::nullopt;
}


std::optional<std::uint32_t> materialize_fp_cast_operand(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::uint32_t scratch_fpr,
    std::uint32_t scratch_gpr) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home != nullptr) {
    return fpr_register_number_for_home(*home);
  }
  const auto bits = rv64_fp_immediate_bits_as_i64(value);
  if (!bits.has_value()) {
    return std::nullopt;
  }
  append_rv64_load_immediate(fragment, scratch_gpr, *bits);
  if (!append_rv64_gpr_to_fpr_move(
          fragment, scratch_fpr, scratch_gpr, value.type)) {
    return std::nullopt;
  }
  return scratch_fpr;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_floating_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast) {
  std::optional<std::uint32_t> rs2;
  std::optional<std::uint32_t> funct7;
  if (cast.opcode == c4c::backend::bir::CastOpcode::FPExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::F32 &&
      cast.result.type == c4c::backend::bir::TypeKind::F64) {
    rs2 = 0;
    funct7 = 0x21;  // fcvt.d.s fd, fs, rne
  } else if (cast.opcode == c4c::backend::bir::CastOpcode::FPTrunc &&
             cast.operand.type == c4c::backend::bir::TypeKind::F64 &&
             cast.result.type == c4c::backend::bir::TypeKind::F32) {
    rs2 = 1;
    funct7 = 0x20;  // fcvt.s.d fd, fs, rne
  } else {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  const auto destination =
      destination_home == nullptr ? std::nullopt : fpr_register_number_for_home(*destination_home);
  const auto source_home_register =
      source_home == nullptr ? std::nullopt : fpr_register_number_for_home(*source_home);
  const auto source_scratch = choose_fp_scratch_fpr({destination, source_home_register});
  if (!destination.has_value() || !source_scratch.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  const auto source =
      materialize_fp_cast_operand(fragment,
                                  names,
                                  lookups,
                                  cast.operand,
                                  *source_scratch,
                                  28);
  if (!source.has_value()) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53,
                            *destination,
                            0,
                            *source,
                            *rs2,
                            *funct7));
  return fragment;
}


std::optional<std::uint32_t> rv64_fp_binary_funct7(
    c4c::backend::bir::BinaryOpcode opcode,
    c4c::backend::bir::TypeKind type) {
  const std::uint32_t precision_bit =
      type == c4c::backend::bir::TypeKind::F64 ? 0x01 : 0x00;
  if (type != c4c::backend::bir::TypeKind::F32 &&
      type != c4c::backend::bir::TypeKind::F64) {
    return std::nullopt;
  }
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return 0x00 | precision_bit;  // fadd.s/fadd.d
    case c4c::backend::bir::BinaryOpcode::Sub:
      return 0x04 | precision_bit;  // fsub.s/fsub.d
    case c4c::backend::bir::BinaryOpcode::Mul:
      return 0x08 | precision_bit;  // fmul.s/fmul.d
    case c4c::backend::bir::BinaryOpcode::SDiv:
      return 0x0c | precision_bit;  // fdiv.s/fdiv.d
    default:
      return std::nullopt;
  }
}


std::optional<std::uint32_t> choose_fp_binary_scratch_fpr(
    std::initializer_list<std::optional<std::uint32_t>> occupied) {
  return choose_fp_scratch_fpr(occupied);
}


std::optional<std::uint32_t> materialize_fp_binary_operand(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::uint32_t scratch_fpr,
    std::uint32_t scratch_gpr) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home != nullptr) {
    return fpr_register_number_for_home(*home);
  }
  const auto bits = rv64_fp_immediate_bits_as_i64(value);
  if (!bits.has_value()) {
    return std::nullopt;
  }
  append_rv64_load_immediate(fragment,
                             scratch_gpr,
                             *bits);
  if (!append_rv64_gpr_to_fpr_move(
          fragment, scratch_fpr, scratch_gpr, value.type)) {
    return std::nullopt;
  }
  return scratch_fpr;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_fp_binary(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary) {
  if ((binary.result.type != c4c::backend::bir::TypeKind::F32 &&
       binary.result.type != c4c::backend::bir::TypeKind::F64) ||
      binary.operand_type != binary.result.type ||
      binary.lhs.type != binary.result.type ||
      binary.rhs.type != binary.result.type) {
    return std::nullopt;
  }
  const auto funct7 = rv64_fp_binary_funct7(binary.opcode, binary.result.type);
  if (!funct7.has_value()) {
    return std::nullopt;
  }

  const auto* destination_home = prepared_value_home_for(names, lookups, binary.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : fpr_register_number_for_home(*destination_home);
  if (!destination.has_value()) {
    return std::nullopt;
  }
  const auto* lhs_home = prepared_value_home_for(names, lookups, binary.lhs);
  const auto* rhs_home = prepared_value_home_for(names, lookups, binary.rhs);
  const auto lhs_home_register =
      lhs_home == nullptr ? std::nullopt : fpr_register_number_for_home(*lhs_home);
  const auto rhs_home_register =
      rhs_home == nullptr ? std::nullopt : fpr_register_number_for_home(*rhs_home);
  const auto lhs_scratch = choose_fp_binary_scratch_fpr(
      {*destination, lhs_home_register, rhs_home_register});
  const auto rhs_scratch = choose_fp_binary_scratch_fpr(
      {*destination, lhs_home_register, rhs_home_register, lhs_scratch});
  if (!lhs_scratch.has_value() || !rhs_scratch.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  const auto lhs =
      materialize_fp_binary_operand(fragment,
                                    names,
                                    lookups,
                                    binary.lhs,
                                    *lhs_scratch,
                                    28);
  const auto rhs =
      materialize_fp_binary_operand(fragment,
                                    names,
                                    lookups,
                                    binary.rhs,
                                    *rhs_scratch,
                                    29);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53, *destination, 0, *lhs, *rhs, *funct7));
  return fragment;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_fp_to_int_cast(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast) {
  const auto rs2 = rv64_fp_to_int_cast_rs2(cast.opcode, cast.result.type);
  const auto funct7 = rv64_fp_to_int_cast_funct7(cast.operand.type);
  if (!rs2.has_value() || !funct7.has_value()) {
    return std::nullopt;
  }

  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto source =
      source_home == nullptr ? std::nullopt : fpr_register_number_for_home(*source_home);
  if (!destination.has_value() || !source.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_r_type(0x53,
                            *destination,
                            1,
                            *source,
                            *rs2,
                            *funct7));
  return fragment;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_int_to_fp_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes) {
  const auto rs2 = rv64_int_to_fp_cast_rs2(cast.opcode, cast.operand.type);
  const auto funct7 = rv64_int_to_fp_cast_funct7(cast.result.type);
  if (!rs2.has_value() || !funct7.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : fpr_register_number_for_home(*destination_home);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  const auto source = gpr_register_number_for_value(names, lookups, cast.operand);
  const std::uint32_t source_register = source.value_or(5);
  RiscvEncodedFragment fragment;
  if (!source.has_value() &&
      !append_rv64_move_value_to_register(fragment,
                                          source_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          cast.operand,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_le32(fragment.bytes,
              encode_r_type(0x53,
                            *destination,
                            0,
                            source_register,
                            *rs2,
                            *funct7));
  return fragment;
}


bool rv64_prepared_pointer_cast_types_supported(
    c4c::backend::bir::CastOpcode opcode,
    c4c::backend::bir::TypeKind source_type,
    c4c::backend::bir::TypeKind result_type) {
  if (opcode == c4c::backend::bir::CastOpcode::IntToPtr) {
    return result_type == c4c::backend::bir::TypeKind::Ptr &&
           (source_type == c4c::backend::bir::TypeKind::I32 ||
            source_type == c4c::backend::bir::TypeKind::I64);
  }
  if (opcode == c4c::backend::bir::CastOpcode::PtrToInt) {
    return source_type == c4c::backend::bir::TypeKind::Ptr &&
           result_type == c4c::backend::bir::TypeKind::I64;
  }
  return false;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_pointer_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes) {
  if (!rv64_prepared_pointer_cast_types_supported(cast.opcode,
                                                  cast.operand.type,
                                                  cast.result.type)) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *destination_home,
                                                     stack_frame_bytes,
                                                     8);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }

  const std::uint32_t destination_register = destination.value_or(30);
  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          destination_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          cast.operand,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  if (destination_stack_offset.has_value()) {
    if (!append_rv64_store_register_to_stack_offset(fragment,
                                                   destination_register,
                                                   *destination_stack_offset,
                                                   8)) {
      return std::nullopt;
    }
  }
  return fragment;
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes) {
  if (auto fragment = fragment_for_prepared_floating_cast(names, lookups, cast)) {
    return fragment;
  }
  if (auto fragment = fragment_for_prepared_fp_to_int_cast(names, lookups, cast)) {
    return fragment;
  }
  if (auto fragment = fragment_for_prepared_int_to_fp_cast(stack_layout,
                                                           names,
                                                           lookups,
                                                           cast,
                                                           stack_frame_bytes)) {
    return fragment;
  }
  if (auto fragment = fragment_for_prepared_pointer_cast(stack_layout,
                                                         names,
                                                         lookups,
                                                         cast,
                                                         stack_frame_bytes)) {
    return fragment;
  }

  const auto source_bits = rv64_integer_type_bits(cast.operand.type);
  const auto result_bits = rv64_integer_type_bits(cast.result.type);
  if (!source_bits.has_value() || !result_bits.has_value()) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type(cast.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *destination_home,
                                                     stack_frame_bytes,
                                                     *size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }

  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      *result_bits == *source_bits) {
    if (!rv64_fixed_integer_type(cast.operand.type) ||
        !rv64_fixed_integer_type(cast.result.type) ||
        !destination.has_value()) {
      return std::nullopt;
    }
    const auto* source_home = prepared_value_home_for(names, lookups, cast.operand);
    const auto source =
        source_home == nullptr ? std::nullopt : gpr_register_number_for_home(*source_home);
    if (!source.has_value()) {
      return std::nullopt;
    }
    RiscvEncodedFragment fragment;
    append_rv64_move(fragment, *destination, *source);
    return fragment;
  }

  const std::uint32_t destination_register = destination.value_or(30);
  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          destination_register,
                                          stack_layout,
                                          names,
                                          lookups,
                                          cast.operand,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  switch (cast.opcode) {
    case c4c::backend::bir::CastOpcode::ZExt:
      if (*result_bits <= *source_bits) {
        return std::nullopt;
      }
      append_rv64_zero_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *source_bits);
      break;
    case c4c::backend::bir::CastOpcode::SExt:
      if (*result_bits <= *source_bits) {
        return std::nullopt;
      }
      append_rv64_sign_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *source_bits);
      break;
    case c4c::backend::bir::CastOpcode::Trunc:
      if (*result_bits >= *source_bits) {
        return std::nullopt;
      }
      append_rv64_zero_extend_register(fragment,
                                       destination_register,
                                       destination_register,
                                       *result_bits);
      break;
    case c4c::backend::bir::CastOpcode::Bitcast:
      if (*result_bits != *source_bits) {
        return std::nullopt;
      }
      break;
    default:
      return std::nullopt;
  }
  if (destination_stack_offset.has_value() &&
      !append_rv64_store_register_to_stack_offset(fragment,
                                                 destination_register,
                                                 *destination_stack_offset,
                                                 *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}


[[nodiscard]] const c4c::backend::prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_current_instruction(
    const PreparedCurrentInstructionContext& context,
    c4c::backend::prepare::PreparedValueId value_id);

[[nodiscard]] std::optional<std::uint32_t> fresh_gpr_register_for_value(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value);

[[nodiscard]] bool append_fresh_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes);


std::optional<RiscvEncodedFragment> fragment_for_prepared_binary(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes,
    std::optional<std::size_t> block_index,
    std::optional<std::size_t> instruction_index) {
  const PreparedCurrentInstructionContext context{
      .names = names,
      .lookups = lookups,
      .block_index = block_index,
      .instruction_index = instruction_index.value_or(0),
  };
  auto direct_fresh_gpr_register_for_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::uint32_t> {
    const auto* home = prepared_value_home_for(names, lookups, value);
    if (home == nullptr) {
      return std::nullopt;
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value() && lookups != nullptr &&
        block_index.has_value() && instruction_index.has_value()) {
      const c4c::backend::prepare::PreparedCallPlan* selected = nullptr;
      for (const auto& entry : lookups->call_plans.calls_by_position) {
        const auto* call = entry.second;
        if (call == nullptr || call->block_index != *block_index ||
            call->instruction_index >= *instruction_index) {
          continue;
        }
        if (selected == nullptr ||
            selected->instruction_index < call->instruction_index) {
          selected = call;
        }
      }
      if (selected != nullptr) {
        for (const auto& clobber : selected->clobbered_registers) {
          if (clobber.bank == c4c::backend::prepare::PreparedRegisterBank::Gpr &&
              (clobber.register_name == *home->register_name ||
               std::find(clobber.occupied_register_names.begin(),
                         clobber.occupied_register_names.end(),
                         *home->register_name) !=
                   clobber.occupied_register_names.end())) {
            return std::nullopt;
          }
        }
      }
    }
    return gpr_register_number_for_home(*home);
  };
  if (auto fragment = fragment_for_prepared_fp_binary(names, lookups, binary)) {
    return fragment;
  }
  if ((binary.opcode == c4c::backend::bir::BinaryOpcode::Add ||
       binary.opcode == c4c::backend::bir::BinaryOpcode::Sub) &&
      binary.result.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto destination_home =
        prepared_value_home_for(names, lookups, binary.result);
    const auto destination =
        destination_home == nullptr
            ? std::nullopt
            : gpr_register_number_for_home(*destination_home);
    const auto destination_stack_offset =
        destination_home == nullptr
            ? std::nullopt
            : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                       *destination_home,
                                                       stack_frame_bytes,
                                                       8);
    if (!destination.has_value() && !destination_stack_offset.has_value()) {
      return std::nullopt;
    }

    const c4c::backend::bir::Value* base = nullptr;
    const c4c::backend::bir::Value* offset = nullptr;
    const bool is_sub = binary.opcode == c4c::backend::bir::BinaryOpcode::Sub;
    if (binary.lhs.type == c4c::backend::bir::TypeKind::Ptr &&
        binary.rhs.type != c4c::backend::bir::TypeKind::Ptr) {
      base = &binary.lhs;
      offset = &binary.rhs;
    } else if (!is_sub &&
               binary.rhs.type == c4c::backend::bir::TypeKind::Ptr &&
               binary.lhs.type != c4c::backend::bir::TypeKind::Ptr) {
      base = &binary.rhs;
      offset = &binary.lhs;
    } else {
      return std::nullopt;
    }

    RiscvEncodedFragment fragment;
    const std::uint32_t destination_register = destination.value_or(28);
    std::uint32_t base_register = 0;
    if (const auto direct_base_register = direct_fresh_gpr_register_for_value(*base);
        direct_base_register.has_value()) {
      base_register = *direct_base_register;
    } else {
      const auto* base_home = prepared_value_home_for(names, lookups, *base);
      const auto base_stack_offset =
          base_home == nullptr
              ? std::nullopt
              : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                         *base_home,
                                                         stack_frame_bytes,
                                                         8);
      if (!base_stack_offset.has_value() ||
          !append_rv64_load_stack_offset_to_register(fragment,
                                                     destination_register,
                                                     *base_stack_offset,
                                                     8)) {
        return std::nullopt;
      }
      base_register = destination_register;
    }
    const auto offset_immediate = integer_immediate_for_value(names, lookups, *offset);
    if (offset_immediate.has_value()) {
      if (is_sub && *offset_immediate == std::numeric_limits<std::int64_t>::min()) {
        return std::nullopt;
      }
      const std::int64_t adjusted_offset =
          is_sub ? -*offset_immediate : *offset_immediate;
      if (!fits_signed_12_bit_immediate(adjusted_offset)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                0,
                                base_register,
                                static_cast<std::int32_t>(adjusted_offset)));
    } else {
      const auto offset_register = direct_fresh_gpr_register_for_value(*offset);
      if (!offset_register.has_value()) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_r_type(0x33,
                                destination_register,
                                0,
                                base_register,
                                *offset_register,
                                is_sub ? 0x20 : 0));
    }
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   destination_register,
                                                   *destination_stack_offset,
                                                   8)) {
      return std::nullopt;
    }
    return fragment;
  }
  if (binary.result.type != c4c::backend::bir::TypeKind::I32 &&
      binary.result.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  const auto result_size_bytes =
      rv64_scalar_memory_size_for_type(binary.result.type);
  if (!result_size_bytes.has_value()) {
    return std::nullopt;
  }
  const auto destination_home = prepared_value_home_for(names, lookups, binary.result);
  const auto destination =
      destination_home == nullptr ? std::nullopt : gpr_register_number_for_home(*destination_home);
  const auto destination_stack_offset =
      destination_home == nullptr
          ? std::nullopt
          : prepared_stack_slot_home_absolute_offset(stack_layout,
                                                     *destination_home,
                                                     stack_frame_bytes,
                                                     *result_size_bytes);
  if (!destination.has_value() && !destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  const std::uint32_t destination_register = destination.value_or(30);

  const auto lhs_register = direct_fresh_gpr_register_for_value(binary.lhs);
  const auto rhs_register = direct_fresh_gpr_register_for_value(binary.rhs);
  const auto lhs_immediate = integer_immediate_for_value(names, lookups, binary.lhs);
  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);

  RiscvEncodedFragment fragment;
  auto finish = [&]() -> std::optional<RiscvEncodedFragment> {
    if (destination_stack_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset(fragment,
                                                   destination_register,
                                                   *destination_stack_offset,
                                                   *result_size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  };
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  0,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  0,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  0,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Sub:
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          *rhs_immediate != std::numeric_limits<std::int64_t>::min()) {
        const auto negated = -*rhs_immediate;
        if (fits_signed_12_bit_immediate(negated)) {
          append_le32(fragment.bytes,
                      encode_i_type(0x13,
                                    destination_register,
                                    0,
                                    *lhs_register,
                                    static_cast<std::int32_t>(negated)));
          return finish();
        }
      }
      break;
    case c4c::backend::bir::BinaryOpcode::And:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  7,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  7,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  7,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Xor:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  4,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return finish();
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  4,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return finish();
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  4,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return finish();
      }
      break;
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
      if (!append_fresh_move_value_to_register(fragment,
                                               28,
                                               stack_layout,
                                               context,
                                               binary.lhs,
                                               stack_frame_bytes) ||
          !append_fresh_move_value_to_register(fragment,
                                               29,
                                               stack_layout,
                                               context,
                                               binary.rhs,
                                               stack_frame_bytes)) {
        return std::nullopt;
      }
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 4, 28, 29, 0));
      if (binary.opcode == c4c::backend::bir::BinaryOpcode::Eq) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  destination_register,
                                  3,
                                  destination_register,
                                  1));
      } else {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  destination_register,
                                  3,
                                  0,
                                  destination_register,
                                  0));
      }
      return finish();
    default:
      break;
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr &&
      rhs_immediate.has_value()) {
    const std::int64_t shift_width =
        binary.result.type == c4c::backend::bir::TypeKind::I32 ? 32 : 64;
    if (*rhs_immediate < 0 || *rhs_immediate >= shift_width) {
      return std::nullopt;
    }
    if (!append_fresh_move_value_to_register(fragment,
                                             28,
                                             stack_layout,
                                             context,
                                             binary.lhs,
                                             stack_frame_bytes)) {
      return std::nullopt;
    }
    append_le32(fragment.bytes,
                encode_i_type(binary.result.type == c4c::backend::bir::TypeKind::I32
                                  ? 0x1b
                                  : 0x13,
                              destination_register,
                              5,
                              28,
                              static_cast<std::int32_t>(0x400 | *rhs_immediate)));
    return finish();
  }
  if (!append_fresh_move_value_to_register(fragment,
                                           28,
                                           stack_layout,
                                           context,
                                           binary.lhs,
                                           stack_frame_bytes) ||
      !append_fresh_move_value_to_register(fragment,
                                           29,
                                           stack_layout,
                                           context,
                                           binary.rhs,
                                           stack_frame_bytes)) {
    return std::nullopt;
  }
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sub:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 0x20));
      return finish();
    case c4c::backend::bir::BinaryOpcode::And:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 7, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Or:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 6, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Xor:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 4, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Shl:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 1, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::LShr:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 5, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::AShr:
      append_le32(fragment.bytes,
                  encode_r_type(binary.result.type == c4c::backend::bir::TypeKind::I32
                                    ? 0x3b
                                    : 0x33,
                                destination_register,
                                5,
                                28,
                                29,
                                0x20));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Mul:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 0, 28, 29, 1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::SDiv: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 4, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::UDiv: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 5, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::SRem: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 6, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::URem: {
      const std::uint32_t opcode =
          binary.result.type == c4c::backend::bir::TypeKind::I32 ? 0x3b : 0x33;
      append_le32(fragment.bytes,
                  encode_r_type(opcode, destination_register, 7, 28, 29, 1));
      return finish();
    }
    case c4c::backend::bir::BinaryOpcode::Slt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sgt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 29, 28, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sle:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 29, 28, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Sge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 2, 28, 29, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ult:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 28, 29, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ugt:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 29, 28, 0));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Ule:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 29, 28, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    case c4c::backend::bir::BinaryOpcode::Uge:
      append_le32(fragment.bytes,
                  encode_r_type(0x33, destination_register, 3, 28, 29, 0));
      append_le32(fragment.bytes,
                  encode_i_type(0x13,
                                destination_register,
                                4,
                                destination_register,
                                1));
      return finish();
    default:
      return std::nullopt;
  }
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_compare_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string true_label,
    std::string false_label,
    std::size_t stack_frame_bytes) {
  const auto normalized = normalize_rv64_branch_predicate(opcode, lhs, rhs);
  if (!normalized.has_value()) {
    return std::nullopt;
  }
  const auto funct3 = rv64_branch_funct3(normalized->opcode);
  if (!funct3.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  if (!append_rv64_move_value_to_register(fragment,
                                          28,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->lhs,
                                          stack_frame_bytes) ||
      !append_rv64_move_value_to_register(fragment,
                                          29,
                                          stack_layout,
                                          names,
                                          lookups,
                                          normalized->rhs,
                                          stack_frame_bytes)) {
    return std::nullopt;
  }
  append_rv64_local_branch(fragment, *funct3, 28, 29, std::move(true_label));
  append_rv64_local_jump(fragment, std::move(false_label));
  return fragment;
}

bool prepared_compare_feeds_supported_scalar_trunc_publication(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;

  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);
  if (binary.opcode != bir::BinaryOpcode::Sge ||
      binary.result.kind != bir::Value::Kind::Named ||
      binary.result.type != bir::TypeKind::I32 ||
      binary.operand_type != bir::TypeKind::I32 ||
      binary.lhs.type != bir::TypeKind::I32 ||
      binary.rhs.type != bir::TypeKind::I32 ||
      !rhs_immediate.has_value() ||
      !fits_signed_12_bit_immediate(*rhs_immediate)) {
    return false;
  }
  const auto* compare_home = prepared_value_home_for(names, lookups, binary.result);
  if (compare_home == nullptr ||
      !gpr_register_number_for_home(*compare_home).has_value()) {
    return false;
  }
  const PreparedCurrentInstructionContext context{
      .names = names,
      .lookups = lookups,
      .block_index = block_index,
      .instruction_index = instruction_index,
  };
  const auto* lhs_home = prepared_value_home_for(names, lookups, binary.lhs);
  if (lhs_home == nullptr ||
      (!fresh_gpr_register_for_value(context, binary.lhs).has_value() &&
       find_prior_preserved_value_for_current_instruction(context, lhs_home->value_id) ==
           nullptr)) {
    return false;
  }

  const bir::CastInst* selected_trunc = nullptr;
  for (std::size_t index = instruction_index + 1; index < block.insts.size(); ++index) {
    const auto* cast = std::get_if<bir::CastInst>(&block.insts[index]);
    if (cast == nullptr || cast->opcode != bir::CastOpcode::Trunc ||
        cast->operand.kind != bir::Value::Kind::Named ||
        cast->operand.name != binary.result.name) {
      continue;
    }
    if (selected_trunc != nullptr) {
      return false;
    }
    selected_trunc = cast;
  }
  if (selected_trunc == nullptr ||
      selected_trunc->operand.type != bir::TypeKind::I32 ||
      selected_trunc->result.type != bir::TypeKind::I16) {
    return false;
  }
  const auto* trunc_home = prepared_value_home_for(names, lookups, selected_trunc->result);
  if (trunc_home == nullptr ||
      gpr_register_number_for_home(*trunc_home).has_value()) {
    return trunc_home != nullptr;
  }
  return prepared_stack_slot_home_offset(stack_layout,
                                         *trunc_home,
                                         stack_frame_bytes,
                                         2)
      .has_value();
}


std::optional<RiscvEncodedFragment> fragment_for_prepared_scalar_compare_trunc_source(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes) {
  if (!prepared_compare_feeds_supported_scalar_trunc_publication(stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 block,
                                                                 block_index,
                                                                 instruction_index,
                                                                 binary,
                                                                 stack_frame_bytes)) {
    return std::nullopt;
  }
  const auto destination =
      gpr_register_number_for_value(names, lookups, binary.result);
  const auto rhs = integer_immediate_for_value(names, lookups, binary.rhs);
  if (!destination.has_value() || !rhs.has_value() ||
      !fits_signed_12_bit_immediate(*rhs)) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  const PreparedCurrentInstructionContext context{
      .names = names,
      .lookups = lookups,
      .block_index = block_index,
      .instruction_index = instruction_index,
  };
  std::uint32_t lhs_register = 28;
  if (const auto fresh_lhs = fresh_gpr_register_for_value(context, binary.lhs)) {
    lhs_register = *fresh_lhs;
  } else {
    if (!append_fresh_move_value_to_register(fragment,
                                             lhs_register,
                                             stack_layout,
                                             context,
                                             binary.lhs,
                                             stack_frame_bytes)) {
      return std::nullopt;
    }
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                             *destination,
                             2,
                             lhs_register,
                             static_cast<std::int32_t>(*rhs)));
  append_le32(fragment.bytes,
              encode_i_type(0x13, *destination, 4, *destination, 1));
  return fragment;
}



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

[[nodiscard]] std::optional<std::string> load_mnemonic_for_scalar_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I16:
      return std::string{"lh"};
    case c4c::backend::bir::TypeKind::I32:
      return std::string{"lw"};
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return std::string{"ld"};
    default:
      return std::nullopt;
  }
}

[[nodiscard]] const c4c::backend::prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_current_instruction(
    const PreparedCurrentInstructionContext& context,
    c4c::backend::prepare::PreparedValueId value_id) {
  if (context.lookups == nullptr || !context.block_index.has_value()) {
    return nullptr;
  }
  const c4c::backend::prepare::PreparedCallPlan current_position{
      .block_index = *context.block_index,
      .instruction_index = context.instruction_index,
  };
  const auto result =
      c4c::backend::prepare::find_unique_indexed_prior_preserved_value_source(
          context.lookups->call_plans,
          nullptr,
          current_position,
          value_id);
  return result.status ==
                 c4c::backend::prepare::PreparedPriorPreservedValueLookupStatus::Found
             ? result.preserved
             : nullptr;
}

[[nodiscard]] const c4c::backend::prepare::PreparedCallPlan*
latest_same_block_call_before_current_instruction(
    const PreparedCurrentInstructionContext& context) {
  if (context.lookups == nullptr || !context.block_index.has_value()) {
    return nullptr;
  }
  const c4c::backend::prepare::PreparedCallPlan* selected = nullptr;
  for (const auto& entry : context.lookups->call_plans.calls_by_position) {
    const auto* call = entry.second;
    if (call == nullptr || call->block_index != *context.block_index ||
        call->instruction_index >= context.instruction_index) {
      continue;
    }
    if (selected == nullptr ||
        selected->instruction_index < call->instruction_index) {
      selected = call;
    }
  }
  return selected;
}

[[nodiscard]] bool call_clobbers_gpr_register(
    const c4c::backend::prepare::PreparedCallPlan& call,
    std::string_view register_name) {
  for (const auto& clobber : call.clobbered_registers) {
    if (clobber.bank != c4c::backend::prepare::PreparedRegisterBank::Gpr) {
      continue;
    }
    if (clobber.register_name == register_name ||
        std::find(clobber.occupied_register_names.begin(),
                  clobber.occupied_register_names.end(),
                  register_name) != clobber.occupied_register_names.end()) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool direct_register_home_is_stale_after_call(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return false;
  }
  const auto* call = latest_same_block_call_before_current_instruction(context);
  return call != nullptr && call_clobbers_gpr_register(*call, *home.register_name);
}

[[nodiscard]] std::optional<std::uint32_t> fresh_gpr_register_for_value(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(context.names, context.lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    if (find_prior_preserved_value_for_current_instruction(context, home->value_id) !=
        nullptr) {
      return std::nullopt;
    }
    if (direct_register_home_is_stale_after_call(context, *home)) {
      return std::nullopt;
    }
  }
  return gpr_register_number_for_home(*home);
}

[[nodiscard]] bool append_fresh_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto immediate = integer_immediate_for_value(context.names, context.lookups, value);
  if (immediate.has_value()) {
    append_rv64_load_immediate(fragment, destination, *immediate);
    return true;
  }
  const auto* home = prepared_value_home_for(context.names, context.lookups, value);
  if (home == nullptr) {
    return false;
  }
  if (const auto* preserved =
          find_prior_preserved_value_for_current_instruction(context, home->value_id);
      preserved != nullptr) {
    return append_prior_preserved_value_to_register(fragment,
                                                   destination,
                                                   *preserved,
                                                   value.type);
  }
  if (direct_register_home_is_stale_after_call(context, *home)) {
    return false;
  }
  return append_rv64_move_value_to_register(fragment,
                                           destination,
                                           stack_layout,
                                           context.names,
                                           context.lookups,
                                           value,
                                           stack_frame_bytes);
}

bool emit_prior_preserved_value_to_register(
    std::string& out,
    std::string_view destination_register,
    const c4c::backend::prepare::PreparedCallPreservedValue& preserved,
    c4c::backend::bir::TypeKind type) {
  namespace prepare = c4c::backend::prepare;

  if (preserved.route == prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    if (preserved.register_bank !=
            std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
        !preserved.register_name.has_value() || preserved.register_name->empty() ||
        preserved.contiguous_width != 1 || preserved.occupied_register_names.empty() ||
        !preserved.register_placement.has_value()) {
      return false;
    }
    if (*preserved.register_name != destination_register) {
      out += "    mv ";
      out += destination_register;
      out += ", ";
      out += *preserved.register_name;
      out += "\n";
    }
    return true;
  }

  if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
      !preserved.stack_offset_bytes.has_value() ||
      !preserved.stack_size_bytes.has_value()) {
    return false;
  }
  const auto mnemonic = load_mnemonic_for_scalar_type(type);
  if (!mnemonic.has_value() ||
      !fits_signed_12_bit_load_offset(*preserved.stack_offset_bytes)) {
    return false;
  }
  out += "    ";
  out += *mnemonic;
  out += " ";
  out += destination_register;
  out += ", ";
  out += std::to_string(*preserved.stack_offset_bytes);
  out += "(sp)\n";
  return true;
}

bool emit_move_to_register(std::string& out,
                           std::string_view destination_register,
                           const c4c::backend::prepare::PreparedNameTables& names,
                           const c4c::backend::prepare::PreparedFunctionLookups* lookups,
                           const c4c::backend::bir::Value& value) {
  return emit_move_to_register(out,
                               destination_register,
                               PreparedCurrentInstructionContext{
                                   .names = names,
                                   .lookups = lookups,
                               },
                               value);
}

bool emit_move_to_register(std::string& out,
                           std::string_view destination_register,
                           const PreparedCurrentInstructionContext& context,
                           const c4c::backend::bir::Value& value) {
  const auto& names = context.names;
  const auto* lookups = context.lookups;
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
    if (const auto* home = prepared_value_home_for(names, lookups, value);
        home != nullptr) {
      if (const auto* preserved =
              find_prior_preserved_value_for_current_instruction(context,
                                                                 home->value_id);
          preserved != nullptr) {
        return emit_prior_preserved_value_to_register(out,
                                                      destination_register,
                                                      *preserved,
                                                      value.type);
      }
      if (direct_register_home_is_stale_after_call(context, *home)) {
        return false;
      }
    }
    if (*source_register != destination_register) {
      out += "    mv ";
      out += destination_register;
      out += ", ";
      out += *source_register;
      out += "\n";
    }
    return true;
  }
  if (value.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto pointer_register = prepared_pointer_register_for_value(names, lookups, value);
    if (pointer_register.has_value()) {
      if (*pointer_register != destination_register) {
        out += "    mv ";
        out += destination_register;
        out += ", ";
        out += *pointer_register;
        out += "\n";
      }
      return true;
    }
  }

  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home != nullptr &&
      (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
       home->kind == c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset) &&
      home->offset_bytes.has_value()) {
    if (value.type == c4c::backend::bir::TypeKind::Ptr &&
        home->size_bytes == std::optional<std::size_t>{8} &&
        fits_signed_12_bit_load_offset(*home->offset_bytes)) {
      out += "    ld ";
      out += destination_register;
      out += ", ";
      out += std::to_string(*home->offset_bytes);
      out += "(sp)\n";
      return true;
    }
    if (value.type == c4c::backend::bir::TypeKind::I16 &&
        home->size_bytes == std::optional<std::size_t>{2} &&
        fits_signed_12_bit_load_offset(*home->offset_bytes)) {
      out += "    lh ";
      out += destination_register;
      out += ", ";
      out += std::to_string(*home->offset_bytes);
      out += "(sp)\n";
      return true;
    }
    if (value.type == c4c::backend::bir::TypeKind::I32 &&
        home->size_bytes == std::optional<std::size_t>{4}) {
      const auto loaded =
          emit_i32_load_from_stack_offset(destination_register,
                                          static_cast<std::int64_t>(*home->offset_bytes));
      if (!loaded.has_value()) {
        return false;
      }
      out += *loaded;
      return true;
    }
  }

  return false;
}

std::optional<std::string> emit_riscv_simple_cast(
    const c4c::backend::bir::CastInst& cast,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if ((cast.opcode != c4c::backend::bir::CastOpcode::SExt &&
       cast.opcode != c4c::backend::bir::CastOpcode::ZExt &&
       cast.opcode != c4c::backend::bir::CastOpcode::Trunc &&
       cast.opcode != c4c::backend::bir::CastOpcode::PtrToInt &&
       cast.opcode != c4c::backend::bir::CastOpcode::IntToPtr) ||
      cast.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  if ((cast.opcode == c4c::backend::bir::CastOpcode::PtrToInt ||
       cast.opcode == c4c::backend::bir::CastOpcode::IntToPtr) &&
      !rv64_prepared_pointer_cast_types_supported(cast.opcode,
                                                  cast.operand.type,
                                                  cast.result.type)) {
    return std::nullopt;
  }
  const auto destination_register =
      cast.result.type == c4c::backend::bir::TypeKind::Ptr
          ? prepared_pointer_register_for_value(names, lookups, cast.result)
          : prepared_register_for_value(names, lookups, cast.result);
  const auto* destination_home = prepared_value_home_for(names, lookups, cast.result);
  const std::string materialization_register =
      destination_register.has_value() ? *destination_register : "t3";

  std::string out;
  if (!emit_move_to_register(out, materialization_register, names, lookups, cast.operand)) {
    return std::nullopt;
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::SExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I8 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32) {
    out += "    slli " + materialization_register + ", " + materialization_register + ", 56\n";
    out += "    srai " + materialization_register + ", " + materialization_register + ", 56\n";
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::SExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I16 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32) {
    out += "    slli " + materialization_register + ", " + materialization_register + ", 48\n";
    out += "    srai " + materialization_register + ", " + materialization_register + ", 48\n";
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I8 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32) {
    out += "    andi " + materialization_register + ", " + materialization_register + ", 255\n";
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I16 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32) {
    out += "    slli " + materialization_register + ", " + materialization_register + ", 48\n";
    out += "    srli " + materialization_register + ", " + materialization_register + ", 48\n";
  }
  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I32 &&
      cast.result.type == c4c::backend::bir::TypeKind::I64) {
    out += "    slli " + materialization_register + ", " + materialization_register + ", 32\n";
    out += "    srli " + materialization_register + ", " + materialization_register + ", 32\n";
  }
  if (destination_register.has_value()) {
    return out;
  }
  if (destination_home == nullptr ||
      destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !destination_home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  if (cast.result.type == c4c::backend::bir::TypeKind::Ptr ||
      cast.result.type == c4c::backend::bir::TypeKind::I64) {
    if (destination_home->size_bytes != std::optional<std::size_t>{8} ||
        !fits_signed_12_bit_immediate(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    out += "    sd " + materialization_register + ", " +
           std::to_string(*destination_home->offset_bytes) + "(sp)\n";
    return out;
  }
  if (cast.result.type == c4c::backend::bir::TypeKind::I16) {
    if (destination_home->size_bytes != std::optional<std::size_t>{2} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    out += "    sh " + materialization_register + ", " +
           std::to_string(*destination_home->offset_bytes) + "(sp)\n";
    return out;
  }
  if (destination_home->size_bytes != std::optional<std::size_t>{4}) {
    return std::nullopt;
  }
  const auto stored = emit_i32_store_to_stack_offset(
      materialization_register,
      static_cast<std::int64_t>(*destination_home->offset_bytes));
  if (!stored.has_value()) {
    return std::nullopt;
  }
  out += *stored;
  return out;
}

std::optional<std::string> emit_riscv_simple_select(
    const c4c::backend::bir::SelectInst& select,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Block* block) {
  if (select.result.kind != c4c::backend::bir::Value::Kind::Named ||
      (select.result.type != c4c::backend::bir::TypeKind::I32 &&
       select.result.type != c4c::backend::bir::TypeKind::I16 &&
       select.result.type != c4c::backend::bir::TypeKind::Ptr)) {
    return std::nullopt;
  }
  const auto* destination_home = prepared_value_home_for(context, select.result);
  if (destination_home == nullptr) {
    return std::string{};
  }
  std::size_t label_serial = 0;
  if (select.result.type == c4c::backend::bir::TypeKind::Ptr) {
    return emit_select_to_pointer_location(
        select,
        prepared,
        function_name,
        context,
        *destination_home,
        block,
        0,
        label_serial);
  }
  if (select.result.type == c4c::backend::bir::TypeKind::I16) {
    return emit_select_to_i16_location(
        select,
        function_name,
        context,
        *destination_home,
        block,
        0,
        label_serial);
  }
  return emit_select_to_i32_location(
      select,
      function_name,
      context,
      *destination_home,
      block,
      0,
      label_serial);
}

std::optional<std::string> emit_riscv_simple_prepared_pointer_add(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedCurrentInstructionContext& context) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if ((binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub) ||
      binary.result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary.result.type != c4c::backend::bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for(context, binary.result);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (has_frame_slot_address_materialization_at(context)) {
    if ((home->kind == prepare::PreparedValueHomeKind::Register ||
         home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset) &&
        home->register_name.has_value() &&
        !home->register_name->empty()) {
      const auto* materializations =
          prepare::find_indexed_prepared_address_materializations(
              &context.lookups->address_materializations,
              context.block_label);
      if (materializations == nullptr) {
        return std::nullopt;
      }
      const prepare::PreparedAddressMaterialization* selected = nullptr;
      const auto result_name = context.names.value_names.find(binary.result.name);
      for (const auto* materialization : *materializations) {
        if (materialization == nullptr ||
            materialization->inst_index != context.instruction_index ||
            materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
            materialization->result_value_name !=
                std::optional<c4c::ValueNameId>{result_name} ||
            !materialization->frame_slot_id.has_value() ||
            !fits_signed_12_bit_immediate(materialization->byte_offset)) {
          continue;
        }
        if (selected != nullptr) {
          return std::nullopt;
        }
        selected = materialization;
      }
      if (selected == nullptr) {
        return std::nullopt;
      }
      return "    addi " + *home->register_name + ", sp, " +
             std::to_string(selected->byte_offset) + "\n";
    }
    if ((home->kind == prepare::PreparedValueHomeKind::StackSlot ||
         home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset) &&
        home->offset_bytes.has_value() &&
        home->size_bytes == std::optional<std::size_t>{8} &&
        fits_signed_12_bit_load_offset(*home->offset_bytes)) {
      const auto* materializations =
          prepare::find_indexed_prepared_address_materializations(
              &context.lookups->address_materializations,
              context.block_label);
      if (materializations == nullptr) {
        return std::nullopt;
      }
      const prepare::PreparedAddressMaterialization* selected = nullptr;
      const auto result_name = context.names.value_names.find(binary.result.name);
      for (const auto* materialization : *materializations) {
        if (materialization == nullptr ||
            materialization->inst_index != context.instruction_index ||
            materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
            materialization->result_value_name !=
                std::optional<c4c::ValueNameId>{result_name} ||
            !materialization->frame_slot_id.has_value() ||
            !fits_signed_12_bit_immediate(materialization->byte_offset)) {
          continue;
        }
        if (selected != nullptr) {
          return std::nullopt;
        }
        selected = materialization;
      }
      if (selected == nullptr) {
        return std::string{};
      }
      return "    addi t3, sp, " + std::to_string(selected->byte_offset) + "\n"
             "    sd t3, " + std::to_string(*home->offset_bytes) + "(sp)\n";
    }
    return std::nullopt;
  }
  if (home->kind != prepare::PreparedValueHomeKind::Register &&
      home->kind != prepare::PreparedValueHomeKind::StackSlot &&
      home->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset) {
    return std::nullopt;
  }

  const bir::Value* base = nullptr;
  const bir::Value* offset = nullptr;
  const bool is_sub = binary.opcode == bir::BinaryOpcode::Sub;
  if (binary.lhs.type == bir::TypeKind::Ptr && binary.rhs.type != bir::TypeKind::Ptr) {
    base = &binary.lhs;
    offset = &binary.rhs;
  } else if (!is_sub &&
             binary.rhs.type == bir::TypeKind::Ptr &&
             binary.lhs.type != bir::TypeKind::Ptr) {
    base = &binary.rhs;
    offset = &binary.lhs;
  } else {
    return std::nullopt;
  }

  std::string out;
  std::string base_register_name;
  if (const auto base_register = prepared_pointer_register_for_value(context, *base);
      base_register.has_value()) {
    base_register_name = *base_register;
  } else {
    if (!emit_move_to_register(out, "t3", context, *base)) {
      return std::nullopt;
    }
    base_register_name = "t3";
  }
  const auto offset_immediate =
      simple_or_prepared_integer_immediate(context.names, context.lookups, *offset);
  if (offset_immediate.has_value()) {
    if (is_sub && *offset_immediate == std::numeric_limits<std::int64_t>::min()) {
      return std::nullopt;
    }
    const std::int64_t adjusted_offset = is_sub ? -*offset_immediate : *offset_immediate;
    if (!fits_signed_12_bit_immediate(adjusted_offset)) {
      return std::nullopt;
    }
    out += "    addi t3, " + base_register_name + ", " +
           std::to_string(adjusted_offset) + "\n";
  } else {
    const auto offset_register = prepared_register_for_value(context, *offset);
    if (!offset_register.has_value()) {
      return std::nullopt;
    }
    out += "    ";
    out += is_sub ? "sub" : "add";
    out += " t3, " + base_register_name + ", " + *offset_register + "\n";
  }
  if (home->register_name.has_value() && !home->register_name->empty()) {
    if (*home->register_name != "t3") {
      out += "    mv " + *home->register_name + ", t3\n";
    }
    return out;
  }
  if (!home->offset_bytes.has_value() ||
      home->size_bytes != std::optional<std::size_t>{8} ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(*home->offset_bytes))) {
    return std::nullopt;
  }
  out += "    sd t3, " + std::to_string(*home->offset_bytes) + "(sp)\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_binary(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedCurrentInstructionContext& context) {
  const auto& names = context.names;
  const auto* lookups = context.lookups;
  if ((binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Mul &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::SDiv &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::SRem &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::And &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Or &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Xor &&
       !c4c::backend::bir::is_compare_opcode(binary.opcode)) ||
      binary.result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(names, lookups, binary.result);
  if (!destination_register.has_value()) {
    const auto* destination_home = prepared_value_home_for(names, lookups, binary.result);
    if (binary.result.type == c4c::backend::bir::TypeKind::I32 &&
        destination_home != nullptr &&
        destination_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        destination_home->offset_bytes.has_value() &&
        destination_home->size_bytes == std::optional<std::size_t>{4}) {
      std::string out;
      if (!emit_move_to_register(out, "t3", context, binary.lhs) ||
          !emit_move_to_register(out, "t4", context, binary.rhs)) {
        return std::nullopt;
      }
      switch (binary.opcode) {
        case c4c::backend::bir::BinaryOpcode::Add:
          out += "    add t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::Sub:
          out += "    sub t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::Mul:
          out += "    mul t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::SDiv:
          out += "    divw t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::SRem:
          out += "    remw t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::And:
          out += "    and t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::Or:
          out += "    or t3, t3, t4\n";
          break;
        case c4c::backend::bir::BinaryOpcode::Xor:
          out += "    xor t3, t3, t4\n";
          break;
        default:
          return std::nullopt;
      }
      const auto stored = emit_i32_store_to_stack_offset(
          "t3",
          static_cast<std::int64_t>(*destination_home->offset_bytes));
      if (!stored.has_value()) {
        return std::nullopt;
      }
      out += *stored;
      return out;
    }
    const auto result_immediate = prepared_immediate_i32_for_value(names, lookups, binary.result);
    return result_immediate.has_value() ? std::optional<std::string>{std::string{}} : std::nullopt;
  }
  if ((binary.opcode == c4c::backend::bir::BinaryOpcode::SDiv ||
       binary.opcode == c4c::backend::bir::BinaryOpcode::SRem) &&
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (c4c::backend::bir::is_compare_opcode(binary.opcode)) {
    return emit_riscv_simple_compare_value(
        binary,
        *destination_register,
        context);
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
      case c4c::backend::bir::BinaryOpcode::SDiv:
        if (*rhs_imm == 0) {
          return std::nullopt;
        }
        result = *lhs_imm / *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::SRem:
        if (*rhs_imm == 0) {
          return std::nullopt;
        }
        result = *lhs_imm % *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::And:
        result = *lhs_imm & *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::Or:
        result = *lhs_imm | *rhs_imm;
        break;
      case c4c::backend::bir::BinaryOpcode::Xor:
        result = *lhs_imm ^ *rhs_imm;
        break;
      default:
        return std::nullopt;
    }
    out += "    li " + *destination_register + ", " +
           std::to_string(result) + "\n";
    return out;
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::SDiv ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::SRem) {
    std::string lhs_register_name;
    if (lhs_register.has_value()) {
      lhs_register_name = *lhs_register;
    } else if (lhs_imm.has_value()) {
      lhs_register_name = "t3";
      out += "    li " + lhs_register_name + ", " + std::to_string(*lhs_imm) + "\n";
    } else if (emit_move_to_register(out, "t3", context, binary.lhs)) {
      lhs_register_name = "t3";
    } else {
      return std::nullopt;
    }

    std::string rhs_register_name;
    if (rhs_register.has_value()) {
      rhs_register_name = *rhs_register;
    } else if (rhs_imm.has_value()) {
      rhs_register_name = "t4";
      out += "    li " + rhs_register_name + ", " + std::to_string(*rhs_imm) + "\n";
    } else if (emit_move_to_register(out, "t4", context, binary.rhs)) {
      rhs_register_name = "t4";
    } else {
      return std::nullopt;
    }

    const char* opcode =
        binary.opcode == c4c::backend::bir::BinaryOpcode::SDiv ? "divw" : "remw";
    out += std::string{"    "} + opcode + " " + *destination_register + ", " +
           lhs_register_name + ", " + rhs_register_name + "\n";
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
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Or ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    const auto register_immediate_opcode =
        binary.opcode == c4c::backend::bir::BinaryOpcode::And ? "andi" :
        binary.opcode == c4c::backend::bir::BinaryOpcode::Or ? "ori" : "xori";
    const auto register_register_opcode =
        binary.opcode == c4c::backend::bir::BinaryOpcode::And ? "and" :
        binary.opcode == c4c::backend::bir::BinaryOpcode::Or ? "or" : "xor";
    if (lhs_register.has_value() && rhs_imm.has_value() &&
        fits_signed_12_bit_immediate(*rhs_imm)) {
      out += std::string{"    "} + register_immediate_opcode + " " +
             *destination_register + ", " + *lhs_register + ", " +
             std::to_string(*rhs_imm) + "\n";
      return out;
    }
    if (rhs_register.has_value() && lhs_imm.has_value() &&
        fits_signed_12_bit_immediate(*lhs_imm)) {
      out += std::string{"    "} + register_immediate_opcode + " " +
             *destination_register + ", " + *rhs_register + ", " +
             std::to_string(*lhs_imm) + "\n";
      return out;
    }
    if (lhs_register.has_value() && rhs_register.has_value()) {
      out += std::string{"    "} + register_register_opcode + " " +
             *destination_register + ", " + *lhs_register + ", " +
             *rhs_register + "\n";
      return out;
    }
    return std::nullopt;
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
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Sub ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Mul ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::SDiv ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::SRem ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::And ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Or ||
      binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    if (!emit_move_to_register(out, "t3", context, binary.lhs) ||
        !emit_move_to_register(out, "t4", context, binary.rhs)) {
      return std::nullopt;
    }
    const char* opcode = nullptr;
    switch (binary.opcode) {
      case c4c::backend::bir::BinaryOpcode::Add:
        opcode = "add";
        break;
      case c4c::backend::bir::BinaryOpcode::Sub:
        opcode = "sub";
        break;
      case c4c::backend::bir::BinaryOpcode::Mul:
        opcode = "mul";
        break;
      case c4c::backend::bir::BinaryOpcode::SDiv:
        opcode = "divw";
        break;
      case c4c::backend::bir::BinaryOpcode::SRem:
        opcode = "remw";
        break;
      case c4c::backend::bir::BinaryOpcode::And:
        opcode = "and";
        break;
      case c4c::backend::bir::BinaryOpcode::Or:
        opcode = "or";
        break;
      case c4c::backend::bir::BinaryOpcode::Xor:
        opcode = "xor";
        break;
      default:
        return std::nullopt;
    }
    out += std::string{"    "} + opcode + " " + *destination_register +
           ", t3, t4\n";
    return out;
  }
  return std::nullopt;
}

std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedBirModule& prepared,
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
  if (terminator.value->type == c4c::backend::bir::TypeKind::Ptr &&
      terminator.value->kind == c4c::backend::bir::Value::Kind::Immediate &&
      terminator.value->immediate == 0) {
    std::string out = "    li a0, 0\n";
    append_epilogue(out);
    return out;
  }
  const auto immediate = simple_integer_immediate(*terminator.value);
  if (!immediate.has_value()) {
    std::string out;
    if (terminator.value->type == c4c::backend::bir::TypeKind::Ptr) {
      const auto function_address = emit_riscv_direct_function_address_materialization(
          prepared,
          *terminator.value,
          "a0");
      if (function_address.has_value()) {
        out += *function_address;
      } else if (!emit_move_to_register(out, "a0", names, lookups, *terminator.value)) {
        return std::nullopt;
      }
    } else if (!emit_move_to_register(out, "a0", names, lookups, *terminator.value)) {
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
