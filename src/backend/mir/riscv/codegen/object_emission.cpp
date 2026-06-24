#include "object_emission.hpp"

#include "../../../prealloc/prepared_lookups.hpp"

#include <algorithm>
#include <cstdint>
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

RiscvEncodedFragment make_rv64_return_immediate_fragment(std::int64_t immediate) {
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes,
              encode_i_type(0x13, 10, 0, 0, static_cast<std::int32_t>(immediate)));
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));  // ret
  return fragment;
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
    const c4c::backend::bir::CallInst& call) {
  namespace prepare = c4c::backend::prepare;

  if (call_plan == nullptr || call.is_indirect || call.callee_value.has_value() ||
      !call.args.empty() || call.result.has_value() ||
      call.return_type != c4c::backend::bir::TypeKind::Void ||
      call_plan->is_indirect || call_plan->indirect_callee.has_value() ||
      call_plan->memory_return.has_value() ||
      call_plan->outgoing_stack_argument_area.has_value() ||
      !call_plan->arguments.empty() || call_plan->result.has_value()) {
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
  return make_rv64_direct_call_fragment(*call_plan->direct_callee_name);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_return(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Terminator& terminator) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.return_lanes.empty()) {
    return std::nullopt;
  }
  if (!terminator.value.has_value()) {
    return make_rv64_return_immediate_fragment(0);
  }
  const auto immediate =
      integer_immediate_for_value(names, lookups, *terminator.value);
  if (!immediate.has_value() || !fits_signed_12_bit_immediate(*immediate)) {
    return std::nullopt;
  }
  return make_rv64_return_immediate_fragment(*immediate);
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
  if (function == nullptr || function->is_variadic || !function->params.empty() ||
      !function->local_slots.empty() || !function->atomic_operations.empty() ||
      function->blocks.size() != 1) {
    return std::nullopt;
  }
  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto& block = function->blocks.front();
  RiscvObjectFunction object_function{
      .name = function_name,
      .global = true,
  };

  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(
        &block.insts[instruction_index]);
    if (call == nullptr) {
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
    auto fragment = fragment_for_prepared_call(call_plan, *call);
    if (!fragment.has_value()) {
      return std::nullopt;
    }
    object_function.fragments.push_back(std::move(*fragment));
  }

  auto return_fragment =
      fragment_for_prepared_return(prepared.names, &lookups, block.terminator);
  if (!return_fragment.has_value()) {
    return std::nullopt;
  }
  object_function.fragments.push_back(std::move(*return_fragment));
  return object_function;
}

}  // namespace

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
