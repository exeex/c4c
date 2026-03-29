// Mirror of ref/claudes-c-compiler/src/backend/linker_common/parse_object.rs.
#include "mod.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace c4c::backend::linker_common {
namespace {

constexpr std::size_t kElfHeaderSize = 64;
constexpr std::size_t kElfSectionHeaderSize = 64;
constexpr std::size_t kElfSymbolSize = 24;
constexpr std::size_t kElfRelaSize = 24;

bool range_fits(std::size_t offset, std::size_t size, std::size_t total) {
  return offset <= total && size <= total - offset;
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  return static_cast<std::uint16_t>(data[offset]) |
         (static_cast<std::uint16_t>(data[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  return static_cast<std::uint32_t>(data[offset]) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& data, std::size_t offset) {
  return static_cast<std::uint64_t>(data[offset]) |
         (static_cast<std::uint64_t>(data[offset + 1]) << 8) |
         (static_cast<std::uint64_t>(data[offset + 2]) << 16) |
         (static_cast<std::uint64_t>(data[offset + 3]) << 24) |
         (static_cast<std::uint64_t>(data[offset + 4]) << 32) |
         (static_cast<std::uint64_t>(data[offset + 5]) << 40) |
         (static_cast<std::uint64_t>(data[offset + 6]) << 48) |
         (static_cast<std::uint64_t>(data[offset + 7]) << 56);
}

std::int64_t read_i64(const std::vector<std::uint8_t>& data, std::size_t offset) {
  return static_cast<std::int64_t>(read_u64(data, offset));
}

std::string read_cstr(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if (offset >= data.size()) return {};
  std::size_t end = offset;
  while (end < data.size() && data[end] != 0) ++end;
  return std::string(data.begin() + static_cast<std::ptrdiff_t>(offset),
                     data.begin() + static_cast<std::ptrdiff_t>(end));
}

}  // namespace

std::optional<Elf64Object> parse_elf64_object(const std::vector<std::uint8_t>& data,
                                              const std::string& source_name,
                                              std::uint16_t expected_machine,
                                              std::string* error) {
  const auto fail = [&](const std::string& message) -> std::optional<Elf64Object> {
    if (error != nullptr) *error = message;
    return std::nullopt;
  };

  if (error != nullptr) error->clear();

  if (data.size() < kElfHeaderSize) {
    return fail(source_name + ": file too small for ELF header");
  }
  if (!std::equal(elf::ELF_MAGIC.begin(), elf::ELF_MAGIC.end(), data.begin())) {
    return fail(source_name + ": not an ELF file");
  }
  if (data[4] != elf::ELFCLASS64) {
    return fail(source_name + ": not 64-bit ELF");
  }
  if (data[5] != elf::ELFDATA2LSB) {
    return fail(source_name + ": not little-endian ELF");
  }

  const auto e_type = read_u16(data, 16);
  if (e_type != elf::ET_REL) {
    return fail(source_name + ": not a relocatable object (type=" + std::to_string(e_type) +
                ")");
  }

  if (expected_machine != 0) {
    const auto e_machine = read_u16(data, 18);
    if (e_machine != expected_machine) {
      return fail(source_name + ": wrong machine type (expected=" +
                  std::to_string(expected_machine) + ", got=" +
                  std::to_string(e_machine) + ")");
    }
  }

  const auto e_shoff = static_cast<std::size_t>(read_u64(data, 40));
  const auto e_shentsize = static_cast<std::size_t>(read_u16(data, 58));
  const auto e_shnum = static_cast<std::size_t>(read_u16(data, 60));
  const auto e_shstrndx = static_cast<std::size_t>(read_u16(data, 62));

  if (e_shoff == 0 || e_shnum == 0) {
    return fail(source_name + ": no section headers");
  }
  if (e_shentsize < kElfSectionHeaderSize) {
    return fail(source_name + ": unsupported section header size");
  }
  if (!range_fits(e_shoff, e_shentsize * e_shnum, data.size())) {
    return fail(source_name + ": section header table out of bounds");
  }

  std::vector<Elf64Section> sections;
  sections.reserve(e_shnum);
  for (std::size_t index = 0; index < e_shnum; ++index) {
    const auto offset = e_shoff + index * e_shentsize;
    sections.push_back(Elf64Section{
        .name_idx = read_u32(data, offset),
        .name = {},
        .sh_type = read_u32(data, offset + 4),
        .flags = read_u64(data, offset + 8),
        .addr = read_u64(data, offset + 16),
        .offset = read_u64(data, offset + 24),
        .size = read_u64(data, offset + 32),
        .link = read_u32(data, offset + 40),
        .info = read_u32(data, offset + 44),
        .addralign = read_u64(data, offset + 48),
        .entsize = read_u64(data, offset + 56),
    });
  }

  if (e_shstrndx < sections.size()) {
    const auto& shstrtab = sections[e_shstrndx];
    if (range_fits(static_cast<std::size_t>(shstrtab.offset),
                   static_cast<std::size_t>(shstrtab.size), data.size())) {
      const auto strtab_begin = data.begin() + static_cast<std::ptrdiff_t>(shstrtab.offset);
      const auto strtab_end = strtab_begin + static_cast<std::ptrdiff_t>(shstrtab.size);
      const std::vector<std::uint8_t> shstrtab_data(strtab_begin, strtab_end);
      for (auto& section : sections) {
        section.name = read_cstr(shstrtab_data, section.name_idx);
      }
    }
  }

  std::vector<std::vector<std::uint8_t>> section_data;
  section_data.reserve(sections.size());
  for (const auto& section : sections) {
    if (section.sh_type == elf::SHT_NOBITS || section.size == 0) {
      section_data.emplace_back();
      continue;
    }
    const auto start = static_cast<std::size_t>(section.offset);
    const auto size = static_cast<std::size_t>(section.size);
    if (!range_fits(start, size, data.size())) {
      return fail(source_name + ": section '" + section.name + "' data out of bounds");
    }
    section_data.emplace_back(data.begin() + static_cast<std::ptrdiff_t>(start),
                              data.begin() + static_cast<std::ptrdiff_t>(start + size));
  }

  std::vector<Elf64Symbol> symbols;
  for (std::size_t index = 0; index < sections.size(); ++index) {
    if (sections[index].sh_type != elf::SHT_SYMTAB) continue;

    const auto strtab_index = static_cast<std::size_t>(sections[index].link);
    if (strtab_index >= section_data.size()) {
      return fail(source_name + ": symbol string table index out of bounds");
    }

    const auto& strtab_data = section_data[strtab_index];
    const auto& symtab_data = section_data[index];
    const auto entry_size =
        sections[index].entsize == 0 ? kElfSymbolSize : static_cast<std::size_t>(sections[index].entsize);
    if (entry_size < kElfSymbolSize) {
      return fail(source_name + ": unsupported symbol entry size");
    }

    const auto symbol_count = symtab_data.size() / entry_size;
    symbols.reserve(symbol_count);
    for (std::size_t symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
      const auto offset = symbol_index * entry_size;
      const auto name_idx = read_u32(symtab_data, offset);
      auto name = read_cstr(strtab_data, name_idx);
      if (name.size() >= 4 && name.compare(name.size() - 4, 4, "@PLT") == 0) {
        name.resize(name.size() - 4);
      }
      symbols.push_back(Elf64Symbol{
          .name_idx = name_idx,
          .name = std::move(name),
          .info = symtab_data[offset + 4],
          .other = symtab_data[offset + 5],
          .shndx = read_u16(symtab_data, offset + 6),
          .value = read_u64(symtab_data, offset + 8),
          .size = read_u64(symtab_data, offset + 16),
      });
    }
    break;
  }

  std::vector<std::vector<Elf64Rela>> relocations(sections.size());
  for (std::size_t index = 0; index < sections.size(); ++index) {
    if (sections[index].sh_type != elf::SHT_RELA) continue;

    const auto target_section = static_cast<std::size_t>(sections[index].info);
    if (target_section >= relocations.size()) {
      return fail(source_name + ": relocation target section out of bounds");
    }

    const auto& rela_data = section_data[index];
    const auto entry_size =
        sections[index].entsize == 0 ? kElfRelaSize : static_cast<std::size_t>(sections[index].entsize);
    if (entry_size < kElfRelaSize) {
      return fail(source_name + ": unsupported relocation entry size");
    }

    const auto rela_count = rela_data.size() / entry_size;
    auto& relas = relocations[target_section];
    relas.reserve(rela_count);
    for (std::size_t rela_index = 0; rela_index < rela_count; ++rela_index) {
      const auto offset = rela_index * entry_size;
      const auto r_info = read_u64(rela_data, offset + 8);
      relas.push_back(Elf64Rela{
          .offset = read_u64(rela_data, offset),
          .sym_idx = static_cast<std::uint32_t>(r_info >> 32),
          .rela_type = static_cast<std::uint32_t>(r_info & 0xffffffffu),
          .addend = read_i64(rela_data, offset + 16),
      });
    }
  }

  return Elf64Object{
      .sections = std::move(sections),
      .symbols = std::move(symbols),
      .section_data = std::move(section_data),
      .relocations = std::move(relocations),
      .source_name = source_name,
  };
}

}  // namespace c4c::backend::linker_common
