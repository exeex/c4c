#include "parser.hpp"
#include "encoder/mod.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

// x86-64 ELF relocatable object writer adaptor.

namespace c4c::backend::x86::assembler {

static constexpr std::uint32_t ET_REL = 1;
static constexpr std::uint16_t EM_X86_64 = 62;
static constexpr std::uint32_t SHT_NULL = 0;
static constexpr std::uint32_t SHT_PROGBITS = 1;
static constexpr std::uint32_t SHT_RELA = 4;
static constexpr std::uint32_t SHT_SYMTAB = 2;
static constexpr std::uint32_t SHT_STRTAB = 3;
static constexpr std::uint64_t SHF_ALLOC = 0x2;
static constexpr std::uint64_t SHF_EXECINSTR = 0x4;
static constexpr std::uint8_t STB_LOCAL = 0;
static constexpr std::uint8_t STB_GLOBAL = 1;
static constexpr std::uint8_t STT_NOTYPE = 0;
static constexpr std::uint8_t STT_FUNC = 2;
static constexpr std::uint8_t STT_SECTION = 3;

namespace {

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
  }
}

void append_i64(std::vector<std::uint8_t>& out, std::int64_t value) {
  append_u64(out, static_cast<std::uint64_t>(value));
}

void append_zeroes(std::vector<std::uint8_t>& out, std::size_t count) {
  out.insert(out.end(), count, 0);
}

std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) return value;
  const auto mask = alignment - 1;
  return (value + mask) & ~mask;
}

std::uint8_t symbol_info(std::uint8_t binding, std::uint8_t symbol_type) {
  return static_cast<std::uint8_t>((binding << 4) | symbol_type);
}

struct MinimalObjectSlice {
  std::string symbol;
  std::vector<std::uint8_t> text_bytes;
  std::vector<encoder::EncodedRelocation> relocations;
};

std::optional<MinimalObjectSlice> parse_minimal_text_slice(
    const std::vector<AsmStatement>& statements) {
  bool in_text = false;
  std::string global_symbol;
  std::string active_label;

  for (const auto& statement : statements) {
    if (statement.kind == AsmStatementKind::Directive) {
      if (statement.op == ".intel_syntax noprefix") continue;
      if (statement.op == ".text") {
        in_text = true;
        continue;
      }
      if ((statement.op == ".globl" || statement.op == ".global") &&
          !statement.raw_operands.empty()) {
        global_symbol = trim_asm(statement.raw_operands);
        continue;
      }
      return std::nullopt;
    }

    if (!in_text) return std::nullopt;

    if (statement.kind == AsmStatementKind::Label) {
      active_label = statement.op;
      continue;
    }

    if (statement.kind != AsmStatementKind::Instruction) {
      return std::nullopt;
    }
  }

  if (global_symbol.empty() || active_label.empty() || global_symbol != active_label) {
    return std::nullopt;
  }

  const auto encoded = encoder::encode_function(statements);
  if (!encoded.encoded || encoded.bytes.empty()) return std::nullopt;
  return MinimalObjectSlice{global_symbol, encoded.bytes, encoded.relocations};
}

std::vector<std::uint8_t> build_elf_object(const MinimalObjectSlice& slice) {
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;
  const bool has_relocations = !slice.relocations.empty();

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += slice.symbol;
  strtab.push_back('\0');
  std::vector<std::uint32_t> relocation_symbol_names;
  relocation_symbol_names.reserve(slice.relocations.size());
  for (const auto& relocation : slice.relocations) {
    relocation_symbol_names.push_back(static_cast<std::uint32_t>(strtab.size()));
    strtab += relocation.symbol;
    strtab.push_back('\0');
  }

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  std::uint32_t rela_text_name = 0;
  if (has_relocations) {
    rela_text_name = static_cast<std::uint32_t>(shstrtab.size());
    shstrtab += ".rela.text";
    shstrtab.push_back('\0');
  }
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> symtab;
  append_zeroes(symtab, 24);
  append_u32(symtab, 0);
  symtab.push_back(symbol_info(STB_LOCAL, STT_SECTION));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, 0);
  append_u32(symtab, main_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 0);
  append_u64(symtab, slice.text_bytes.size());

  for (std::size_t index = 0; index < slice.relocations.size(); ++index) {
    append_u32(symtab, relocation_symbol_names[index]);
    symtab.push_back(symbol_info(STB_GLOBAL, STT_NOTYPE));
    symtab.push_back(0);
    append_u16(symtab, 0);
    append_u64(symtab, 0);
    append_u64(symtab, 0);
  }

  std::vector<std::uint8_t> rela_text;
  for (std::size_t index = 0; index < slice.relocations.size(); ++index) {
    const auto& relocation = slice.relocations[index];
    append_u64(rela_text, relocation.offset);
    append_u64(rela_text, ((static_cast<std::uint64_t>(3 + index) << 32) |
                           relocation.reloc_type));
    append_i64(rela_text, relocation.addend);
  }

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += slice.text_bytes.size();
  offset = align_up(offset, 8);

  std::size_t rela_text_offset = 0;
  if (has_relocations) {
    rela_text_offset = offset;
    offset += rela_text.size();
    offset = align_up(offset, 8);
  }

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;
  const std::uint16_t section_count = has_relocations ? 6 : 5;
  const std::uint16_t shstrtab_index = has_relocations ? 5 : 4;
  const std::uint32_t symtab_link = has_relocations ? 4 : 3;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + section_count * kSectionHeaderSize);
  out.push_back(0x7f);
  out.push_back('E');
  out.push_back('L');
  out.push_back('F');
  out.push_back(2);
  out.push_back(1);
  out.push_back(1);
  out.push_back(0);
  out.push_back(0);
  append_zeroes(out, 7);
  append_u16(out, ET_REL);
  append_u16(out, EM_X86_64);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, 0);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, section_count);
  append_u16(out, shstrtab_index);

  out.insert(out.end(), slice.text_bytes.begin(), slice.text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  if (has_relocations) {
    out.insert(out.end(), rela_text.begin(), rela_text.end());
    append_zeroes(out, align_up(out.size(), 8) - out.size());
  }

  out.insert(out.end(), symtab.begin(), symtab.end());
  out.insert(out.end(), strtab.begin(), strtab.end());
  out.insert(out.end(), shstrtab.begin(), shstrtab.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());

  append_zeroes(out, kSectionHeaderSize);

  append_u32(out, text_name);
  append_u32(out, SHT_PROGBITS);
  append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
  append_u64(out, 0);
  append_u64(out, text_offset);
  append_u64(out, slice.text_bytes.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  if (has_relocations) {
    append_u32(out, rela_text_name);
    append_u32(out, SHT_RELA);
    append_u64(out, 0);
    append_u64(out, 0);
    append_u64(out, rela_text_offset);
    append_u64(out, rela_text.size());
    append_u32(out, 3);
    append_u32(out, 1);
    append_u64(out, 8);
    append_u64(out, 24);
  }

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, symtab_link);
  append_u32(out, 2);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, strtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, strtab_offset);
  append_u64(out, strtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  append_u32(out, shstrtab_name);
  append_u32(out, SHT_STRTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, shstrtab_offset);
  append_u64(out, shstrtab.size());
  append_u32(out, 0);
  append_u32(out, 0);
  append_u64(out, 1);
  append_u64(out, 0);

  return out;
}

}  // namespace

bool write_elf_object(const std::vector<AsmStatement>& statements,
                      const std::string& output_path) {
  const auto slice = parse_minimal_text_slice(statements);
  if (!slice.has_value()) return false;

  std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
  if (!out) return false;

  const auto bytes = build_elf_object(*slice);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  return out.good();
}

}  // namespace c4c::backend::x86::assembler
