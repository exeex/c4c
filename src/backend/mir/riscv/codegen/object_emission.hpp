#pragma once

#include "mir/object/elf_writer.hpp"
#include "mir/object/model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::riscv::codegen {

enum class RiscvObjectFixupKind {
  CallPlt,
};

struct RiscvObjectFixup {
  std::uint64_t offset_bytes = 0;
  RiscvObjectFixupKind kind = RiscvObjectFixupKind::CallPlt;
  std::string symbol_name;
  std::int64_t addend = 0;
};

struct RiscvEncodedFragment {
  std::vector<std::uint8_t> bytes;
  std::vector<RiscvObjectFixup> fixups;
};

struct RiscvObjectFunction {
  std::string name;
  bool global = true;
  std::vector<RiscvEncodedFragment> fragments;
};

[[nodiscard]] std::optional<std::uint32_t> rv64_elf_relocation_type(
    RiscvObjectFixupKind kind);

[[nodiscard]] c4c::backend::mir::object::RelocatableElfConfig
rv64_relocatable_elf_config();

[[nodiscard]] RiscvEncodedFragment make_rv64_return_zero_fragment();

[[nodiscard]] RiscvEncodedFragment make_rv64_direct_call_fragment(
    std::string callee_name);

[[nodiscard]] std::optional<c4c::backend::mir::object::ObjectModule>
build_rv64_text_object_module(const std::vector<RiscvObjectFunction>& functions);

[[nodiscard]] std::optional<c4c::backend::mir::object::RelocatableElfImage>
write_rv64_relocatable_elf_object(
    const c4c::backend::mir::object::ObjectModule& module);

}  // namespace c4c::backend::riscv::codegen
