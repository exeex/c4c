#include "src/backend/mir/object/model.hpp"
#include "src/backend/mir/riscv/codegen/object_emission.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/module.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace object = c4c::backend::mir::object;
namespace prepare = c4c::backend::prepare;
namespace rv64 = c4c::backend::riscv::codegen;

constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_SYMTAB = 2;

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset + shift / 8]) << shift;
  }
  return value;
}

std::string read_c_string(const std::vector<std::uint8_t>& bytes,
                          std::size_t offset) {
  std::string value;
  while (offset < bytes.size() && bytes[offset] != 0) {
    value.push_back(static_cast<char>(bytes[offset]));
    ++offset;
  }
  return value;
}

int fail(const std::string& message) {
  std::cerr << message << "\n";
  return 1;
}

std::optional<object::ObjectModule> make_minimal_call_module() {
  return rv64::build_rv64_text_object_module({
      rv64::RiscvObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              rv64::make_rv64_direct_call_fragment("callee"),
              rv64::make_rv64_return_zero_fragment(),
          },
      },
  });
}

bir::Function make_prepared_return_zero_function(std::string name) {
  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::immediate_i32(0);
  return bir::Function{
      .name = std::move(name),
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  };
}

prepare::PreparedBirModule make_prepared_direct_call_module() {
  prepare::PreparedBirModule prepared;
  const auto caller_name = prepared.names.function_names.intern("caller");
  const auto callee_name = prepared.names.function_names.intern("callee");

  bir::CallInst call;
  call.return_type = bir::TypeKind::Void;
  bir::Block caller_entry{
      .label = "entry",
      .insts = {call},
      .terminator = bir::Terminator{},
  };
  caller_entry.terminator.value = bir::Value::immediate_i32(0);
  prepared.module.functions.push_back(bir::Function{
      .name = "caller",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(caller_entry)},
  });
  prepared.module.functions.push_back(make_prepared_return_zero_function("callee"));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = caller_name,
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = callee_name,
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = caller_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
          .direct_callee_name = std::string{"callee"},
      }},
  });
  return prepared;
}

int records_minimal_text_and_call_relocation() {
  const auto module = make_minimal_call_module();
  if (!module.has_value()) {
    return fail("expected RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->bytes.size() != 16 || text->size_bytes != 16 ||
      !text->executable || text->writable) {
    return fail("expected executable .text section with four RV64 words");
  }
  if (text->bytes[0] != 0x97 || text->bytes[1] != 0x00 ||
      text->bytes[2] != 0x00 || text->bytes[3] != 0x00) {
    return fail("expected direct call fragment to start with auipc ra, 0");
  }
  if (text->bytes[4] != 0xe7 || text->bytes[5] != 0x80 ||
      text->bytes[6] != 0x00 || text->bytes[7] != 0x00) {
    return fail("expected direct call fragment to include jalr ra, 0(ra)");
  }

  const auto* caller = object::find_symbol(*module, "caller");
  const auto* callee = object::find_symbol(*module, "callee");
  if (caller == nullptr || caller->binding != object::SymbolBinding::Global ||
      caller->kind != object::SymbolKind::Function ||
      caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != 16) {
    return fail("expected defined global caller function symbol");
  }
  if (callee == nullptr || callee->binding != object::SymbolBinding::Global ||
      callee->kind != object::SymbolKind::Function ||
      !object::is_undefined_symbol(*callee)) {
    return fail("expected undefined global callee function symbol");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != 19 ||
      module->relocations[0].symbol != callee->id ||
      module->relocations[0].addend != 0) {
    return fail("expected R_RISCV_CALL_PLT relocation at the call pair");
  }
  return 0;
}

int records_same_module_direct_call_symbol() {
  const auto module = rv64::build_rv64_text_object_module({
      rv64::RiscvObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              rv64::make_rv64_direct_call_fragment("callee"),
              rv64::make_rv64_return_zero_fragment(),
          },
      },
      rv64::RiscvObjectFunction{
          .name = "callee",
          .global = true,
          .fragments = {
              rv64::make_rv64_return_zero_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected same-module RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "callee");
  if (text == nullptr || callee == nullptr ||
      callee->section != std::optional<object::SectionId>{text->id} ||
      callee->value != 16 || callee->size_bytes != 8 ||
      object::is_undefined_symbol(*callee)) {
    return fail("expected same-module callee to resolve as a defined function");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].symbol != callee->id ||
      module->relocations[0].type != 19) {
    return fail("expected same-module direct call to target the defined callee symbol");
  }
  return 0;
}

int builds_prepared_text_object_module_without_call_text() {
  const auto prepared = make_prepared_direct_call_module();
  const auto module = rv64::build_rv64_prepared_text_object_module(prepared);
  if (!module.has_value()) {
    return fail("expected prepared RV64 object module construction to succeed");
  }
  const auto* text = object::find_section(*module, ".text");
  const auto* caller = object::find_symbol(*module, "caller");
  const auto* callee = object::find_symbol(*module, "callee");
  if (text == nullptr || caller == nullptr || callee == nullptr) {
    return fail("expected prepared object module to publish .text and function symbols");
  }
  if (text->bytes.size() != 24 || text->size_bytes != 24) {
    return fail("expected prepared caller and callee text fragments");
  }
  if (caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != 16 ||
      callee->section != std::optional<object::SectionId>{text->id} ||
      callee->value != 16 || callee->size_bytes != 8) {
    return fail("expected prepared function symbols to use shared object helpers");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != 19 ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected prepared direct call to lower through R_RISCV_CALL_PLT");
  }
  return 0;
}

int rejects_prepared_data_without_asm_fallback() {
  auto prepared = make_prepared_direct_call_module();
  prepared.module.globals.push_back(bir::Global{
      .name = "g",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(1),
  });
  if (rv64::build_rv64_prepared_text_object_module(prepared).has_value()) {
    return fail("expected prepared RV64 object path to reject unsupported globals");
  }
  return 0;
}

int serializes_rv64_relocatable_elf_contract() {
  const auto module = make_minimal_call_module();
  if (!module.has_value()) {
    return fail("expected RV64 object module construction to succeed");
  }
  const auto image = rv64::write_rv64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected RV64 ELF writer to produce an image");
  }
  const auto& bytes = image->bytes;
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F') {
    return fail("expected ELF image magic");
  }
  if (bytes[4] != 2 || bytes[5] != 1 || read_u16(bytes, 16) != 1 ||
      read_u16(bytes, 18) != 243 || read_u32(bytes, 48) != 0x5) {
    return fail("expected ELF64 little-endian relocatable RV64 header");
  }

  const std::size_t shoff = read_u64(bytes, 40);
  const std::size_t shentsize = read_u16(bytes, 58);
  const std::size_t shnum = read_u16(bytes, 60);
  const std::size_t shstrndx = read_u16(bytes, 62);
  if (shoff == 0 || shentsize != 64 || shnum < 5 || shstrndx >= shnum) {
    return fail("expected valid section header table");
  }
  const std::size_t shstr_header = shoff + shstrndx * shentsize;
  const std::size_t shstr_offset = read_u64(bytes, shstr_header + 24);

  std::size_t rela_text_header = 0;
  std::size_t symtab_header = 0;
  std::size_t text_header = 0;
  for (std::size_t index = 1; index < shnum; ++index) {
    const std::size_t header = shoff + index * shentsize;
    const std::string name =
        read_c_string(bytes, shstr_offset + read_u32(bytes, header));
    if (name == ".text") {
      text_header = header;
    } else if (name == ".rela.text") {
      rela_text_header = header;
    } else if (name == ".symtab") {
      symtab_header = header;
    }
  }
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0) {
    return fail("expected .text, .rela.text, and .symtab sections");
  }
  if (read_u64(bytes, text_header + 32) != 16 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA ||
      read_u32(bytes, symtab_header + 4) != SHT_SYMTAB) {
    return fail("expected RV64 text and relocation section shapes");
  }

  const std::size_t rela_offset = read_u64(bytes, rela_text_header + 24);
  const std::size_t rela_size = read_u64(bytes, rela_text_header + 32);
  const std::size_t rela_entsize = read_u64(bytes, rela_text_header + 56);
  if (rela_size != 24 || rela_entsize != 24 || read_u64(bytes, rela_offset) != 0) {
    return fail("expected one call-pair relocation at text offset zero");
  }
  const std::uint64_t r_info = read_u64(bytes, rela_offset + 8);
  if ((r_info & 0xffffffffull) != 19) {
    return fail("expected serialized R_RISCV_CALL_PLT relocation type");
  }

  const std::size_t symtab_offset = read_u64(bytes, symtab_header + 24);
  const std::size_t symbol_index = r_info >> 32;
  const std::size_t symbol_offset = symtab_offset + symbol_index * 24;
  const std::size_t strtab_header =
      shoff + read_u32(bytes, symtab_header + 40) * shentsize;
  const std::size_t strtab_offset = read_u64(bytes, strtab_header + 24);
  const std::string symbol_name =
      read_c_string(bytes, strtab_offset + read_u32(bytes, symbol_offset));
  if (symbol_name != "callee" || read_u16(bytes, symbol_offset + 6) != 0) {
    return fail("expected relocation to reference undefined callee symbol");
  }
  return 0;
}

}  // namespace

int main() {
  int status = 0;
  status |= records_minimal_text_and_call_relocation();
  status |= records_same_module_direct_call_symbol();
  status |= builds_prepared_text_object_module_without_call_text();
  status |= rejects_prepared_data_without_asm_fallback();
  status |= serializes_rv64_relocatable_elf_contract();
  return status;
}
