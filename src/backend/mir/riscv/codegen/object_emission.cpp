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

object::SymbolBinding binding_for_function(const RiscvObjectFunction& function) {
  return function.global ? object::SymbolBinding::Global
                         : object::SymbolBinding::Local;
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

bool prepared_return_supported(const c4c::backend::bir::Terminator& terminator) {
  if (terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !terminator.return_lanes.empty()) {
    return false;
  }
  if (!terminator.value.has_value()) {
    return true;
  }
  return terminator.value->kind == c4c::backend::bir::Value::Kind::Immediate &&
         terminator.value->immediate == 0 &&
         (terminator.value->type == c4c::backend::bir::TypeKind::I32 ||
          terminator.value->type == c4c::backend::bir::TypeKind::I64);
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

  if (!prepared_return_supported(block.terminator)) {
    return std::nullopt;
  }
  object_function.fragments.push_back(make_rv64_return_zero_fragment());
  return object_function;
}

}  // namespace

std::optional<std::uint32_t> rv64_elf_relocation_type(
    RiscvObjectFixupKind kind) {
  switch (kind) {
    case RiscvObjectFixupKind::CallPlt:
      return kRiscvRelocCallPlt;
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
  RiscvEncodedFragment fragment;
  append_le32(fragment.bytes, encode_i_type(0x13, 10, 0, 0, 0));  // addi a0, zero, 0
  append_le32(fragment.bytes, encode_i_type(0x67, 0, 0, 1, 0));   // ret
  return fragment;
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
