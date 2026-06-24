#include "object_emission.hpp"

#include "../../../prealloc/prepared_lookups.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

namespace c4c::backend::riscv::codegen {
namespace {

namespace object = c4c::backend::mir::object;
namespace prepare = c4c::backend::prepare;

constexpr std::uint16_t kElfMachineRiscv = 243;
constexpr std::uint32_t kRiscvElfFlagsRv64DoubleFloatAbi = 0x5;
constexpr std::uint32_t kRiscvRelocCallPlt = 19;
constexpr std::uint32_t kRiscvRelocPcrelHi20 = 23;
constexpr std::uint32_t kRiscvRelocPcrelLo12I = 24;

constexpr std::uint32_t encode_u_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t imm20) {
  return ((imm20 & 0xfffffu) << 12) | ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_i_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::int32_t imm12) {
  return ((static_cast<std::uint32_t>(imm12) & 0xfffu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_s_type(std::uint32_t opcode, std::uint32_t funct3,
                                      std::uint32_t rs1, std::uint32_t rs2,
                                      std::int32_t imm12) {
  const auto imm = static_cast<std::uint32_t>(imm12) & 0xfffu;
  return ((imm >> 5) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((imm & 0x1fu) << 7) | (opcode & 0x7fu);
}

constexpr std::uint32_t encode_r_type(std::uint32_t opcode, std::uint32_t rd,
                                      std::uint32_t funct3, std::uint32_t rs1,
                                      std::uint32_t rs2, std::uint32_t funct7) {
  return ((funct7 & 0x7fu) << 25) | ((rs2 & 0x1fu) << 20) |
         ((rs1 & 0x1fu) << 15) | ((funct3 & 0x7u) << 12) |
         ((rd & 0x1fu) << 7) | (opcode & 0x7fu);
}

void append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  bytes.push_back(static_cast<std::uint8_t>(word & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 8) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 16) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 24) & 0xffu));
}

constexpr bool fits_signed_12_bit_immediate(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

object::SymbolBinding binding_for_function(const RiscvObjectFunction& function) {
  return function.global ? object::SymbolBinding::Global
                         : object::SymbolBinding::Local;
}

std::optional<std::uint32_t> rv64_register_number(std::string_view name) {
  if (name.size() >= 2 && name.front() == 'x') {
    std::uint32_t value = 0;
    const char* const begin = name.data() + 1;
    const char* const end = name.data() + name.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec == std::errc{} && ptr == end && value <= 31) {
      return value;
    }
    return std::nullopt;
  }
  if (name == "zero") return 0;
  if (name == "ra") return 1;
  if (name == "sp") return 2;
  if (name == "gp") return 3;
  if (name == "tp") return 4;
  if (name == "t0") return 5;
  if (name == "t1") return 6;
  if (name == "t2") return 7;
  if (name == "s0" || name == "fp") return 8;
  if (name == "s1") return 9;
  if (name == "a0") return 10;
  if (name == "a1") return 11;
  if (name == "a2") return 12;
  if (name == "a3") return 13;
  if (name == "a4") return 14;
  if (name == "a5") return 15;
  if (name == "a6") return 16;
  if (name == "a7") return 17;
  if (name == "s2") return 18;
  if (name == "s3") return 19;
  if (name == "s4") return 20;
  if (name == "s5") return 21;
  if (name == "s6") return 22;
  if (name == "s7") return 23;
  if (name == "s8") return 24;
  if (name == "s9") return 25;
  if (name == "s10") return 26;
  if (name == "s11") return 27;
  if (name == "t3") return 28;
  if (name == "t4") return 29;
  if (name == "t5") return 30;
  if (name == "t6") return 31;
  return std::nullopt;
}

std::optional<std::uint32_t> gpr_register_number_for_home(
    const c4c::backend::prepare::PreparedValueHome& home) {
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }
  if (home.target_register_identity.has_value() &&
      home.target_register_identity->bank !=
          c4c::backend::prepare::PreparedRegisterBank::Gpr) {
    return std::nullopt;
  }
  return rv64_register_number(*home.register_name);
}

const c4c::backend::prepare::PreparedValueHome* prepared_value_home_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (lookups == nullptr || value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return nullptr;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  return home_it == lookups->value_homes.homes_by_id.end() ? nullptr
                                                           : home_it->second;
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
  if (home == nullptr ||
      home->kind != c4c::backend::prepare::PreparedValueHomeKind::
                        RematerializableImmediate ||
      !home->immediate_i32.has_value()) {
    return std::nullopt;
  }
  return home->immediate_i32;
}

std::optional<std::uint32_t> gpr_register_number_for_value(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt : gpr_register_number_for_home(*home);
}

void append_rv64_move(RiscvEncodedFragment& fragment,
                      std::uint32_t destination,
                      std::uint32_t source) {
  if (destination == source) {
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13, destination, 0, source, 0));  // addi rd, rs, 0
}

void append_rv64_load_immediate(RiscvEncodedFragment& fragment,
                                std::uint32_t destination,
                                std::int64_t immediate) {
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            destination,
                            0,
                            0,
                            static_cast<std::int32_t>(immediate)));
}

std::string_view trim_ascii(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t' ||
                          text.front() == '\n' || text.front() == '\r')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t' ||
                          text.back() == '\n' || text.back() == '\r')) {
    text.remove_suffix(1);
  }
  return text;
}

std::optional<std::uint32_t> parse_rv64_insn_u32(std::string_view text,
                                                 std::uint32_t max_value) {
  text = trim_ascii(text);
  if (text.empty()) {
    return std::nullopt;
  }
  int base = 10;
  if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text.remove_prefix(2);
    base = 16;
  }
  std::uint32_t value = 0;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, base);
  if (ec != std::errc{} || ptr != end || value > max_value) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint32_t> rv64_register_number_for_inline_asm_operand(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier,
    std::string_view token) {
  token = trim_ascii(token);
  if (token.size() < 2 || (token.front() != '%' && token.front() != '$')) {
    return std::nullopt;
  }
  token.remove_prefix(1);
  std::size_t operand_index = 0;
  const char* const begin = token.data();
  const char* const end = token.data() + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, operand_index);
  if (ec != std::errc{} || ptr != end || operand_index >= carrier.operands.size()) {
    return std::nullopt;
  }

  const auto& operand = carrier.operands[operand_index];
  const auto constraint_is_decimal_index = [](std::string_view constraint) {
    return !constraint.empty() &&
           std::all_of(constraint.begin(), constraint.end(), [](unsigned char ch) {
             return std::isdigit(ch) != 0;
           });
  };
  switch (operand.kind) {
    case c4c::backend::bir::InlineAsmOperandKind::RegisterOutput:
      if (operand.constraint != "=r" && operand.constraint != "+r") {
        return std::nullopt;
      }
      if (!operand.output_index.has_value() || *operand.output_index != 0 ||
          !carrier.result_home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*carrier.result_home);
    case c4c::backend::bir::InlineAsmOperandKind::RegisterInput:
      if (operand.constraint != "r") {
        return std::nullopt;
      }
      if (!operand.arg_index.has_value() || !operand.home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*operand.home);
    case c4c::backend::bir::InlineAsmOperandKind::TiedInput:
      if (!constraint_is_decimal_index(operand.constraint)) {
        return std::nullopt;
      }
      if (!operand.arg_index.has_value() || !operand.home.has_value()) {
        return std::nullopt;
      }
      return gpr_register_number_for_home(*operand.home);
    case c4c::backend::bir::InlineAsmOperandKind::Unsupported:
    case c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput:
    case c4c::backend::bir::InlineAsmOperandKind::MemoryInput:
    case c4c::backend::bir::InlineAsmOperandKind::AddressInput:
    case c4c::backend::bir::InlineAsmOperandKind::Clobber:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<RiscvEncodedFragment> fragment_for_rv64_insn_r_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier* carrier,
    const c4c::backend::bir::CallInst& call) {
  namespace prepare = c4c::backend::prepare;
  if (carrier == nullptr ||
      carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !call.inline_asm.has_value() || call.callee != "llvm.inline_asm" ||
      call.is_indirect || call.callee_value.has_value() ||
      carrier->has_named_operand_references || carrier->has_template_modifiers ||
      !carrier->clobbers.empty() || !carrier->missing_required_facts.empty()) {
    return std::nullopt;
  }

  std::string_view text = trim_ascii(call.inline_asm->asm_text);
  constexpr std::string_view prefix = ".insn r";
  if (text.size() < prefix.size() || text.substr(0, prefix.size()) != prefix ||
      (text.size() > prefix.size() && text[prefix.size()] != ' ' &&
       text[prefix.size()] != '\t')) {
    return std::nullopt;
  }
  text.remove_prefix(prefix.size());
  text = trim_ascii(text);

  std::vector<std::string_view> fields;
  while (true) {
    const std::size_t comma = text.find(',');
    const bool last = comma == std::string_view::npos;
    fields.push_back(trim_ascii(last ? text : text.substr(0, comma)));
    if (last) {
      break;
    }
    text.remove_prefix(comma + 1);
    if (fields.size() == 6) {
      return std::nullopt;
    }
  }
  if (fields.size() != 6 ||
      std::any_of(fields.begin(), fields.end(), [](std::string_view field) {
        return field.empty();
      })) {
    return std::nullopt;
  }

  const auto opcode = parse_rv64_insn_u32(fields[0], 0x7f);
  const auto funct3 = parse_rv64_insn_u32(fields[1], 0x7);
  const auto funct7 = parse_rv64_insn_u32(fields[2], 0x7f);
  const auto rd = rv64_register_number_for_inline_asm_operand(*carrier, fields[3]);
  const auto rs1 = rv64_register_number_for_inline_asm_operand(*carrier, fields[4]);
  const auto rs2 = rv64_register_number_for_inline_asm_operand(*carrier, fields[5]);
  if (!opcode.has_value() || !funct3.has_value() || !funct7.has_value() ||
      !rd.has_value() || !rs1.has_value() || !rs2.has_value()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_r_type(*opcode, *rd, *funct3, *rs1, *rs2, *funct7));
  return fragment;
}

RiscvEncodedFragment make_rv64_return_immediate_fragment(std::int64_t immediate) {
  RiscvEncodedFragment fragment;
  append_rv64_load_immediate(fragment, 10, immediate);
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
}

std::size_t rv64_call_frame_size(std::size_t local_frame_bytes) {
  return local_frame_bytes + 16;
}

std::int32_t rv64_call_frame_ra_offset(std::size_t local_frame_bytes) {
  return static_cast<std::int32_t>(local_frame_bytes + 8);
}

RiscvEncodedFragment make_rv64_call_frame_prologue_fragment(
    std::size_t local_frame_bytes) {
  RiscvEncodedFragment fragment;
  const auto frame_size = rv64_call_frame_size(local_frame_bytes);
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            -static_cast<std::int32_t>(frame_size)));
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            3,
                            2,
                            1,
                            rv64_call_frame_ra_offset(local_frame_bytes)));
  return fragment;
}

RiscvEncodedFragment make_rv64_stack_frame_prologue_fragment(
    std::size_t stack_frame_bytes) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            -static_cast<std::int32_t>(stack_frame_bytes)));
  return fragment;
}

void append_rv64_call_frame_epilogue(RiscvEncodedFragment& fragment,
                                     std::size_t local_frame_bytes) {
  const auto frame_size = rv64_call_frame_size(local_frame_bytes);
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            1,
                            3,
                            2,
                            rv64_call_frame_ra_offset(local_frame_bytes)));
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            static_cast<std::int32_t>(frame_size)));
}

void append_rv64_stack_frame_epilogue(RiscvEncodedFragment& fragment,
                                      std::size_t stack_frame_bytes) {
  if (stack_frame_bytes == 0) {
    return;
  }
  append_le32(fragment.bytes,
              encode_i_type(0x13,
                            2,
                            0,
                            2,
                            static_cast<std::int32_t>(stack_frame_bytes)));
}

std::optional<std::size_t> rv64_object_stack_frame_size(
    const c4c::backend::prepare::PreparedAddressingFunction* addressing) {
  if (addressing == nullptr || addressing->frame_size_bytes == 0) {
    return 0;
  }
  const auto aligned = ((addressing->frame_size_bytes + 15) / 16) * 16;
  return fits_signed_12_bit_immediate(static_cast<std::int64_t>(aligned))
             ? std::optional<std::size_t>{aligned}
             : std::nullopt;
}

const c4c::backend::bir::Value* pure_instruction_result(
    const c4c::backend::bir::Inst& inst) {
  if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
    return &binary->result;
  }
  if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
    return &select->result;
  }
  if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
    return &cast->result;
  }
  return nullptr;
}

bool prepared_pure_instruction_is_rematerialized_immediate(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Inst& inst) {
  const auto* result = pure_instruction_result(inst);
  if (result == nullptr) {
    return false;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, *result);
  return immediate.has_value() && fits_signed_12_bit_immediate(*immediate);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_call(
    const c4c::backend::prepare::PreparedCallPlan* call_plan,
    const c4c::backend::prepare::PreparedInlineAsmCarrier* inline_asm_carrier,
    const c4c::backend::bir::CallInst& call) {
  namespace prepare = c4c::backend::prepare;

  if (call.inline_asm.has_value()) {
    return fragment_for_rv64_insn_r_inline_asm(inline_asm_carrier, call);
  }

  if (call_plan == nullptr || call.is_indirect || call.callee_value.has_value() ||
      call_plan->is_indirect || call_plan->indirect_callee.has_value() ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value()) {
    return std::nullopt;
  }
  switch (call_plan->wrapper_kind) {
    case prepare::PreparedCallWrapperKind::SameModule:
    case prepare::PreparedCallWrapperKind::DirectExternFixedArity:
      break;
    case prepare::PreparedCallWrapperKind::DirectExternVariadic:
    case prepare::PreparedCallWrapperKind::Indirect:
      return std::nullopt;
  }
  if (!call_plan->direct_callee_name.has_value() ||
      call_plan->direct_callee_name->empty()) {
    return std::nullopt;
  }
  if (!call.callee.empty() && call.callee != *call_plan->direct_callee_name) {
    return std::nullopt;
  }

  if (call.args.size() != call_plan->arguments.size()) {
    return std::nullopt;
  }

  RiscvEncodedFragment fragment;
  for (std::size_t arg_index = 0; arg_index < call_plan->arguments.size();
       ++arg_index) {
    const auto& argument = call_plan->arguments[arg_index];
    if (argument.arg_index != arg_index ||
        argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        argument.destination_contiguous_width != 1 ||
        !argument.destination_register_name.has_value()) {
      return std::nullopt;
    }
    const auto destination = rv64_register_number(*argument.destination_register_name);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
        argument.source_literal.has_value()) {
      const auto immediate =
          integer_immediate_for_value({}, nullptr, *argument.source_literal);
      if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
        return std::nullopt;
      }
      append_rv64_load_immediate(fragment, *destination, *immediate);
      continue;
    }
    if (argument.source_encoding == prepare::PreparedStorageEncodingKind::Register &&
        argument.source_register_bank == prepare::PreparedRegisterBank::Gpr &&
        argument.source_register_name.has_value()) {
      const auto source = rv64_register_number(*argument.source_register_name);
      if (!source.has_value()) {
        return std::nullopt;
      }
      append_rv64_move(fragment, *destination, *source);
      continue;
    }
    return std::nullopt;
  }

  const auto call_offset = fragment.bytes.size();
  append_le32(fragment.bytes, encode_u_type(0x17, 1, 0));       // auipc ra, 0
  append_le32(fragment.bytes, encode_i_type(0x67, 1, 0, 1, 0));  // jalr ra, 0(ra)
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = call_offset,
      .kind = RiscvObjectFixupKind::CallPlt,
      .symbol_name = *call_plan->direct_callee_name,
      .addend = 0,
  });

  if (call.result.has_value() != call_plan->result.has_value()) {
    return std::nullopt;
  }
  if (call_plan->result.has_value()) {
    const auto& result = *call_plan->result;
    if (result.value_bank != prepare::PreparedRegisterBank::Gpr ||
        result.source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        result.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        result.source_register_bank != prepare::PreparedRegisterBank::Gpr ||
        result.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        result.source_contiguous_width != 1 ||
        result.destination_contiguous_width != 1 ||
        !result.source_register_name.has_value() ||
        !result.destination_register_name.has_value()) {
      return std::nullopt;
    }
    const auto source = rv64_register_number(*result.source_register_name);
    const auto destination = rv64_register_number(*result.destination_register_name);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    append_rv64_move(fragment, *destination, *source);
  } else if (call.return_type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }

  return fragment;
}

const c4c::backend::prepare::PreparedInlineAsmCarrier* find_prepared_inline_asm_carrier(
    const c4c::backend::prepare::PreparedInlineAsmCarrierFunction* function_carriers,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == block_index && carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_return(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Terminator& terminator,
    bool restore_return_address,
    std::size_t stack_frame_bytes) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.return_lanes.empty()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  if (!terminator.value.has_value()) {
    if (restore_return_address) {
      append_rv64_call_frame_epilogue(fragment, stack_frame_bytes);
    } else {
      append_rv64_stack_frame_epilogue(fragment, stack_frame_bytes);
    }
    append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
    return fragment;
  }
  const auto immediate =
      integer_immediate_for_value(names, lookups, *terminator.value);
  if (immediate.has_value()) {
    if (!fits_signed_12_bit_immediate(*immediate)) {
      return std::nullopt;
    }
    append_rv64_load_immediate(fragment, 10, *immediate);
  } else {
    const auto source = gpr_register_number_for_value(names, lookups, *terminator.value);
    if (!source.has_value()) {
      return std::nullopt;
    }
    append_rv64_move(fragment, 10, *source);
  }
  if (restore_return_address) {
    append_rv64_call_frame_epilogue(fragment, stack_frame_bytes);
  } else {
    append_rv64_stack_frame_epilogue(fragment, stack_frame_bytes);
  }
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
}

const c4c::backend::prepare::PreparedMemoryAccess*
prepared_memory_access_for_instruction(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (lookups == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
}

std::optional<std::int32_t> prepared_frame_slot_absolute_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (access == nullptr || access->address_space != c4c::backend::bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind !=
          c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset || access->address.size_bytes != 4 ||
      access->address.align_bytes > 4 ||
      access->address.byte_offset < 0 ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  std::size_t slot_offset = 0;
  if (*access->address.frame_slot_id != 0 || !stack_layout.frame_slots.empty()) {
    const auto slot_it =
        std::find_if(stack_layout.frame_slots.begin(),
                     stack_layout.frame_slots.end(),
                     [&](const c4c::backend::prepare::PreparedFrameSlot& slot) {
                       return slot.slot_id == *access->address.frame_slot_id;
                     });
    if (slot_it == stack_layout.frame_slots.end()) {
      return std::nullopt;
    }
    slot_offset = slot_it->offset_bytes;
  }
  const auto offset =
      slot_offset + static_cast<std::size_t>(access->address.byte_offset);
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < 4 ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_store_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::StoreLocalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  const auto offset =
      prepared_frame_slot_absolute_offset(stack_layout, access, stack_frame_bytes);
  if (!offset.has_value()) {
    return std::nullopt;
  }
  const auto immediate = integer_immediate_for_value(names, lookups, store.value);
  if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  append_rv64_load_immediate(fragment, 6, *immediate);  // t1
  append_le32(fragment.bytes,
              encode_s_type(0x23,
                            2,
                            2,
                            6,
                            *offset));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_load_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  const auto offset =
      prepared_frame_slot_absolute_offset(stack_layout, access, stack_frame_bytes);
  if (!offset.has_value()) {
    return std::nullopt;
  }
  const auto destination = gpr_register_number_for_value(names, lookups, load.result);
  if (!destination.has_value()) {
    return std::nullopt;
  }
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x03,
                            *destination,
                            2,
                            2,
                            *offset));
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_binary(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary) {
  if (binary.result.type != c4c::backend::bir::TypeKind::I32 &&
      binary.result.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  const auto destination = gpr_register_number_for_value(names, lookups, binary.result);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  const auto lhs_register = gpr_register_number_for_value(names, lookups, binary.lhs);
  const auto rhs_register = gpr_register_number_for_value(names, lookups, binary.rhs);
  const auto lhs_immediate = integer_immediate_for_value(names, lookups, binary.lhs);
  const auto rhs_immediate = integer_immediate_for_value(names, lookups, binary.rhs);

  RiscvEncodedFragment fragment;
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      if (lhs_register.has_value() && rhs_register.has_value()) {
        append_le32(fragment.bytes,
                    encode_r_type(0x33,
                                  *destination,
                                  0,
                                  *lhs_register,
                                  *rhs_register,
                                  0));
        return fragment;
      }
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*rhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  *destination,
                                  0,
                                  *lhs_register,
                                  static_cast<std::int32_t>(*rhs_immediate)));
        return fragment;
      }
      if (rhs_register.has_value() && lhs_immediate.has_value() &&
          fits_signed_12_bit_immediate(*lhs_immediate)) {
        append_le32(fragment.bytes,
                    encode_i_type(0x13,
                                  *destination,
                                  0,
                                  *rhs_register,
                                  static_cast<std::int32_t>(*lhs_immediate)));
        return fragment;
      }
      return std::nullopt;
    case c4c::backend::bir::BinaryOpcode::Sub:
      if (lhs_register.has_value() && rhs_immediate.has_value() &&
          *rhs_immediate != std::numeric_limits<std::int64_t>::min()) {
        const auto negated = -*rhs_immediate;
        if (fits_signed_12_bit_immediate(negated)) {
          append_le32(fragment.bytes,
                      encode_i_type(0x13,
                                    *destination,
                                    0,
                                    *lhs_register,
                                    static_cast<std::int32_t>(negated)));
          return fragment;
        }
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

bool prepared_param_homes_supported(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function) {
  for (const auto& param : function.params) {
    const auto home =
        gpr_register_number_for_value(names,
                                      lookups,
                                      c4c::backend::bir::Value::named(param.type,
                                                                      param.name));
    if (!home.has_value()) {
      return false;
    }
  }
  return true;
}

const c4c::backend::bir::Function* find_defined_bir_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto it = std::find_if(
      prepared.module.functions.begin(),
      prepared.module.functions.end(),
      [&](const c4c::backend::bir::Function& candidate) {
        return !candidate.is_declaration && candidate.name == function_name;
      });
  return it == prepared.module.functions.end() ? nullptr : &*it;
}

std::optional<RiscvObjectFunction> prepared_function_to_object_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow) {
  namespace prepare = c4c::backend::prepare;

  const std::string function_name(
      prepare::prepared_function_name(prepared.names, control_flow.function_name));
  if (function_name.empty()) {
    return std::nullopt;
  }
  const auto* function = find_defined_bir_function(prepared, function_name);
  if (function == nullptr || function->is_variadic ||
      !function->atomic_operations.empty() ||
      function->blocks.size() != 1) {
    return std::nullopt;
  }
  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  if (!prepared_param_homes_supported(prepared.names, &lookups, *function)) {
    return std::nullopt;
  }
  const auto& block = function->blocks.front();
  RiscvObjectFunction object_function{
      .name = function_name,
      .global = true,
  };
  const auto* addressing = prepare::find_prepared_addressing(prepared, control_flow.function_name);
  const auto* inline_asm_carriers =
      prepare::find_prepared_inline_asm_carriers(prepared, control_flow.function_name);
  const auto stack_frame_bytes = rv64_object_stack_frame_size(addressing);
  if (!stack_frame_bytes.has_value()) {
    return std::nullopt;
  }

  const bool has_call = std::any_of(block.insts.begin(),
                                    block.insts.end(),
                                    [](const c4c::backend::bir::Inst& inst) {
                                      const auto* call = std::get_if<
                                          c4c::backend::bir::CallInst>(&inst);
                                      return call != nullptr &&
                                             !call->inline_asm.has_value();
                                    });
  if (has_call) {
    const auto call_frame_size = rv64_call_frame_size(*stack_frame_bytes);
    if (!fits_signed_12_bit_immediate(static_cast<std::int64_t>(call_frame_size)) ||
        !fits_signed_12_bit_immediate(rv64_call_frame_ra_offset(*stack_frame_bytes))) {
      return std::nullopt;
    }
    object_function.fragments.push_back(
        make_rv64_call_frame_prologue_fragment(*stack_frame_bytes));
  } else if (*stack_frame_bytes > 0) {
    object_function.fragments.push_back(
        make_rv64_stack_frame_prologue_fragment(*stack_frame_bytes));
  }

  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(
        &block.insts[instruction_index]);
    if (call == nullptr) {
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(
              &block.insts[instruction_index])) {
        auto fragment = fragment_for_prepared_binary(prepared.names, &lookups, *binary);
        if (fragment.has_value()) {
          object_function.fragments.push_back(std::move(*fragment));
          continue;
        }
      }
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(
              &block.insts[instruction_index])) {
        auto fragment = fragment_for_prepared_store_local(
            prepared.stack_layout,
            prepared.names,
            &lookups,
            *store,
            prepared_memory_access_for_instruction(
                &lookups,
                block.label_id,
                instruction_index),
            *stack_frame_bytes);
        if (fragment.has_value()) {
          object_function.fragments.push_back(std::move(*fragment));
          continue;
        }
      }
      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(
              &block.insts[instruction_index])) {
        auto fragment = fragment_for_prepared_load_local(
            prepared.stack_layout,
            prepared.names,
            &lookups,
            *load,
            prepared_memory_access_for_instruction(
                &lookups,
                block.label_id,
                instruction_index),
            *stack_frame_bytes);
        if (fragment.has_value()) {
          object_function.fragments.push_back(std::move(*fragment));
          continue;
        }
      }
      if (prepared_pure_instruction_is_rematerialized_immediate(
              prepared.names,
              &lookups,
              block.insts[instruction_index])) {
        continue;
      }
      return std::nullopt;
    }
    const auto* call_plan = prepare::find_indexed_prepared_call_plan(
        &lookups.call_plans,
        prepare::find_prepared_call_plans(prepared, control_flow.function_name),
        0,
        instruction_index);
    const auto* inline_asm_carrier =
        find_prepared_inline_asm_carrier(inline_asm_carriers, 0, instruction_index);
    auto fragment = fragment_for_prepared_call(call_plan, inline_asm_carrier, *call);
    if (!fragment.has_value()) {
      return std::nullopt;
    }
    object_function.fragments.push_back(std::move(*fragment));
  }

  auto return_fragment =
      fragment_for_prepared_return(prepared.names,
                                   &lookups,
                                   block.terminator,
                                   has_call,
                                   *stack_frame_bytes);
  if (!return_fragment.has_value()) {
    return std::nullopt;
  }
  object_function.fragments.push_back(std::move(*return_fragment));
  return object_function;
}

std::string_view strip_inline_asm_register_constraint(std::string_view constraint) {
  while (!constraint.empty()) {
    const char ch = constraint.front();
    if (ch == '=' || ch == '+' || ch == '&' || ch == '%') {
      constraint.remove_prefix(1);
      continue;
    }
    break;
  }
  return constraint;
}

bool is_decimal_inline_asm_tie(std::string_view constraint) {
  return !constraint.empty() &&
         std::all_of(constraint.begin(), constraint.end(), [](unsigned char ch) {
           return std::isdigit(ch) != 0;
         });
}

bool is_supported_rv64_substitution_register_constraint(std::string_view constraint) {
  constraint = strip_inline_asm_register_constraint(constraint);
  return constraint == "r" || constraint == "VR" || constraint == "VRM2" ||
         constraint == "VRM4";
}

const prepare::PreparedValueHome* substitution_home_for_operand(
    const prepare::PreparedInlineAsmCarrier& carrier,
    const prepare::PreparedInlineAsmOperand& operand) {
  switch (operand.kind) {
    case c4c::backend::bir::InlineAsmOperandKind::RegisterOutput:
      if (!is_supported_rv64_substitution_register_constraint(operand.constraint) ||
          !operand.output_index.has_value() || *operand.output_index != 0 ||
          !carrier.result_home.has_value()) {
        return nullptr;
      }
      return &*carrier.result_home;
    case c4c::backend::bir::InlineAsmOperandKind::RegisterInput:
      if (!is_supported_rv64_substitution_register_constraint(operand.constraint) ||
          !operand.arg_index.has_value() || !operand.home.has_value()) {
        return nullptr;
      }
      return &*operand.home;
    case c4c::backend::bir::InlineAsmOperandKind::TiedInput:
      if (!is_decimal_inline_asm_tie(operand.constraint) ||
          !operand.arg_index.has_value() || !operand.home.has_value()) {
        return nullptr;
      }
      return &*operand.home;
    case c4c::backend::bir::InlineAsmOperandKind::Unsupported:
    case c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput:
    case c4c::backend::bir::InlineAsmOperandKind::MemoryInput:
    case c4c::backend::bir::InlineAsmOperandKind::AddressInput:
    case c4c::backend::bir::InlineAsmOperandKind::Clobber:
      return nullptr;
  }
  return nullptr;
}

std::optional<std::string> register_name_for_inline_asm_substitution_home(
    const prepare::PreparedValueHome& home,
    c4c::backend::bir::InlineAsmRegisterClass register_class) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || !home.target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home.target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64) {
    return std::nullopt;
  }
  switch (register_class) {
    case c4c::backend::bir::InlineAsmRegisterClass::General:
      if (identity.bank != prepare::PreparedRegisterBank::Gpr ||
          identity.register_class != prepare::PreparedRegisterClass::General) {
        return std::nullopt;
      }
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::Vector:
      if (identity.bank != prepare::PreparedRegisterBank::Vreg ||
          identity.register_class != prepare::PreparedRegisterClass::Vector) {
        return std::nullopt;
      }
      break;
    case c4c::backend::bir::InlineAsmRegisterClass::None:
      break;
  }
  return *home.register_name;
}

std::optional<std::string> substitute_positional_riscv_inline_asm_operands(
    std::string_view text,
    const std::vector<std::optional<std::string>>& gcc_operand_registers) {
  std::string result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '%' || i + 1 >= text.size()) {
      result.push_back(text[i]);
      continue;
    }

    ++i;
    if (text[i] == '%') {
      result.push_back('%');
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
      std::size_t num = 0;
      while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
        num = num * 10 + static_cast<std::size_t>(text[i] - '0');
        ++i;
      }
      if (num >= gcc_operand_registers.size() || !gcc_operand_registers[num].has_value()) {
        return std::nullopt;
      }
      result += *gcc_operand_registers[num];
      --i;
      continue;
    }

    result.push_back('%');
    result.push_back(text[i]);
  }
  return result;
}

}  // namespace

std::optional<std::string> substitute_prepared_riscv_inline_asm_operands(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier) {
  namespace prepare = c4c::backend::prepare;
  if (carrier.carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      carrier.has_named_operand_references || carrier.has_template_modifiers ||
      !carrier.clobbers.empty() || !carrier.missing_required_facts.empty()) {
    return std::nullopt;
  }

  std::size_t operand_count = 0;
  for (const auto& operand : carrier.operands) {
    operand_count = std::max(operand_count, operand.constraint_index + 1);
  }

  std::vector<std::optional<std::string>> gcc_operand_registers(operand_count);
  for (const auto& operand : carrier.operands) {
    if (operand.constraint_index >= gcc_operand_registers.size() ||
        gcc_operand_registers[operand.constraint_index].has_value()) {
      return std::nullopt;
    }
    const auto* home = substitution_home_for_operand(carrier, operand);
    if (home == nullptr) {
      return std::nullopt;
    }
    auto register_name =
        register_name_for_inline_asm_substitution_home(*home, operand.register_class);
    if (!register_name.has_value()) {
      return std::nullopt;
    }
    gcc_operand_registers[operand.constraint_index] = std::move(register_name);
  }

  return substitute_positional_riscv_inline_asm_operands(
      carrier.asm_text,
      gcc_operand_registers);
}

std::optional<std::uint32_t> rv64_elf_relocation_type(
    RiscvObjectFixupKind kind) {
  switch (kind) {
    case RiscvObjectFixupKind::CallPlt:
      return kRiscvRelocCallPlt;
    case RiscvObjectFixupKind::PcrelHi20:
      return kRiscvRelocPcrelHi20;
    case RiscvObjectFixupKind::PcrelLo12I:
      return kRiscvRelocPcrelLo12I;
  }
  return std::nullopt;
}

object::RelocatableElfConfig rv64_relocatable_elf_config() {
  return object::RelocatableElfConfig{
      .elf_class = object::ElfClass::Elf64,
      .data_encoding = object::ElfDataEncoding::LittleEndian,
      .machine = kElfMachineRiscv,
      .flags = kRiscvElfFlagsRv64DoubleFloatAbi,
  };
}

RiscvEncodedFragment make_rv64_return_zero_fragment() {
  return make_rv64_return_immediate_fragment(0);
}

RiscvEncodedFragment make_rv64_direct_call_fragment(std::string callee_name) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes, encode_u_type(0x17, 1, 0));       // auipc ra, 0
  append_le32(fragment.bytes, encode_i_type(0x67, 1, 0, 1, 0));  // jalr ra, 0(ra)
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 0,
      .kind = RiscvObjectFixupKind::CallPlt,
      .symbol_name = std::move(callee_name),
      .addend = 0,
  });
  return fragment;
}

RiscvEncodedFragment make_rv64_pcrel_address_fragment(
    std::string symbol_name, std::string auipc_label_name) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes, encode_u_type(0x17, 5, 0));       // auipc t0, 0
  append_le32(fragment.bytes, encode_i_type(0x13, 5, 0, 5, 0));  // addi t0, t0, 0
  fragment.labels.push_back(RiscvObjectLabel{
      .offset_bytes = 0,
      .name = auipc_label_name,
  });
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 0,
      .kind = RiscvObjectFixupKind::PcrelHi20,
      .symbol_name = std::move(symbol_name),
      .addend = 0,
  });
  fragment.fixups.push_back(RiscvObjectFixup{
      .offset_bytes = 4,
      .kind = RiscvObjectFixupKind::PcrelLo12I,
      .symbol_name = std::move(auipc_label_name),
      .addend = 0,
  });
  return fragment;
}

std::optional<object::ObjectModule> build_rv64_text_object_module(
    const std::vector<RiscvObjectFunction>& functions) {
  object::ObjectModule module;
  auto& text = object::create_section(module,
                                      ".text",
                                      object::SectionKind::Text,
                                      4,
                                      true,
                                      true,
                                      false);

  std::unordered_map<std::string, object::SymbolId> symbols_by_name;
  for (const auto& function : functions) {
    if (function.name.empty()) {
      return std::nullopt;
    }
  }

  std::unordered_set<std::string> defined_function_names;
  for (const auto& function : functions) {
    if (!defined_function_names.insert(function.name).second) {
      return std::nullopt;
    }
  }

  for (const auto& function : functions) {
    const auto start_offset = text.size_bytes;
    for (const auto& fragment : function.fragments) {
      const auto fragment_offset = object::append_section_bytes(text, fragment.bytes);
      for (const auto& label : fragment.labels) {
        if (label.name.empty() || label.offset_bytes > fragment.bytes.size() ||
            symbols_by_name.find(label.name) != symbols_by_name.end()) {
          return std::nullopt;
        }
        const auto label_offset = fragment_offset + label.offset_bytes;
        object::bind_label(module, label.name, text.id, label_offset);
        auto& label_symbol = object::define_symbol(module,
                                                   label.name,
                                                   object::SymbolBinding::Local,
                                                   object::SymbolKind::NoType,
                                                   text.id,
                                                   label_offset,
                                                   0);
        symbols_by_name.emplace(label_symbol.name, label_symbol.id);
      }
      for (const auto& fixup : fragment.fixups) {
        const auto reloc_type = rv64_elf_relocation_type(fixup.kind);
        if (!reloc_type.has_value() || fixup.symbol_name.empty() ||
            fixup.offset_bytes > fragment.bytes.size()) {
          return std::nullopt;
        }
        object::SymbolId target_symbol{};
        const auto existing = symbols_by_name.find(fixup.symbol_name);
        if (existing != symbols_by_name.end()) {
          target_symbol = existing->second;
        } else {
          auto& undefined = object::declare_undefined_symbol(
              module,
              fixup.symbol_name,
              object::SymbolBinding::Global,
              object::SymbolKind::Function);
          target_symbol = undefined.id;
          symbols_by_name.emplace(undefined.name, undefined.id);
        }
        object::attach_relocation(module,
                                  text.id,
                                  fragment_offset + fixup.offset_bytes,
                                  *reloc_type,
                                  target_symbol,
                                  fixup.addend);
      }
    }
    auto& symbol = object::define_symbol(module,
                                         function.name,
                                         binding_for_function(function),
                                         object::SymbolKind::Function,
                                         text.id,
                                         start_offset,
                                         text.size_bytes - start_offset);
    symbols_by_name[symbol.name] = symbol.id;
  }

  return module;
}

std::optional<object::ObjectModule> build_rv64_prepared_text_object_module(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  if (!prepared.module.globals.empty() ||
      !prepared.module.string_constants.empty()) {
    return std::nullopt;
  }
  std::vector<RiscvObjectFunction> functions;
  functions.reserve(prepared.control_flow.functions.size());
  for (const auto& control_flow : prepared.control_flow.functions) {
    auto function = prepared_function_to_object_function(prepared, control_flow);
    if (!function.has_value()) {
      return std::nullopt;
    }
    functions.push_back(std::move(*function));
  }
  if (functions.empty()) {
    return std::nullopt;
  }
  return build_rv64_text_object_module(functions);
}

std::optional<object::RelocatableElfImage> write_rv64_relocatable_elf_object(
    const object::ObjectModule& module) {
  return object::write_relocatable_elf(module, rv64_relocatable_elf_config());
}

std::optional<object::RelocatableElfImage>
write_rv64_prepared_relocatable_elf_object(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  const auto module = build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return std::nullopt;
  }
  return write_rv64_relocatable_elf_object(*module);
}

}  // namespace c4c::backend::riscv::codegen
