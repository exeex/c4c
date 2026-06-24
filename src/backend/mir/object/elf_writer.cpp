#include "mir/object/elf_writer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::mir::object {

namespace {

constexpr std::uint16_t ET_REL = 1;
constexpr std::uint32_t SHT_NULL = 0;
constexpr std::uint32_t SHT_PROGBITS = 1;
constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t SHT_STRTAB = 3;
constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_NOBITS = 8;
constexpr std::uint64_t SHF_WRITE = 0x1;
constexpr std::uint64_t SHF_ALLOC = 0x2;
constexpr std::uint64_t SHF_EXECINSTR = 0x4;
constexpr std::uint64_t SHF_INFO_LINK = 0x40;
constexpr std::uint8_t STB_LOCAL = 0;
constexpr std::uint8_t STB_GLOBAL = 1;
constexpr std::uint8_t STT_NOTYPE = 0;
constexpr std::uint8_t STT_OBJECT = 1;
constexpr std::uint8_t STT_FUNC = 2;
constexpr std::uint8_t STT_SECTION = 3;
constexpr std::uint16_t SHN_UNDEF = 0;
constexpr std::uint16_t EV_CURRENT = 1;
constexpr std::uint16_t ELF_HEADER_SIZE = 64;
constexpr std::uint16_t ELF64_SHENTSIZE = 64;
constexpr std::uint16_t ELF64_SYMENTSIZE = 24;
constexpr std::uint64_t ELF64_RELAENTSIZE = 24;

struct StringTable {
  std::vector<std::uint8_t> bytes{0};

  std::uint32_t add(const std::string& value) {
    const auto offset = static_cast<std::uint32_t>(bytes.size());
    bytes.insert(bytes.end(), value.begin(), value.end());
    bytes.push_back(0);
    return offset;
  }
};

struct SectionHeader {
  std::uint32_t name = 0;
  std::uint32_t type = SHT_NULL;
  std::uint64_t flags = 0;
  std::uint64_t addr = 0;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint32_t link = 0;
  std::uint32_t info = 0;
  std::uint64_t addralign = 0;
  std::uint64_t entsize = 0;
};

struct RelocationGroup {
  SectionId target_section{};
  std::string section_name;
  std::vector<const RelocationRecord*> relocations;
};

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
  }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
  }
}

void append_i64(std::vector<std::uint8_t>& out, std::int64_t value) {
  append_u64(out, static_cast<std::uint64_t>(value));
}

std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

void pad_to(std::vector<std::uint8_t>& out, std::uint64_t offset) {
  if (out.size() < offset) {
    out.insert(out.end(), static_cast<std::size_t>(offset - out.size()), 0);
  }
}

std::uint64_t section_flags(const SectionRecord& section) {
  std::uint64_t flags = 0;
  if (section.writable) {
    flags |= SHF_WRITE;
  }
  if (section.alloc) {
    flags |= SHF_ALLOC;
  }
  if (section.executable) {
    flags |= SHF_EXECINSTR;
  }
  return flags;
}

std::uint32_t section_type(const SectionRecord& section) {
  return section.kind == SectionKind::Bss ? SHT_NOBITS : SHT_PROGBITS;
}

std::uint8_t symbol_binding(SymbolBinding binding) {
  return binding == SymbolBinding::Global ? STB_GLOBAL : STB_LOCAL;
}

std::uint8_t symbol_type(SymbolKind kind) {
  switch (kind) {
    case SymbolKind::Function:
      return STT_FUNC;
    case SymbolKind::Object:
      return STT_OBJECT;
    case SymbolKind::Section:
      return STT_SECTION;
    case SymbolKind::NoType:
      return STT_NOTYPE;
  }
  return STT_NOTYPE;
}

std::uint8_t symbol_info(const SymbolRecord& symbol) {
  return static_cast<std::uint8_t>((symbol_binding(symbol.binding) << 4) |
                                   symbol_type(symbol.kind));
}

bool is_supported_config(const RelocatableElfConfig& config) {
  return config.elf_class == ElfClass::Elf64 &&
         config.data_encoding == ElfDataEncoding::LittleEndian;
}

std::vector<const SymbolRecord*> ordered_symbols(const ObjectModule& module) {
  std::vector<const SymbolRecord*> symbols;
  symbols.reserve(module.symbols.size());
  for (const auto& symbol : module.symbols) {
    if (symbol.binding == SymbolBinding::Local) {
      symbols.push_back(&symbol);
    }
  }
  for (const auto& symbol : module.symbols) {
    if (symbol.binding == SymbolBinding::Global) {
      symbols.push_back(&symbol);
    }
  }
  return symbols;
}

std::vector<RelocationGroup> group_relocations(const ObjectModule& module) {
  std::map<std::size_t, RelocationGroup> groups;
  for (const auto& relocation : module.relocations) {
    const auto* target = find_section(module, relocation.section);
    if (target == nullptr) {
      continue;
    }
    auto& group = groups[relocation.section.value];
    group.target_section = relocation.section;
    group.section_name = ".rela" + target->name;
    group.relocations.push_back(&relocation);
  }

  std::vector<RelocationGroup> ordered;
  ordered.reserve(groups.size());
  for (auto& entry : groups) {
    ordered.push_back(std::move(entry.second));
  }
  return ordered;
}

std::vector<std::uint8_t> encode_symbol_table(
    const ObjectModule& module, const StringTable& strtab,
    const std::vector<const SymbolRecord*>& symbols,
    const std::unordered_map<std::size_t, std::uint32_t>& section_indices) {
  std::unordered_map<std::string, std::uint32_t> name_offsets;
  std::uint32_t offset = 1;
  for (std::size_t i = 1; i < strtab.bytes.size();) {
    const std::string name(reinterpret_cast<const char*>(&strtab.bytes[i]));
    name_offsets.emplace(name, static_cast<std::uint32_t>(i));
    i += name.size() + 1;
    offset = static_cast<std::uint32_t>(i);
  }
  (void)offset;

  std::vector<std::uint8_t> bytes;
  bytes.insert(bytes.end(), ELF64_SYMENTSIZE, 0);
  for (const auto* symbol : symbols) {
    const auto name_it = name_offsets.find(symbol->name);
    const auto name = name_it == name_offsets.end() ? 0 : name_it->second;
    const auto shndx =
        symbol->section.has_value()
            ? static_cast<std::uint16_t>(
                  section_indices.at(symbol->section->value))
            : SHN_UNDEF;
    append_u32(bytes, name);
    bytes.push_back(symbol_info(*symbol));
    bytes.push_back(0);
    append_u16(bytes, shndx);
    append_u64(bytes, symbol->value);
    append_u64(bytes, symbol->size_bytes);
  }
  (void)module;
  return bytes;
}

void append_section_header(std::vector<std::uint8_t>& out,
                           const SectionHeader& header) {
  append_u32(out, header.name);
  append_u32(out, header.type);
  append_u64(out, header.flags);
  append_u64(out, header.addr);
  append_u64(out, header.offset);
  append_u64(out, header.size);
  append_u32(out, header.link);
  append_u32(out, header.info);
  append_u64(out, header.addralign);
  append_u64(out, header.entsize);
}

std::int64_t relocation_addend(const ObjectModule& module,
                               const RelocationRecord& relocation) {
  if (relocation.base == RelocationBase::Label && relocation.label.has_value()) {
    if (relocation.label->value < module.labels.size()) {
      return relocation.addend +
             static_cast<std::int64_t>(module.labels[relocation.label->value].offset);
    }
  }
  return relocation.addend;
}

}  // namespace

std::optional<RelocatableElfImage> write_relocatable_elf(
    const ObjectModule& module, const RelocatableElfConfig& config) {
  if (!is_supported_config(config)) {
    return std::nullopt;
  }

  StringTable shstrtab;
  StringTable strtab;
  std::vector<SectionHeader> headers(1);
  std::unordered_map<std::size_t, std::uint32_t> section_indices;

  for (const auto& section : module.sections) {
    const auto index = static_cast<std::uint32_t>(headers.size());
    section_indices.emplace(section.id.value, index);
    headers.push_back(SectionHeader{
        .name = shstrtab.add(section.name),
        .type = section_type(section),
        .flags = section_flags(section),
        .size = section.size_bytes,
        .addralign = std::max<std::uint64_t>(1, section.align_bytes),
    });
  }

  const auto relocation_groups = group_relocations(module);
  const auto symbols = ordered_symbols(module);
  for (const auto* symbol : symbols) {
    strtab.add(symbol->name);
  }
  const auto local_symbol_count =
      static_cast<std::uint32_t>(1 + std::count_if(
                                         symbols.begin(), symbols.end(),
                                         [](const SymbolRecord* symbol) {
                                           return symbol->binding ==
                                                  SymbolBinding::Local;
                                         }));
  std::unordered_map<std::size_t, std::uint32_t> symbol_indices;
  for (std::size_t i = 0; i < symbols.size(); ++i) {
    symbol_indices.emplace(symbols[i]->id.value, static_cast<std::uint32_t>(i + 1));
  }

  std::vector<std::vector<std::uint8_t>> relocation_bytes;
  relocation_bytes.reserve(relocation_groups.size());
  for (const auto& group : relocation_groups) {
    std::vector<std::uint8_t> bytes;
    for (const auto* relocation : group.relocations) {
      const auto symbol_it = symbol_indices.find(relocation->symbol.value);
      if (symbol_it == symbol_indices.end()) {
        return std::nullopt;
      }
      append_u64(bytes, relocation->offset);
      append_u64(bytes, (static_cast<std::uint64_t>(symbol_it->second) << 32u) |
                            relocation->type);
      append_i64(bytes, relocation_addend(module, *relocation));
    }
    relocation_bytes.push_back(std::move(bytes));
  }

  const auto symtab_bytes =
      encode_symbol_table(module, strtab, symbols, section_indices);

  const auto first_rela_index = static_cast<std::uint32_t>(headers.size());
  for (std::size_t i = 0; i < relocation_groups.size(); ++i) {
    headers.push_back(SectionHeader{
        .name = shstrtab.add(relocation_groups[i].section_name),
        .type = SHT_RELA,
        .flags = SHF_INFO_LINK,
        .size = static_cast<std::uint64_t>(relocation_bytes[i].size()),
        .link = 0,
        .info = section_indices.at(relocation_groups[i].target_section.value),
        .addralign = 8,
        .entsize = ELF64_RELAENTSIZE,
    });
  }

  const auto symtab_index = static_cast<std::uint32_t>(headers.size());
  headers.push_back(SectionHeader{
      .name = shstrtab.add(".symtab"),
      .type = SHT_SYMTAB,
      .size = static_cast<std::uint64_t>(symtab_bytes.size()),
      .link = 0,
      .info = local_symbol_count,
      .addralign = 8,
      .entsize = ELF64_SYMENTSIZE,
  });
  const auto strtab_index = static_cast<std::uint32_t>(headers.size());
  headers.push_back(SectionHeader{
      .name = shstrtab.add(".strtab"),
      .type = SHT_STRTAB,
      .size = static_cast<std::uint64_t>(strtab.bytes.size()),
      .addralign = 1,
  });
  const auto shstrtab_index = static_cast<std::uint32_t>(headers.size());
  headers.push_back(SectionHeader{
      .name = shstrtab.add(".shstrtab"),
      .type = SHT_STRTAB,
      .size = static_cast<std::uint64_t>(shstrtab.bytes.size()),
      .addralign = 1,
  });

  headers[symtab_index].link = strtab_index;
  for (std::uint32_t i = 0; i < relocation_groups.size(); ++i) {
    headers[first_rela_index + i].link = symtab_index;
  }

  std::vector<std::uint8_t> out;
  out.reserve(ELF_HEADER_SIZE + symtab_bytes.size() + strtab.bytes.size() +
              shstrtab.bytes.size() + headers.size() * ELF64_SHENTSIZE);
  out.insert(out.end(), ELF_HEADER_SIZE, 0);

  for (const auto& section : module.sections) {
    auto& header = headers[section_indices.at(section.id.value)];
    const auto offset = align_up(out.size(), header.addralign);
    header.offset = offset;
    if (header.type == SHT_NOBITS) {
      continue;
    }
    pad_to(out, offset);
    const auto bytes_size =
        std::min<std::uint64_t>(section.bytes.size(), section.size_bytes);
    out.insert(out.end(), section.bytes.begin(),
               section.bytes.begin() + static_cast<std::ptrdiff_t>(bytes_size));
    if (bytes_size < section.size_bytes) {
      out.insert(out.end(),
                 static_cast<std::size_t>(section.size_bytes - bytes_size), 0);
    }
  }

  for (std::size_t i = 0; i < relocation_bytes.size(); ++i) {
    auto& header = headers[first_rela_index + i];
    const auto offset = align_up(out.size(), header.addralign);
    header.offset = offset;
    pad_to(out, offset);
    out.insert(out.end(), relocation_bytes[i].begin(), relocation_bytes[i].end());
  }

  headers[symtab_index].offset = align_up(out.size(), headers[symtab_index].addralign);
  pad_to(out, headers[symtab_index].offset);
  out.insert(out.end(), symtab_bytes.begin(), symtab_bytes.end());

  headers[strtab_index].offset = align_up(out.size(), 1);
  out.insert(out.end(), strtab.bytes.begin(), strtab.bytes.end());

  headers[shstrtab_index].size = static_cast<std::uint64_t>(shstrtab.bytes.size());
  headers[shstrtab_index].offset = align_up(out.size(), 1);
  out.insert(out.end(), shstrtab.bytes.begin(), shstrtab.bytes.end());

  const auto section_header_offset = align_up(out.size(), 8);
  pad_to(out, section_header_offset);
  for (const auto& header : headers) {
    append_section_header(out, header);
  }

  std::vector<std::uint8_t> header;
  header.reserve(ELF_HEADER_SIZE);
  header.push_back(0x7f);
  header.push_back('E');
  header.push_back('L');
  header.push_back('F');
  header.push_back(2);
  header.push_back(1);
  header.push_back(1);
  header.push_back(0);
  header.insert(header.end(), 8, 0);
  append_u16(header, ET_REL);
  append_u16(header, config.machine);
  append_u32(header, EV_CURRENT);
  append_u64(header, 0);
  append_u64(header, 0);
  append_u64(header, section_header_offset);
  append_u32(header, config.flags);
  append_u16(header, ELF_HEADER_SIZE);
  append_u16(header, 0);
  append_u16(header, 0);
  append_u16(header, ELF64_SHENTSIZE);
  append_u16(header, static_cast<std::uint16_t>(headers.size()));
  append_u16(header, static_cast<std::uint16_t>(shstrtab_index));
  std::copy(header.begin(), header.end(), out.begin());

  return RelocatableElfImage{.bytes = std::move(out)};
}

}  // namespace c4c::backend::mir::object
