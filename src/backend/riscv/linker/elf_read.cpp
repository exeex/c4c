// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/elf_read.rs
// ELF64 parser boundary for the RISC-V linker.
//
// The reusable ELF/archive parsing lives in the shared `elf` and
// `linker_common` modules. The RISC-V linker-specific wrappers stay local to
// this translation unit so they can be translated independently from the
// phase-1 input loader.

#include "../../elf/mod.hpp"
#include "../../linker_common/mod.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {
namespace {

using ElfObject = linker_common::Elf64Object;
using Symbol = linker_common::Elf64Symbol;

struct DynSymbol {
  std::string name;
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t info = 0;
  std::optional<std::string> version;
  std::optional<std::string> from_lib;
};

bool starts_with_bytes(const std::vector<std::uint8_t>& data,
                       std::string_view magic) {
  return data.size() >= magic.size() &&
         std::equal(magic.begin(), magic.end(), data.begin());
}

bool is_thin_archive(const std::vector<std::uint8_t>& data) {
  return starts_with_bytes(data, "!<thin>\n");
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return {};
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

std::string trim_right_spaces(std::string_view value) {
  std::size_t end = value.size();
  while (end > 0 && value[end - 1] == ' ') {
    --end;
  }
  return std::string(value.substr(0, end));
}

std::vector<std::string> parse_thin_archive_members(const std::vector<std::uint8_t>& data) {
  if (!is_thin_archive(data)) return {};

  std::vector<std::string> members;
  std::size_t pos = 8;
  std::optional<std::vector<std::uint8_t>> extended_names;

  while (pos + 60 <= data.size()) {
    const std::string name_raw(
        data.begin() + static_cast<std::ptrdiff_t>(pos),
        data.begin() + static_cast<std::ptrdiff_t>(pos + 16));
    const std::string size_text(
        data.begin() + static_cast<std::ptrdiff_t>(pos + 48),
        data.begin() + static_cast<std::ptrdiff_t>(pos + 58));
    if (data[pos + 58] != '`' || data[pos + 59] != '\n') {
      break;
    }

    std::size_t member_size = 0;
    try {
      member_size = static_cast<std::size_t>(std::stoull(trim_right_spaces(size_text)));
    } catch (const std::exception&) {
      member_size = 0;
    }

    const std::size_t data_start = pos + 60;
    std::string name_str = trim_right_spaces(name_raw);

    if (name_str == "/" || name_str == "/SYM64/") {
      pos = data_start + member_size;
      if ((pos & 1u) != 0u) ++pos;
      continue;
    }
    if (name_str == "//") {
      const std::size_t data_end = std::min(data_start + member_size, data.size());
      extended_names.emplace(data.begin() + static_cast<std::ptrdiff_t>(data_start),
                             data.begin() + static_cast<std::ptrdiff_t>(data_end));
      pos = data_start + member_size;
      if ((pos & 1u) != 0u) ++pos;
      continue;
    }

    if (!name_str.empty() && name_str.front() == '/') {
      const std::string rest = name_str.substr(1);
      const std::string digits = trim_right_spaces(rest);
      if (!digits.empty() &&
          std::all_of(digits.begin(), digits.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
          }) &&
          extended_names.has_value()) {
        const std::size_t name_off = static_cast<std::size_t>(std::stoull(digits));
        if (name_off < extended_names->size()) {
          const auto& ext = *extended_names;
          const auto begin = ext.begin() + static_cast<std::ptrdiff_t>(name_off);
          const auto end = std::find_if(begin, ext.end(), [](std::uint8_t ch) {
            return ch == '/' || ch == '\n' || ch == 0;
          });
          name_str = std::string(begin, end);
        }
      }
    } else if (!name_str.empty() && name_str.back() == '/') {
      name_str.pop_back();
    }

    members.push_back(std::move(name_str));
    pos = data_start + member_size;
    if ((pos & 1u) != 0u) ++pos;
  }

  return members;
}

std::optional<ElfObject> parse_object(const std::vector<std::uint8_t>& data,
                                      const std::string& source_name) {
  return linker_common::parse_elf64_object(data, source_name, 243);
}

std::vector<std::pair<std::string, ElfObject>> parse_archive(
    const std::vector<std::uint8_t>& data,
    const std::string& source_name) {
  std::vector<std::pair<std::string, ElfObject>> results;
  std::string archive_error;
  const auto members = elf::parse_archive_members(data, &archive_error);
  if (!members.has_value()) return results;

  for (const auto& member : *members) {
    const auto begin = data.begin() + static_cast<std::ptrdiff_t>(member.data_offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(member.data_size);
    const std::vector<std::uint8_t> member_data(begin, end);
    if (member_data.size() >= 4 && member_data[0] == 0x7f && member_data[1] == 'E' &&
        member_data[2] == 'L' && member_data[3] == 'F') {
      const std::string full_name = source_name + "(" + member.name + ")";
      if (auto object = parse_object(member_data, full_name)) {
        results.emplace_back(member.name, std::move(*object));
      }
    }
  }

  return results;
}

std::vector<std::pair<std::string, ElfObject>> parse_thin_archive(
    const std::vector<std::uint8_t>& data,
    const std::string& archive_path) {
  const std::vector<std::string> member_names = parse_thin_archive_members(data);
  const std::filesystem::path archive_dir =
      std::filesystem::path(archive_path).parent_path().empty()
          ? std::filesystem::path(".")
          : std::filesystem::path(archive_path).parent_path();

  std::vector<std::pair<std::string, ElfObject>> results;
  for (const std::string& member_name : member_names) {
    const std::filesystem::path member_path = archive_dir / member_name;
    const std::vector<std::uint8_t> member_data = read_file_bytes(member_path.string());
    if (member_data.size() >= 4 && member_data[0] == 0x7f && member_data[1] == 'E' &&
        member_data[2] == 'L' && member_data[3] == 'F') {
      const std::string full_name = archive_path + "(" + member_name + ")";
      if (auto object = parse_object(member_data, full_name)) {
        results.emplace_back(member_name, std::move(*object));
      }
    }
  }

  return results;
}

std::vector<DynSymbol> parse_shared_library_symbols(const std::vector<std::uint8_t>& data,
                                                    const std::string& lib_name) {
  (void)data;
  (void)lib_name;
  return {};
}

std::vector<DynSymbol> read_shared_lib_symbols(const std::string& path) {
  const std::vector<std::uint8_t> data = read_file_bytes(path);
  return parse_shared_library_symbols(data, path);
}

}  // namespace

using c4c::backend::elf::parse_archive_members;

// ELF constants re-exported for the RISC-V linker namespace.
inline constexpr std::uint16_t EM_RISCV = 243;
inline constexpr std::uint32_t SHT_PROGBITS = elf::SHT_PROGBITS;
inline constexpr std::uint32_t SHT_SYMTAB = elf::SHT_SYMTAB;
inline constexpr std::uint32_t SHT_STRTAB = elf::SHT_STRTAB;
inline constexpr std::uint32_t SHT_RELA = elf::SHT_RELA;
inline constexpr std::uint32_t SHT_NOBITS = elf::SHT_NOBITS;
inline constexpr std::uint32_t SHT_GROUP = 17;
inline constexpr std::uint64_t SHF_WRITE = 0x1;
inline constexpr std::uint64_t SHF_ALLOC = elf::SHF_ALLOC;
inline constexpr std::uint64_t SHF_EXECINSTR = elf::SHF_EXECINSTR;
inline constexpr std::uint64_t SHF_TLS = 0x400;
inline constexpr std::uint8_t STB_LOCAL = elf::STB_LOCAL;
inline constexpr std::uint8_t STB_GLOBAL = elf::STB_GLOBAL;
inline constexpr std::uint8_t STB_WEAK = elf::STB_WEAK;
inline constexpr std::uint8_t STT_NOTYPE = elf::STT_NOTYPE;
inline constexpr std::uint8_t STT_OBJECT = 1;
inline constexpr std::uint8_t STT_FUNC = elf::STT_FUNC;
inline constexpr std::uint8_t STT_SECTION = elf::STT_SECTION;
inline constexpr std::uint8_t STV_DEFAULT = 0;
inline constexpr std::uint16_t SHN_UNDEF = elf::SHN_UNDEF;
inline constexpr std::uint16_t SHN_ABS = 0xfff1;
inline constexpr std::uint16_t SHN_COMMON = 0xfff2;
inline constexpr std::uint32_t SHT_RISCV_ATTRIBUTES = 0x70000003;

using Symbol = linker_common::Elf64Symbol;
using ElfObject = linker_common::Elf64Object;

// The RISC-V linker's phase-1 helpers live in `input.cpp` for now so they can
// be shared with the rest of the linker pipeline without introducing another
// exported surface before the shared contract is ready.

}  // namespace c4c::backend::riscv::linker
