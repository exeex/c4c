#include "prepared_scalar_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_global_memory_emit.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"

#include <algorithm>
#include <limits>

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
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if (!c4c::backend::bir::is_compare_opcode(binary.opcode)) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t3", names, lookups, binary.lhs) ||
      !emit_move_to_register(out, "t4", names, lookups, binary.rhs)) {
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
  if (!emit_move_to_register(out, "t3", context.names, context.lookups, select.lhs) ||
      !emit_move_to_register(out, "t4", context.names, context.lookups, select.rhs)) {
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
  if (!emit_move_to_register(out, "t3", context.names, context.lookups, select.lhs) ||
      !emit_move_to_register(out, "t4", context.names, context.lookups, select.rhs)) {
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
  if (emit_move_to_register(
          out,
          destination_register,
          context.names,
          context.lookups,
          value)) {
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
  if (!emit_move_to_register(out, "t3", context.names, context.lookups, select.lhs) ||
      !emit_move_to_register(out, "t4", context.names, context.lookups, select.rhs)) {
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
    if (emit_move_to_register(
        out,
        *destination_home.register_name,
        context.names,
        context.lookups,
        value)) {
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
    if (!materialized && !emit_move_to_register(out, "t3", context.names, context.lookups, value)) {
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
    if (emit_move_to_register(
        out,
        *destination_home.register_name,
        context.names,
        context.lookups,
        value)) {
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
    if (!emit_move_to_register(out, "t3", context.names, context.lookups, value)) {
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
  if (cast.result.type == c4c::backend::bir::TypeKind::Ptr) {
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
  if (home->kind != prepare::PreparedValueHomeKind::StackSlot &&
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

  const auto base_register = prepared_pointer_register_for_value(context, *base);
  if (!base_register.has_value()) {
    return std::nullopt;
  }

  std::string out;
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
    out += "    addi t3, " + *base_register + ", " +
           std::to_string(adjusted_offset) + "\n";
  } else {
    const auto offset_register = prepared_register_for_value(context, *offset);
    if (!offset_register.has_value()) {
      return std::nullopt;
    }
    out += "    ";
    out += is_sub ? "sub" : "add";
    out += " t3, " + *base_register + ", " + *offset_register + "\n";
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
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
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
      if (!emit_move_to_register(out, "t3", names, lookups, binary.lhs) ||
          !emit_move_to_register(out, "t4", names, lookups, binary.rhs)) {
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
        names,
        lookups);
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
    } else if (emit_move_to_register(out, "t3", names, lookups, binary.lhs)) {
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
    } else if (emit_move_to_register(out, "t4", names, lookups, binary.rhs)) {
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
    if (!emit_move_to_register(out, "t3", names, lookups, binary.lhs) ||
        !emit_move_to_register(out, "t4", names, lookups, binary.rhs)) {
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
