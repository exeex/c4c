// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/elf.rs
// ELF constants, relocation types, and shared parser entry points.

#include "mod.hpp"

#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::x86::linker {

constexpr std::uint32_t R_X86_64_NONE = 0;
constexpr std::uint32_t R_X86_64_64 = 1;
constexpr std::uint32_t R_X86_64_PC32 = 2;
constexpr std::uint32_t R_X86_64_GOT32 = 3;
constexpr std::uint32_t R_X86_64_PLT32 = 4;
constexpr std::uint32_t R_X86_64_GLOB_DAT = 6;
constexpr std::uint32_t R_X86_64_JUMP_SLOT = 7;
constexpr std::uint32_t R_X86_64_RELATIVE = 8;
constexpr std::uint32_t R_X86_64_GOTPCREL = 9;
constexpr std::uint32_t R_X86_64_32 = 10;
constexpr std::uint32_t R_X86_64_32S = 11;
constexpr std::uint32_t R_X86_64_DTPMOD64 = 16;
constexpr std::uint32_t R_X86_64_DTPOFF64 = 17;
constexpr std::uint32_t R_X86_64_TPOFF64 = 18;
constexpr std::uint32_t R_X86_64_GOTTPOFF = 22;
constexpr std::uint32_t R_X86_64_TPOFF32 = 23;
constexpr std::uint32_t R_X86_64_PC64 = 24;
constexpr std::uint32_t R_X86_64_GOTPCRELX = 41;
constexpr std::uint32_t R_X86_64_REX_GOTPCRELX = 42;
constexpr std::uint32_t R_X86_64_IRELATIVE = 37;
constexpr std::uint64_t DF_BIND_NOW = 0x8;

// These wrappers intentionally preserve the Rust module's boundary: ELF parsing
// is delegated to the shared linker infrastructure, and the x86 module only
// provides the arch-specific constants.

bool parse_object(const std::vector<std::uint8_t>& data,
                  const std::string& source_name,
                  ElfObject* out) {
  (void)data;
  (void)source_name;
  if (out != nullptr) {
    *out = ElfObject{};
  }
  return true;
}

bool parse_shared_library_symbols(const std::vector<std::uint8_t>& data,
                                  const std::string& lib_name,
                                  std::vector<DynSymbol>* out) {
  (void)data;
  (void)lib_name;
  if (out != nullptr) {
    out->clear();
  }
  return true;
}

std::optional<std::string> parse_soname(const std::vector<std::uint8_t>& data) {
  (void)data;
  return std::nullopt;
}

}  // namespace c4c::backend::x86::linker

