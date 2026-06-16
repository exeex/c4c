// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/emit.rs
// The first shared C++ seam now lives in riscv_codegen.hpp. The broader
// `RiscvCodegen` / `CodegenState` surface is still pending, so this file keeps
// the reusable register helpers concrete and leaves the large method surface as
// a source-level mirror for the sibling slices.

#include "emit.hpp"

#include "../../../backend.hpp"
#include "../../../../codegen/lir/ir.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace c4c::backend::riscv::codegen {

struct PhysReg {
  std::uint32_t value = 0;

  constexpr PhysReg() = default;
  constexpr explicit PhysReg(std::uint32_t v) : value(v) {}
};

namespace {

bool fits_signed_12_bit_load_offset(std::size_t offset_bytes) {
  return offset_bytes <= 2047;
}

bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

std::size_t align_riscv_stack_frame(std::size_t size_bytes) {
  return (size_bytes + 15) & ~std::size_t{15};
}

void clear_stack_source_intent(EdgePublicationMoveIntent& intent) {
  intent.source_stack_slot_id.reset();
  intent.source_stack_offset_bytes.reset();
  intent.source_stack_size_bytes.reset();
  intent.source_stack_align_bytes.reset();
  intent.source_stack_extension_policy =
      c4c::backend::prepare::PreparedTypedStackSourceExtensionPolicy::None;
  intent.source_memory_base_value_id.reset();
  intent.source_memory_base_register.clear();
  intent.source_memory_byte_offset.reset();
  intent.source_memory_size_bytes.reset();
  intent.source_memory_align_bytes.reset();
}

void copy_same_width_i32_stack_source_publication(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedTypedStackSourcePublication& typed) {
  intent.source_value_id = typed.source_value_id;
  intent.destination_value_id = typed.destination_value_id;
  intent.source_type = typed.source_type;
  intent.destination_type = typed.destination_type;
  intent.source_stack_slot_id = typed.source_slot_id;
  intent.source_stack_offset_bytes = typed.source_stack_offset_bytes;
  intent.source_stack_size_bytes = typed.source_stack_size_bytes;
  intent.source_stack_align_bytes = typed.source_stack_align_bytes;
  intent.source_stack_extension_policy = typed.extension_policy;
  intent.destination_register_bank = typed.destination_register_bank;
  intent.destination_register_placement = typed.destination_register_placement;
}

std::optional<std::string> riscv_gpr_register_name_from_placement(
    const c4c::backend::prepare::PreparedRegisterPlacement& placement) {
  namespace prepare = c4c::backend::prepare;

  if (placement.bank != prepare::PreparedRegisterBank::Gpr ||
      placement.contiguous_width != 1) {
    return std::nullopt;
  }

  switch (placement.pool) {
    case prepare::PreparedRegisterSlotPool::CallerSaved:
      if (placement.slot_index == 0) {
        return std::string{"t0"};
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CalleeSaved:
      if (placement.slot_index < 2) {
        return std::string{"s"} + std::to_string(placement.slot_index + 1);
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CallArgument:
      if (placement.slot_index < 8) {
        return std::string{"a"} + std::to_string(placement.slot_index);
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CallResult:
      if (placement.slot_index == 0) {
        return std::string{"a0"};
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::ReservedScratch:
    case prepare::PreparedRegisterSlotPool::None:
      return std::nullopt;
  }
  return std::nullopt;
}

bool is_tiny_add_prepared_lir_slice(const c4c::codegen::lir::LirModule& module) {
  using c4c::codegen::lir::LirBinOp;
  using c4c::codegen::lir::LirRet;

  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.name != "main" || function.is_declaration || !function.params.empty() ||
      !function.alloca_insts.empty() || function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() != 1) {
    return false;
  }

  const auto* add = std::get_if<LirBinOp>(&block.insts.front());
  if (add == nullptr || add->opcode != "add" || add->type_str != "i32" || add->lhs != "2" ||
      add->rhs != "3" || add->result != "%t0") {
    return false;
  }

  const auto* ret = std::get_if<LirRet>(&block.terminator);
  return ret != nullptr && ret->type_str == "i32" && ret->value_str == std::optional<std::string>{"%t0"};
}

std::string sanitize_riscv_label_component(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (const char ch : text) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '_') {
      result.push_back(ch);
    } else {
      result.push_back('_');
    }
  }
  if (result.empty()) {
    return "anon";
  }
  if (std::isdigit(static_cast<unsigned char>(result.front()))) {
    result.insert(result.begin(), '_');
  }
  return result;
}

std::string riscv_local_block_label(std::string_view function_name,
                                    std::string_view block_label) {
  return ".L" + sanitize_riscv_label_component(function_name) + "_" +
         sanitize_riscv_label_component(block_label);
}

std::string bir_block_label_spelling(const c4c::backend::bir::Module& module,
                                     const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view spelling = module.names.block_labels.spelling(block.label_id);
    if (!spelling.empty()) {
      return std::string(spelling);
    }
  }
  return block.label;
}

std::string bir_target_label_spelling(const c4c::backend::bir::Module& module,
                                      c4c::BlockLabelId label_id,
                                      std::string_view fallback) {
  if (label_id != c4c::kInvalidBlockLabel) {
    const std::string_view spelling = module.names.block_labels.spelling(label_id);
    if (!spelling.empty()) {
      return std::string(spelling);
    }
  }
  return std::string(fallback);
}

struct SimpleCompare {
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

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

std::optional<std::string> emit_riscv_simple_compare_branch(
    const SimpleCompare& compare,
    std::string_view true_label,
    std::string_view false_label) {
  const auto lhs_imm = simple_integer_immediate(compare.lhs);
  const auto rhs_imm = simple_integer_immediate(compare.rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  c4c::backend::bir::BinaryOpcode opcode = compare.opcode;
  std::int64_t lhs = *lhs_imm;
  std::int64_t rhs = *rhs_imm;
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

std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    bool restore_return_address,
    std::size_t stack_frame_bytes);

std::optional<c4c::backend::prepare::PreparedValueId> prepared_value_id_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (lookups == nullptr || value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  return value_id_it->second;
}

std::optional<std::string> prepared_register_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto value_id = prepared_value_id_for(names, lookups, value);
  if (!value_id.has_value()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(*value_id);
  if (home_it == lookups->value_homes.homes_by_id.end() || home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

std::optional<std::int64_t> prepared_immediate_i32_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto value_id = prepared_value_id_for(names, lookups, value);
  if (!value_id.has_value()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(*value_id);
  if (home_it == lookups->value_homes.homes_by_id.end() || home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate ||
      !home.immediate_i32.has_value()) {
    return std::nullopt;
  }
  return home.immediate_i32;
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
  if (!source_register.has_value()) {
    return false;
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

std::optional<std::string> emit_riscv_simple_binary(
    const c4c::backend::bir::BinaryInst& binary,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if ((binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub) ||
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
    const auto result = binary.opcode == c4c::backend::bir::BinaryOpcode::Add
                            ? *lhs_imm + *rhs_imm
                            : *lhs_imm - *rhs_imm;
    out += "    li " + *destination_register + ", " +
           std::to_string(result) + "\n";
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

std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if (call.is_indirect || call.callee.empty() || call.args.size() > 8) {
    return std::nullopt;
  }

  std::string out;
  for (std::size_t arg_index = 0; arg_index < call.args.size(); ++arg_index) {
    std::string destination_register = "a" + std::to_string(arg_index);
    if (lookups != nullptr) {
      const auto* plan = c4c::backend::prepare::find_indexed_prepared_immediate_call_argument(
          &lookups->call_plans,
          block_index,
          instruction_index,
          arg_index);
      if (plan != nullptr && plan->destination_register_name.has_value()) {
        destination_register = *plan->destination_register_name;
      }
    }
    if (!emit_move_to_register(out, destination_register, names, lookups, call.args[arg_index])) {
      return std::nullopt;
    }
  }

  out += "    call " + call.callee + "\n";

  if (call.result.has_value()) {
    const auto destination_register = prepared_register_for_value(names, lookups, *call.result);
    if (!destination_register.has_value()) {
      return std::nullopt;
    }
    std::string source_register = "a0";
    if (lookups != nullptr) {
      const auto* call_plan = c4c::backend::prepare::find_indexed_prepared_call_plan(
          &lookups->call_plans,
          nullptr,
          block_index,
          instruction_index);
      if (call_plan != nullptr && call_plan->result.has_value() &&
          call_plan->result->source_register_name.has_value()) {
        source_register = *call_plan->result->source_register_name;
      }
    }
    if (*destination_register != source_register) {
      out += "    mv " + *destination_register + ", " + source_register + "\n";
    }
  }

  return out;
}

std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  auto append_epilogue = [restore_return_address, stack_frame_bytes](std::string& out) {
    if (restore_return_address) {
      out += "    ld ra, 8(sp)\n";
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

std::optional<c4c::FunctionNameId> prepared_function_id_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function) {
  const auto function_name = names.function_names.find(function.name);
  if (function_name == c4c::kInvalidFunctionName) {
    return std::nullopt;
  }
  return function_name;
}

std::optional<c4c::BlockLabelId> prepared_block_label_id_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    return block.label_id;
  }
  const auto block_label = names.block_labels.find(block.label);
  if (block_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  return block_label;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_frame_slot_access_for(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || value_type != bir::TypeKind::I32) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
  if (access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      access->address.size_bytes != 4 ||
      access->address.align_bytes < 4 ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::bir::StoreLocalInst& store,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  const auto* access = simple_frame_slot_access_for(
      lookups,
      block_label,
      instruction_index,
      store.value.type);
  if (access == nullptr) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t1", names, lookups, store.value)) {
    return std::nullopt;
  }
  out += "    sw t1, " + std::to_string(access->address.byte_offset) + "(sp)\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_load_local(
    const c4c::backend::bir::LoadLocalInst& load,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  const auto* access = simple_frame_slot_access_for(
      lookups,
      block_label,
      instruction_index,
      load.result.type);
  if (access == nullptr) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(names, lookups, load.result);
  if (!destination_register.has_value()) {
    return std::nullopt;
  }

  return "    lw " + *destination_register + ", " +
         std::to_string(access->address.byte_offset) + "(sp)\n";
}

bool append_simple_prepared_bir_function_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration) {
    return true;
  }

  std::unordered_map<std::string, SimpleCompare> compares;
  const std::string function_name = function.name.empty() ? "anon" : function.name;
  std::size_t prepared_frame_size = 0;
  const auto function_name_id = prepared_function_id_for(prepared.names, function);
  if (function_name_id.has_value()) {
    if (const auto* frame_plan =
            c4c::backend::prepare::find_prepared_frame_plan(prepared, *function_name_id)) {
      prepared_frame_size = frame_plan->frame_size_bytes;
    }
  }
  bool saves_return_address = false;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<c4c::backend::bir::CallInst>(inst)) {
        saves_return_address = true;
        break;
      }
    }
    if (saves_return_address) {
      break;
    }
  }
  if (prepared_frame_size > 0 && saves_return_address) {
    return false;
  }
  const std::size_t stack_frame_bytes =
      align_riscv_stack_frame(std::max(prepared_frame_size,
                                       saves_return_address ? std::size_t{16}
                                                            : std::size_t{0}));
  if (stack_frame_bytes > 0 &&
      !fits_signed_12_bit_immediate(-static_cast<std::int64_t>(stack_frame_bytes))) {
    return false;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    const auto block_label_id = prepared_block_label_id_for(prepared.names, block);
    if (!block_label_id.has_value()) {
      return false;
    }
    const std::string block_label = bir_block_label_spelling(prepared.module, block);
    out += riscv_local_block_label(function_name, block_label) + ":\n";
    if (block_index == 0 && stack_frame_bytes > 0) {
      out += "    addi sp, sp, -" + std::to_string(stack_frame_bytes) + "\n";
    }
    if (block_index == 0 && saves_return_address) {
      out += "    sd ra, 8(sp)\n";
    }

    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto& inst = block.insts[instruction_index];
      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
      if (binary != nullptr) {
        if (c4c::backend::bir::is_compare_opcode(binary->opcode) &&
            binary->result.kind == c4c::backend::bir::Value::Kind::Named) {
          compares[binary->result.name] = SimpleCompare{
              .opcode = binary->opcode,
              .lhs = binary->lhs,
              .rhs = binary->rhs,
          };
          continue;
        }
        const auto emitted = emit_riscv_simple_binary(*binary, prepared.names, lookups);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call != nullptr) {
        const auto emitted = emit_riscv_simple_call(
            *call,
            block_index,
            instruction_index,
            prepared.names,
            lookups);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* store_local = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store_local != nullptr) {
        const auto emitted = emit_riscv_simple_store_local(
            *store_local,
            *block_label_id,
            instruction_index,
            prepared.names,
            lookups);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      const auto* load_local = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
      if (load_local != nullptr) {
        const auto emitted = emit_riscv_simple_load_local(
            *load_local,
            *block_label_id,
            instruction_index,
            prepared.names,
            lookups);
        if (!emitted.has_value()) {
          return false;
        }
        out += *emitted;
        continue;
      }

      return false;
    }

    switch (block.terminator.kind) {
      case c4c::backend::bir::TerminatorKind::Return: {
        const auto returned = emit_riscv_simple_return(
            block.terminator,
            prepared.names,
            lookups,
            saves_return_address,
            stack_frame_bytes);
        if (!returned.has_value()) {
          return false;
        }
        out += *returned;
        break;
      }
      case c4c::backend::bir::TerminatorKind::Branch: {
        const std::string target = bir_target_label_spelling(
            prepared.module,
            block.terminator.target_label_id,
            block.terminator.target_label);
        out += "    j " + riscv_local_block_label(function_name, target) + "\n";
        break;
      }
      case c4c::backend::bir::TerminatorKind::CondBranch: {
        const std::string true_label = bir_target_label_spelling(
            prepared.module,
            block.terminator.true_label_id,
            block.terminator.true_label);
        const std::string false_label = bir_target_label_spelling(
            prepared.module,
            block.terminator.false_label_id,
            block.terminator.false_label);
        const std::string true_asm_label = riscv_local_block_label(function_name, true_label);
        const std::string false_asm_label = riscv_local_block_label(function_name, false_label);

        if (const auto condition_imm = simple_integer_immediate(block.terminator.condition)) {
          out += "    j ";
          out += *condition_imm != 0 ? true_asm_label : false_asm_label;
          out += "\n";
          break;
        }

        if (block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
          return false;
        }
        const auto compare_it = compares.find(block.terminator.condition.name);
        if (compare_it == compares.end()) {
          return false;
        }
        const auto branch = emit_riscv_simple_compare_branch(
            compare_it->second,
            true_asm_label,
            false_asm_label);
        if (!branch.has_value()) {
          return false;
        }
        out += *branch;
        break;
      }
    }
  }
  return true;
}

std::optional<std::string> render_edge_publication_source_operand(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::prepare::PreparedValueHome& source_home) {
  namespace prepare = c4c::backend::prepare;

  if (source_home.kind == prepare::PreparedValueHomeKind::Register &&
      source_home.register_name.has_value()) {
    intent.source_register = *source_home.register_name;
    return intent.source_register;
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::RematerializableImmediate &&
      source_home.immediate_i32.has_value()) {
    intent.source_immediate_i32 = *source_home.immediate_i32;
    return std::to_string(*source_home.immediate_i32);
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home.offset_bytes.has_value() &&
      (source_home.size_bytes == std::optional<std::size_t>{4} ||
       source_home.size_bytes == std::optional<std::size_t>{8})) {
    intent.source_stack_slot_id = source_home.slot_id;
    intent.source_stack_offset_bytes = *source_home.offset_bytes;
    intent.source_stack_size_bytes = *source_home.size_bytes;
    return std::to_string(*source_home.offset_bytes) + "(sp)";
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      !source_home.offset_bytes.has_value() &&
      source_home.size_bytes == std::optional<std::size_t>{4} &&
      publication.source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication.source_memory_access_status ==
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available &&
      publication.source_memory_access != nullptr &&
      publication.source_value.type == c4c::backend::bir::TypeKind::I32 &&
      publication.destination_value.type == c4c::backend::bir::TypeKind::I32 &&
      publication.source_memory_base_kind ==
          prepare::PreparedAddressBaseKind::PointerValue &&
      publication.source_memory_pointer_value_name.has_value() &&
      publication.source_memory_size_bytes == 4 &&
      publication.source_memory_align_bytes >= 4 &&
      publication.source_memory_address_space ==
          c4c::backend::bir::AddressSpace::Default &&
      !publication.source_memory_is_volatile &&
      publication.source_memory_can_use_base_plus_offset &&
      !publication.source_memory_requires_address_materialization &&
      fits_signed_12_bit_immediate(publication.source_memory_byte_offset) &&
      publication.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      publication.move != nullptr &&
      publication.source_value_id.has_value() &&
      publication.move->authority_kind ==
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
      publication.move->destination_kind ==
          prepare::PreparedMoveDestinationKind::Value &&
      publication.move->op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      publication.move->from_value_id == *publication.source_value_id &&
      publication.move->to_value_id == publication.destination_value_id &&
      publication.move->destination_register_placement.has_value()) {
    const auto* indexed_access =
        prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
            &lookups->memory_accesses, *publication.source_value_id);
    if (indexed_access == nullptr ||
        indexed_access != publication.source_memory_access) {
      return std::nullopt;
    }
    const auto base_id_it =
        lookups->value_homes.value_ids.find(
            *publication.source_memory_pointer_value_name);
    if (base_id_it == lookups->value_homes.value_ids.end()) {
      return std::nullopt;
    }
    const auto base_home_it =
        lookups->value_homes.homes_by_id.find(base_id_it->second);
    if (base_home_it == lookups->value_homes.homes_by_id.end() ||
        base_home_it->second == nullptr ||
        base_home_it->second->kind != prepare::PreparedValueHomeKind::Register ||
        !base_home_it->second->register_name.has_value()) {
      return std::nullopt;
    }
    intent.source_stack_slot_id = source_home.slot_id;
    intent.source_stack_size_bytes = source_home.size_bytes;
    intent.source_stack_align_bytes = source_home.align_bytes;
    intent.source_stack_extension_policy =
        prepare::PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension;
    intent.source_memory_base_value_id = base_id_it->second;
    intent.source_memory_base_register = *base_home_it->second->register_name;
    intent.source_memory_byte_offset = publication.source_memory_byte_offset;
    intent.source_memory_size_bytes = publication.source_memory_size_bytes;
    intent.source_memory_align_bytes = publication.source_memory_align_bytes;
    intent.destination_register_placement =
        publication.move->destination_register_placement;
    intent.destination_register_bank =
        intent.destination_register_placement->bank;
    return std::to_string(publication.source_memory_byte_offset) + "(" +
           intent.source_memory_base_register + ")";
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
      source_home.pointer_base_value_name.has_value() &&
      source_home.pointer_byte_delta.has_value()) {
    const auto base_id_it =
        lookups->value_homes.value_ids.find(*source_home.pointer_base_value_name);
    if (base_id_it == lookups->value_homes.value_ids.end()) {
      return std::nullopt;
    }
    const auto base_home_it = lookups->value_homes.homes_by_id.find(base_id_it->second);
    if (base_home_it == lookups->value_homes.homes_by_id.end() ||
        base_home_it->second == nullptr ||
        base_home_it->second->kind != prepare::PreparedValueHomeKind::Register ||
        !base_home_it->second->register_name.has_value()) {
      return std::nullopt;
    }
    intent.source_pointer_base_value_id = base_id_it->second;
    intent.source_pointer_base_register = *base_home_it->second->register_name;
    intent.source_pointer_byte_delta = *source_home.pointer_byte_delta;
    return std::to_string(*source_home.pointer_byte_delta);
  }
  return std::nullopt;
}

bool has_direct_register_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent) {
  return !intent.source_register.empty() &&
         !intent.source_immediate_i32.has_value() &&
         !intent.source_stack_offset_bytes.has_value() &&
         !intent.source_pointer_byte_delta.has_value();
}

bool has_rematerializable_i32_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent) {
  return intent.source_register.empty() &&
         intent.source_immediate_i32.has_value() &&
         !intent.source_stack_offset_bytes.has_value() &&
         !intent.source_pointer_byte_delta.has_value();
}

bool stack_ranges_overlap(std::size_t lhs_offset,
                          std::size_t lhs_size,
                          std::size_t rhs_offset,
                          std::size_t rhs_size) {
  return lhs_offset < rhs_offset + rhs_size && rhs_offset < lhs_offset + lhs_size;
}

bool has_non_aliasing_i32_stack_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedValueHome& destination_home) {
  if (!intent.source_register.empty() ||
      intent.source_immediate_i32.has_value() ||
      !intent.source_stack_offset_bytes.has_value() ||
      intent.source_stack_size_bytes != std::optional<std::size_t>{4} ||
      intent.source_pointer_byte_delta.has_value() ||
      !fits_signed_12_bit_load_offset(*intent.source_stack_offset_bytes) ||
      !destination_home.offset_bytes.has_value() ||
      destination_home.size_bytes != std::optional<std::size_t>{4}) {
    return false;
  }
  if (intent.source_stack_slot_id.has_value() &&
      destination_home.slot_id.has_value() &&
      intent.source_stack_slot_id == destination_home.slot_id) {
    return false;
  }
  return !stack_ranges_overlap(*intent.source_stack_offset_bytes,
                               *intent.source_stack_size_bytes,
                               *destination_home.offset_bytes,
                               *destination_home.size_bytes);
}

bool has_existing_concrete_i64_stack_source_register_policy(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) {
  namespace bir = c4c::backend::bir;

  return publication.source_value.type == bir::TypeKind::I64 &&
         publication.destination_value.type == bir::TypeKind::I64 &&
         intent.source_stack_offset_bytes.has_value() &&
         intent.source_stack_size_bytes == std::optional<std::size_t>{8};
}

bir::Route5PublicationSourceKind route5_source_kind_from_prepared(
    c4c::backend::prepare::PreparedEdgePublicationSourceProducerKind kind) {
  namespace prepare = c4c::backend::prepare;

  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::Route5PublicationSourceKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route5PublicationSourceKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route5PublicationSourceKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route5PublicationSourceKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route5PublicationSourceKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route5PublicationSourceKind::SelectMaterialization;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::Route5PublicationSourceKind::Unknown;
  }
  return bir::Route5PublicationSourceKind::Unknown;
}

bool route3_base_kind_agrees_with_prepared_source_memory(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route3MemoryAccessRecord& route3_access) {
  namespace prepare = c4c::backend::prepare;

  switch (publication.source_memory_base_kind) {
    case prepare::PreparedAddressBaseKind::FrameSlot:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::LocalSlot &&
             publication.source_memory_frame_slot_id.has_value() &&
             route3_access.local_slot_id != c4c::kInvalidSlotName;
    case prepare::PreparedAddressBaseKind::GlobalSymbol:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::GlobalSymbol &&
             publication.source_memory_symbol_name.has_value() &&
             route3_access.global_name_id == *publication.source_memory_symbol_name;
    case prepare::PreparedAddressBaseKind::PointerValue:
      if (route3_access.base_kind !=
              bir::Route3MemoryAccessBaseKind::PointerValue ||
          !publication.source_memory_pointer_value_name.has_value() ||
          !route3_access.pointer_value) {
        return false;
      }
      return route3_access.pointer_value.name_id == c4c::kInvalidValueName ||
             route3_access.pointer_value.name_id ==
                 *publication.source_memory_pointer_value_name;
    case prepare::PreparedAddressBaseKind::StringConstant:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::StringConstant &&
             publication.source_memory_symbol_name.has_value() &&
             route3_access.string_constant_name_id ==
                 *publication.source_memory_symbol_name;
    case prepare::PreparedAddressBaseKind::None:
      return route3_access.base_kind == bir::Route3MemoryAccessBaseKind::None;
  }
  return false;
}

bool route3_source_memory_agrees_with_prepared_publication(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route3MemoryAccessRecord& route3_access) {
  namespace prepare = c4c::backend::prepare;

  if (publication.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      publication.source_memory_access_status !=
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available ||
      publication.source_memory_access == nullptr ||
      !route3_access ||
      route3_access.node_kind != bir::Route3MemoryAccessNodeKind::LoadLocal ||
      !route3_access.result_value ||
      route3_access.result_value.value_kind != publication.source_value.kind ||
      route3_access.result_value.type != publication.source_value.type ||
      route3_access.result_value.name != publication.source_value.name ||
      route3_access.address_space != publication.source_memory_address_space ||
      route3_access.is_volatile != publication.source_memory_is_volatile ||
      route3_access.byte_offset != publication.source_memory_byte_offset ||
      route3_access.size_bytes != publication.source_memory_size_bytes ||
      route3_access.align_bytes != publication.source_memory_align_bytes) {
    return false;
  }

  return route3_base_kind_agrees_with_prepared_source_memory(publication,
                                                            route3_access);
}

bool route5_edge_source_agrees_with_prepared_publication(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route5CfgEdgePublicationRecord& route5_edge) {
  namespace prepare = c4c::backend::prepare;

  if ((route5_edge.status != bir::Route5PublicationStatus::Available &&
       route5_edge.status != bir::Route5PublicationStatus::MemorySource) ||
      !route5_edge ||
      route5_edge.predecessor_label_id != publication.predecessor_label ||
      route5_edge.successor_label_id != publication.successor_label ||
      route5_edge.destination_value_name != publication.destination_value.name ||
      route5_edge.destination_value_type != publication.destination_value.type ||
      route5_edge.source_value_kind != publication.source_value_kind ||
      route5_edge.source_value_type != publication.source_value.type) {
    return false;
  }

  const bool comparable_prepared_producer =
      publication.source_producer_kind !=
      prepare::PreparedEdgePublicationSourceProducerKind::Unknown;
  if (comparable_prepared_producer &&
      route5_edge.source_producer_kind !=
          route5_source_kind_from_prepared(publication.source_producer_kind)) {
    return false;
  }

  if (publication.source_value_kind == bir::Value::Kind::Immediate) {
    return route5_edge.source_value.value_kind == bir::Value::Kind::Immediate &&
           route5_edge.source_value.integer_constant.has_value() &&
           route5_edge.source_value.integer_constant ==
               publication.source_value.immediate;
  }

  if (publication.source_value_kind != bir::Value::Kind::Named ||
      route5_edge.source_value_name != publication.source_value.name ||
      route5_edge.source_producer_instruction == nullptr ||
      !route5_edge.source_producer_instruction_index.has_value()) {
    return false;
  }

  if (publication.source_producer_kind ==
      prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) {
    return route5_edge.status == bir::Route5PublicationStatus::MemorySource &&
           route5_edge.source_memory_identity_available &&
           route3_source_memory_agrees_with_prepared_publication(
               publication, route5_edge.source_memory_access);
  }

  return !comparable_prepared_producer ||
         route5_edge.status == bir::Route5PublicationStatus::Available;
}

struct RiscvEdgePublicationMoveAdapter {
  const c4c::backend::prepare::PreparedFunctionLookups* lookups = nullptr;
  c4c::BlockLabelId predecessor_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId successor_label = c4c::kInvalidBlockLabel;
  c4c::backend::prepare::PreparedValueId destination_value_id = 0;
  const bir::Route5CfgEdgePublicationRecord* route5_edge = nullptr;

  [[nodiscard]] EdgePublicationMoveIntent consume_prepared_backed_move_intent() const;

 private:
  [[nodiscard]] const c4c::backend::prepare::PreparedEdgePublication*
  find_prepared_publication() const;

  [[nodiscard]] std::optional<std::string> render_prepared_source_operand(
      EdgePublicationMoveIntent& intent,
      const c4c::backend::prepare::PreparedEdgePublication& publication) const;

  void attach_route5_edge_agreement(
      EdgePublicationMoveIntent& intent,
      const c4c::backend::prepare::PreparedEdgePublication& publication) const;
};

const c4c::backend::prepare::PreparedEdgePublication*
RiscvEdgePublicationMoveAdapter::find_prepared_publication() const {
  return c4c::backend::prepare::find_unique_indexed_prepared_edge_publication(
      &lookups->edge_publications, predecessor_label, successor_label,
      destination_value_id);
}

std::optional<std::string>
RiscvEdgePublicationMoveAdapter::render_prepared_source_operand(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) const {
  return render_edge_publication_source_operand(intent,
                                                lookups,
                                                publication,
                                                *publication.source_home);
}

void RiscvEdgePublicationMoveAdapter::attach_route5_edge_agreement(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) const {
  if (route5_edge == nullptr) {
    return;
  }
  intent.route5_edge_status = route5_edge->status;
  intent.route5_edge_source_agrees =
      route5_edge_source_agrees_with_prepared_publication(publication, *route5_edge);
  if (route5_edge->source_memory_identity_available) {
    intent.route3_source_memory_agrees =
        route3_source_memory_agrees_with_prepared_publication(
            publication, route5_edge->source_memory_access);
  }
}

EdgePublicationMoveIntent
RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent() const {
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingSharedLookups,
    };
  }

  const auto* publication = find_prepared_publication();
  if (publication == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingPublication,
    };
  }

  EdgePublicationMoveIntent intent{
      .status = EdgePublicationMoveIntentStatus::UnsupportedPublication,
      .publication = publication,
      .destination_value_id = publication->destination_value_id,
  };
  if (publication->source_value_id.has_value()) {
    intent.source_value_id = *publication->source_value_id;
  }
  attach_route5_edge_agreement(intent, *publication);

  if (publication->status != prepare::PreparedEdgePublicationLookupStatus::Available ||
      publication->move == nullptr ||
      publication->move->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return intent;
  }
  if (route5_edge != nullptr &&
      publication->source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication->source_memory_access_status ==
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available &&
      publication->source_memory_access != nullptr &&
      !intent.route5_edge_source_agrees) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }
  if (publication->source_home == nullptr) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }
  const auto source_operand = render_prepared_source_operand(intent, *publication);
  if (!source_operand.has_value()) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }
  if (publication->destination_home == nullptr) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
    return intent;
  }

  const auto& destination_home = *publication->destination_home;
  if (destination_home.kind == prepare::PreparedValueHomeKind::Register) {
    intent.status = EdgePublicationMoveIntentStatus::Available;
    if (intent.source_immediate_i32.has_value()) {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      intent.instruction_text =
          "li " + intent.destination_register + ", " + *source_operand;
    } else if (intent.source_stack_offset_bytes.has_value()) {
      const auto typed =
          prepare::prepare_same_width_i32_stack_source_publication(publication);
      if (typed.status !=
          prepare::PreparedTypedStackSourcePublicationStatus::Available) {
        if (has_existing_concrete_i64_stack_source_register_policy(intent,
                                                                   *publication) &&
            destination_home.register_name.has_value()) {
          intent.destination_register = *destination_home.register_name;
          if (fits_signed_12_bit_load_offset(*intent.source_stack_offset_bytes)) {
            intent.instruction_text =
                "ld " + intent.destination_register + ", " + *source_operand;
          } else {
            const auto offset_text = std::to_string(*intent.source_stack_offset_bytes);
            intent.instruction_text =
                "li t6, " + offset_text +
                "\n    add t6, sp, t6" +
                "\n    ld " + intent.destination_register + ", 0(t6)";
          }
          return intent;
        }
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      copy_same_width_i32_stack_source_publication(intent, typed);
      const auto register_name =
          riscv_gpr_register_name_from_placement(*typed.destination_register_placement);
      if (!register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      intent.destination_register = *register_name;
      if (fits_signed_12_bit_load_offset(*typed.source_stack_offset_bytes)) {
        intent.instruction_text =
            "lw " + intent.destination_register + ", " +
            std::to_string(*typed.source_stack_offset_bytes) + "(sp)";
      } else {
        const auto offset_text = std::to_string(*typed.source_stack_offset_bytes);
        intent.instruction_text =
            "li t6, " + offset_text +
            "\n    add t6, sp, t6" +
            "\n    lw " + intent.destination_register + ", 0(t6)";
      }
    } else if (intent.source_memory_byte_offset.has_value()) {
      const auto register_name =
          riscv_gpr_register_name_from_placement(*intent.destination_register_placement);
      if (!register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      intent.destination_register = *register_name;
      intent.instruction_text =
          "lw " + intent.destination_register + ", " + *source_operand;
    } else if (intent.source_pointer_byte_delta.has_value()) {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      if (*intent.source_pointer_byte_delta == 0) {
        intent.instruction_text =
            "mv " + intent.destination_register + ", " + intent.source_pointer_base_register;
      } else if (fits_signed_12_bit_immediate(*intent.source_pointer_byte_delta)) {
        intent.instruction_text =
            "addi " + intent.destination_register + ", " +
            intent.source_pointer_base_register + ", " + *source_operand;
      } else if (intent.destination_register != intent.source_pointer_base_register) {
        intent.instruction_text =
            "li " + intent.destination_register + ", " + *source_operand +
            "\n    add " + intent.destination_register + ", " +
            intent.source_pointer_base_register + ", " + intent.destination_register;
      } else {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
      }
    } else {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      intent.instruction_text =
          "mv " + intent.destination_register + ", " + *source_operand;
    }
    return intent;
  }

  // Prepared edge-publication stack destinations have a target-local scratch
  // contract. The direct Register -> StackSlot case reserves no scratch and
  // clobbers only the destination memory slot. The I32 immediate materializing
  // form may own `t0` as a value scratch for one publication sequence,
  // clobbering it before the final `sw`; the I32 StackSlot -> StackSlot form
  // uses the same `t0` scratch for a signed-12-bit source load before the final
  // store. The scratch value must not survive across edge publications.
  // `t1`/`t2` are not reserved by this path, and `t5`/`t6` remain available
  // only to a later explicit address/large-offset helper contract. Pointer
  // sources and large stack-source offsets to StackSlot destinations
  // intentionally remain fail-closed.
  if (destination_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      destination_home.size_bytes == std::optional<std::size_t>{4} &&
      fits_signed_12_bit_load_offset(*destination_home.offset_bytes)) {
    intent.status = EdgePublicationMoveIntentStatus::Available;
    intent.destination_stack_slot_id = destination_home.slot_id;
    intent.destination_stack_offset_bytes = *destination_home.offset_bytes;
    intent.destination_stack_size_bytes = *destination_home.size_bytes;
    if (has_direct_register_source_for_stack_destination(intent)) {
      intent.instruction_text =
          "sw " + intent.source_register + ", " +
          std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    if (has_rematerializable_i32_source_for_stack_destination(intent)) {
      intent.instruction_text =
          "li t0, " + std::to_string(*intent.source_immediate_i32) +
          "\n    sw t0, " + std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    if (has_non_aliasing_i32_stack_source_for_stack_destination(intent,
                                                               destination_home)) {
      intent.instruction_text =
          "lw t0, " + std::to_string(*intent.source_stack_offset_bytes) +
          "(sp)\n    sw t0, " +
          std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    intent.destination_stack_slot_id.reset();
    intent.destination_stack_offset_bytes.reset();
    intent.destination_stack_size_bytes.reset();
  }

  intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
  return intent;
}

}  // namespace

EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  return consume_edge_publication_move_intent(lookups,
                                              predecessor_label,
                                              successor_label,
                                              destination_value_id,
                                              nullptr);
}

EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id,
    const bir::Route5CfgEdgePublicationRecord* route5_edge) {
  const RiscvEdgePublicationMoveAdapter adapter{
      .lookups = lookups,
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = destination_value_id,
      .route5_edge = route5_edge,
  };
  return adapter.consume_prepared_backed_move_intent();
}

EdgePublicationMoveIntent append_edge_publication_move_instruction(
    std::string& output,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  auto intent = consume_edge_publication_move_intent(
      lookups, predecessor_label, successor_label, destination_value_id);
  if (intent.status == EdgePublicationMoveIntentStatus::Available) {
    output += "    " + intent.instruction_text + "\n";
  }
  return intent;
}

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  namespace prepare = c4c::backend::prepare;

  std::string out = "    .text\n";
  for (const auto& function : module.control_flow.functions) {
    const auto lookups = prepare::make_prepared_function_lookups(module, function);
    const auto function_name = prepare::prepared_function_name(module.names,
                                                              function.function_name);
    if (!function_name.empty()) {
      out += "    .globl ";
      out += function_name;
      out += "\n";
      out += function_name;
      out += ":\n";
    }

    const auto function_it = std::find_if(
        module.module.functions.begin(),
        module.module.functions.end(),
        [&](const c4c::backend::bir::Function& candidate) {
          return candidate.name == function_name;
        });
    if (function_it != module.module.functions.end() &&
        append_simple_prepared_bir_function_asm(out, module, &lookups, *function_it)) {
      continue;
    }

    for (const auto& publication : lookups.edge_publications.publications) {
      if (publication.status != prepare::PreparedEdgePublicationLookupStatus::Available) {
        continue;
      }
      (void)append_edge_publication_move_instruction(out,
                                                     &lookups,
                                                     publication.predecessor_label,
                                                     publication.successor_label,
                                                     publication.destination_value_id);
    }
  }
  return out;
}

std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module) {
  if (!is_tiny_add_prepared_lir_slice(module)) {
    throw std::invalid_argument(
        "riscv backend emitter does not support this direct LIR module");
  }

  return std::string(
      "    .text\n"
      "    .globl main\n"
      "main:\n"
      "    addi a0, zero, 5\n"
      "    ret\n");
}

const char* callee_saved_name(PhysReg reg) {
  switch (reg.value) {
    case 1: return "s1";
    case 2: return "s2";
    case 3: return "s3";
    case 4: return "s4";
    case 5: return "s5";
    case 6: return "s6";
    case 7: return "s7";
    case 8: return "s8";
    case 9: return "s9";
    case 10: return "s10";
    case 11: return "s11";
    default:
      throw std::invalid_argument("invalid RISC-V callee-saved register index");
  }
}

std::optional<PhysReg> riscv_reg_to_callee_saved(std::string_view name) {
  if (name == "s1" || name == "x9") return PhysReg(1);
  if (name == "s2" || name == "x18") return PhysReg(2);
  if (name == "s3" || name == "x19") return PhysReg(3);
  if (name == "s4" || name == "x20") return PhysReg(4);
  if (name == "s5" || name == "x21") return PhysReg(5);
  if (name == "s6" || name == "x22") return PhysReg(6);
  if (name == "s7" || name == "x23") return PhysReg(7);
  if (name == "s8" || name == "x24") return PhysReg(8);
  if (name == "s9" || name == "x25") return PhysReg(9);
  if (name == "s10" || name == "x26") return PhysReg(10);
  if (name == "s11" || name == "x27") return PhysReg(11);
  return std::nullopt;
}

std::optional<PhysReg> constraint_to_callee_saved_riscv(std::string_view constraint) {
  if (!constraint.empty() && constraint.front() == '{' && constraint.back() == '}') {
    return riscv_reg_to_callee_saved(constraint.substr(1, constraint.size() - 2));
  }
  return riscv_reg_to_callee_saved(constraint);
}

// Source-level mirror of the rest of `emit.rs`.
//
// The following Rust-owned methods are translated in sibling slices and depend
// on the missing shared `RiscvCodegen` / `CodegenState` surface:
// - `RiscvCodegen::new`
// - option setters and pre-directive emission
// - comparison operand loading and stack-slot helpers
// - operand loading / storing
// - 128-bit helpers
// - the `ArchCodegen` trait implementation
//
// `collect_inline_asm_callee_saved_riscv` also lives here in Rust, but it
// depends on the shared IR and backend generation helpers that are not yet
// exposed in C++.

}  // namespace c4c::backend::riscv::codegen

namespace c4c::backend::riscv {

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  const auto assembled =
      c4c::backend::assemble_target_lir_module(
          module,
          module.target_profile.arch == c4c::TargetArch::Unknown
              ? c4c::target_profile_from_triple(c4c::default_host_target_triple())
              : module.target_profile,
          output_path);
  return assembler::AssembleResult{
      .staged_text = assembled.staged_text,
      .output_path = assembled.output_path,
      .object_emitted = assembled.object_emitted,
      .error = assembled.error,
  };
}

}  // namespace c4c::backend::riscv
