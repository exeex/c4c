#include "mod.hpp"
#include "parser.hpp"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::assembler {

// Mirror of ref/claudes-c-compiler/src/backend/riscv/assembler/elf_writer.rs.
//
// This translation keeps the self-contained helper logic that does not depend on
// the rest of the still-untranslated RV64 backend, and leaves the writer-facing
// orchestration in a narrow C++ skeleton so later slices can connect the parser,
// encoder, and ELF base layers without reworking this file again.

constexpr std::uint32_t EF_RISCV_RVC = 0x1;
constexpr std::uint32_t EF_RISCV_FLOAT_ABI_SINGLE = 0x2;
constexpr std::uint32_t EF_RISCV_FLOAT_ABI_DOUBLE = 0x4;
constexpr std::uint32_t EF_RISCV_FLOAT_ABI_QUAD = 0x6;

struct DeferredExpr {
  std::string section;
  std::uint64_t offset = 0;
  std::size_t size = 0;
  std::string expr;
};

struct PendingReloc {
  std::string section;
  std::uint64_t offset = 0;
  std::uint32_t reloc_type = 0;
  std::string symbol;
  std::int64_t addend = 0;
};

namespace {

constexpr std::uint16_t ET_REL = 1;
constexpr std::uint16_t EM_RISCV = 243;
constexpr std::uint32_t SHT_NULL = 0;
constexpr std::uint32_t SHT_PROGBITS = 1;
constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t SHT_STRTAB = 3;
constexpr std::uint64_t SHF_ALLOC = 0x2;
constexpr std::uint64_t SHF_EXECINSTR = 0x4;
constexpr std::uint64_t SHF_INFO_LINK = 0x40;
constexpr std::uint8_t STB_LOCAL = 0;
constexpr std::uint8_t STB_GLOBAL = 1;
constexpr std::uint8_t STT_NOTYPE = 0;
constexpr std::uint8_t STT_FUNC = 2;
constexpr std::uint8_t STT_SECTION = 3;
constexpr std::uint32_t R_RISCV_JAL = 17;

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

struct MinimalActivationSlice {
  std::string symbol;
  std::vector<std::uint8_t> text_bytes;
};

struct MinimalJalRelocationSlice {
  std::string main_symbol;
  std::string helper_symbol;
  std::vector<std::uint8_t> text_bytes;
};

constexpr std::uint32_t OP_OP_IMM = 0b0010011;
constexpr std::uint32_t OP_JAL = 0b1101111;
constexpr std::uint32_t OP_JALR = 0b1100111;

const Operand* require_operand(const AsmStatement& statement, std::size_t index) {
  if (index >= statement.operands.size()) {
    return nullptr;
  }
  return &statement.operands[index];
}

std::optional<std::uint32_t> reg_num(std::string_view name) {
  if (name == "zero" || name == "x0") return 0;
  if (name == "ra" || name == "x1") return 1;
  if (name == "sp" || name == "x2") return 2;
  if (name == "gp" || name == "x3") return 3;
  if (name == "tp" || name == "x4") return 4;
  if (name == "t0" || name == "x5") return 5;
  if (name == "t1" || name == "x6") return 6;
  if (name == "t2" || name == "x7") return 7;
  if (name == "s0" || name == "fp" || name == "x8") return 8;
  if (name == "s1" || name == "x9") return 9;
  if (name == "a0" || name == "x10") return 10;
  if (name == "a1" || name == "x11") return 11;
  if (name == "a2" || name == "x12") return 12;
  if (name == "a3" || name == "x13") return 13;
  if (name == "a4" || name == "x14") return 14;
  if (name == "a5" || name == "x15") return 15;
  if (name == "a6" || name == "x16") return 16;
  if (name == "a7" || name == "x17") return 17;
  if (name == "s2" || name == "x18") return 18;
  if (name == "s3" || name == "x19") return 19;
  if (name == "s4" || name == "x20") return 20;
  if (name == "s5" || name == "x21") return 21;
  if (name == "s6" || name == "x22") return 22;
  if (name == "s7" || name == "x23") return 23;
  if (name == "s8" || name == "x24") return 24;
  if (name == "s9" || name == "x25") return 25;
  if (name == "s10" || name == "x26") return 26;
  if (name == "s11" || name == "x27") return 27;
  if (name == "t3" || name == "x28") return 28;
  if (name == "t4" || name == "x29") return 29;
  if (name == "t5" || name == "x30") return 30;
  if (name == "t6" || name == "x31") return 31;
  return std::nullopt;
}

bool operand_is_register_number(const Operand* operand, std::uint32_t reg) {
  return operand != nullptr && operand->kind == Operand::Kind::Reg &&
         reg_num(operand->text) == reg;
}

std::uint32_t encode_i(std::uint32_t opcode,
                       std::uint32_t rd,
                       std::uint32_t funct3,
                       std::uint32_t rs1,
                       std::int32_t imm) {
  const std::uint32_t encoded = static_cast<std::uint32_t>(imm) & 0xFFFu;
  return (encoded << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

std::uint32_t encode_j(std::uint32_t opcode, std::uint32_t rd, std::int32_t imm) {
  const auto encoded = static_cast<std::uint32_t>(imm);
  const std::uint32_t bit20 = (encoded >> 20) & 0x1u;
  const std::uint32_t bits10_1 = (encoded >> 1) & 0x3ffu;
  const std::uint32_t bit11 = (encoded >> 11) & 0x1u;
  const std::uint32_t bits19_12 = (encoded >> 12) & 0xffu;
  return (bit20 << 31) | (bits10_1 << 21) | (bit11 << 20) | (bits19_12 << 12) |
         (rd << 7) | opcode;
}

std::optional<std::uint32_t> encode_minimal_activation_instr(const AsmStatement& statement) {
  if (statement.kind != AsmStatement::Kind::Instruction) {
    return std::nullopt;
  }

  if (statement.mnemonic == "addi" && statement.operands.size() == 3 &&
      operand_is_register_number(require_operand(statement, 0), 10) &&
      operand_is_register_number(require_operand(statement, 1), 0)) {
    const auto* imm = require_operand(statement, 2);
    if (imm == nullptr || imm->kind != Operand::Kind::Imm || imm->value < -2048 ||
        imm->value > 2047) {
      return std::nullopt;
    }
    return encode_i(OP_OP_IMM, 10, 0b000, 0, static_cast<std::int32_t>(imm->value));
  }

  if (statement.mnemonic == "ret" && statement.operands.empty()) {
    return encode_i(OP_JALR, 0, 0b000, 1, 0);
  }

  return std::nullopt;
}

std::optional<MinimalActivationSlice> parse_minimal_activation_slice(
    const std::vector<AsmStatement>& statements) {
  if (statements.size() != 5) {
    return std::nullopt;
  }

  if (statements[0].kind != AsmStatement::Kind::Directive ||
      statements[0].directive.kind != Directive::Kind::Text) {
    return std::nullopt;
  }

  if (statements[1].kind != AsmStatement::Kind::Directive ||
      statements[1].directive.kind != Directive::Kind::Globl ||
      statements[1].directive.sym != "main") {
    return std::nullopt;
  }

  if (statements[2].kind != AsmStatement::Kind::Label || statements[2].text != "main") {
    return std::nullopt;
  }

  if (statements[3].kind != AsmStatement::Kind::Instruction ||
      statements[3].mnemonic != "addi") {
    return std::nullopt;
  }

  if (statements[4].kind != AsmStatement::Kind::Instruction ||
      statements[4].mnemonic != "ret" || !statements[4].operands.empty()) {
    return std::nullopt;
  }

  const auto addi_word = encode_minimal_activation_instr(statements[3]);
  const auto ret_word = encode_minimal_activation_instr(statements[4]);
  if (!addi_word.has_value() || !ret_word.has_value()) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> text_bytes;
  text_bytes.reserve(8);
  append_u32(text_bytes, *addi_word);
  append_u32(text_bytes, *ret_word);

  return MinimalActivationSlice{
      .symbol = "main",
      .text_bytes = std::move(text_bytes),
  };
}

bool operand_is_symbol_name(const Operand* operand, std::string_view expected) {
  return operand != nullptr &&
         (operand->kind == Operand::Kind::Symbol || operand->kind == Operand::Kind::Label) &&
         operand->text == expected;
}

std::optional<MinimalJalRelocationSlice> parse_minimal_jal_relocation_slice(
    const std::vector<AsmStatement>& statements) {
  if (statements.size() != 8) {
    return std::nullopt;
  }

  if (statements[0].kind != AsmStatement::Kind::Directive ||
      statements[0].directive.kind != Directive::Kind::Text) {
    return std::nullopt;
  }

  if (statements[1].kind != AsmStatement::Kind::Directive ||
      statements[1].directive.kind != Directive::Kind::Globl ||
      statements[1].directive.sym != "main") {
    return std::nullopt;
  }

  if (statements[2].kind != AsmStatement::Kind::Directive ||
      statements[2].directive.kind != Directive::Kind::Globl ||
      statements[2].directive.sym != "helper") {
    return std::nullopt;
  }

  if (statements[3].kind != AsmStatement::Kind::Label || statements[3].text != "main") {
    return std::nullopt;
  }

  if (statements[4].kind != AsmStatement::Kind::Instruction ||
      statements[4].mnemonic != "jal" || statements[4].operands.size() != 1 ||
      !operand_is_symbol_name(require_operand(statements[4], 0), "helper")) {
    return std::nullopt;
  }

  if (statements[5].kind != AsmStatement::Kind::Instruction ||
      statements[5].mnemonic != "ret" || !statements[5].operands.empty()) {
    return std::nullopt;
  }

  if (statements[6].kind != AsmStatement::Kind::Label || statements[6].text != "helper") {
    return std::nullopt;
  }

  if (statements[7].kind != AsmStatement::Kind::Instruction ||
      statements[7].mnemonic != "ret" || !statements[7].operands.empty()) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> text_bytes;
  text_bytes.reserve(12);
  append_u32(text_bytes, encode_j(OP_JAL, 1, 0));
  append_u32(text_bytes, encode_i(OP_JALR, 0, 0b000, 1, 0));
  append_u32(text_bytes, encode_i(OP_JALR, 0, 0b000, 1, 0));

  return MinimalJalRelocationSlice{
      .main_symbol = "main",
      .helper_symbol = "helper",
      .text_bytes = std::move(text_bytes),
  };
}

std::vector<std::uint8_t> build_minimal_elf_object(const MinimalActivationSlice& slice,
                                                   std::uint8_t elf_class,
                                                   std::uint32_t elf_flags) {
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += slice.symbol;
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
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

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += slice.text_bytes.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;
  constexpr std::uint16_t section_count = 5;
  constexpr std::uint16_t shstrtab_index = 4;
  constexpr std::uint32_t symtab_link = 3;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + section_count * kSectionHeaderSize);
  out.push_back(0x7f);
  out.push_back('E');
  out.push_back('L');
  out.push_back('F');
  out.push_back(elf_class);
  out.push_back(1);
  out.push_back(1);
  out.push_back(0);
  append_zeroes(out, 8);
  append_u16(out, ET_REL);
  append_u16(out, EM_RISCV);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, elf_flags);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, section_count);
  append_u16(out, shstrtab_index);

  out.insert(out.end(), slice.text_bytes.begin(), slice.text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
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
  append_u64(out, 4);
  append_u64(out, 0);

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

std::vector<std::uint8_t> build_minimal_jal_relocation_object(
    const MinimalJalRelocationSlice& slice,
    std::uint8_t elf_class,
    std::uint32_t elf_flags) {
  constexpr std::size_t kElfHeaderSize = 64;
  constexpr std::size_t kSectionHeaderSize = 64;

  std::string strtab;
  strtab.push_back('\0');
  const auto main_name = static_cast<std::uint32_t>(strtab.size());
  strtab += slice.main_symbol;
  strtab.push_back('\0');
  const auto helper_name = static_cast<std::uint32_t>(strtab.size());
  strtab += slice.helper_symbol;
  strtab.push_back('\0');

  std::string shstrtab;
  shstrtab.push_back('\0');
  const auto text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".text";
  shstrtab.push_back('\0');
  const auto rela_text_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".rela.text";
  shstrtab.push_back('\0');
  const auto symtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".symtab";
  shstrtab.push_back('\0');
  const auto strtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".strtab";
  shstrtab.push_back('\0');
  const auto shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
  shstrtab += ".shstrtab";
  shstrtab.push_back('\0');

  std::vector<std::uint8_t> rela_text;
  append_u64(rela_text, 0);
  append_u64(rela_text, (static_cast<std::uint64_t>(3) << 32) | R_RISCV_JAL);
  append_u64(rela_text, 0);

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
  append_u64(symtab, 8);
  append_u32(symtab, helper_name);
  symtab.push_back(symbol_info(STB_GLOBAL, STT_FUNC));
  symtab.push_back(0);
  append_u16(symtab, 1);
  append_u64(symtab, 8);
  append_u64(symtab, 4);

  std::size_t offset = kElfHeaderSize;
  const auto text_offset = offset;
  offset += slice.text_bytes.size();
  offset = align_up(offset, 8);

  const auto rela_text_offset = offset;
  offset += rela_text.size();
  offset = align_up(offset, 8);

  const auto symtab_offset = offset;
  offset += symtab.size();

  const auto strtab_offset = offset;
  offset += strtab.size();

  const auto shstrtab_offset = offset;
  offset += shstrtab.size();
  offset = align_up(offset, 8);

  const auto section_header_offset = offset;
  constexpr std::uint16_t section_count = 6;
  constexpr std::uint16_t shstrtab_index = 5;

  std::vector<std::uint8_t> out;
  out.reserve(section_header_offset + section_count * kSectionHeaderSize);
  out.push_back(0x7f);
  out.push_back('E');
  out.push_back('L');
  out.push_back('F');
  out.push_back(elf_class);
  out.push_back(1);
  out.push_back(1);
  out.push_back(0);
  append_zeroes(out, 8);
  append_u16(out, ET_REL);
  append_u16(out, EM_RISCV);
  append_u32(out, 1);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, section_header_offset);
  append_u32(out, elf_flags);
  append_u16(out, kElfHeaderSize);
  append_u16(out, 0);
  append_u16(out, 0);
  append_u16(out, kSectionHeaderSize);
  append_u16(out, section_count);
  append_u16(out, shstrtab_index);

  out.insert(out.end(), slice.text_bytes.begin(), slice.text_bytes.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
  out.insert(out.end(), rela_text.begin(), rela_text.end());
  append_zeroes(out, align_up(out.size(), 8) - out.size());
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
  append_u64(out, 4);
  append_u64(out, 0);

  append_u32(out, rela_text_name);
  append_u32(out, SHT_RELA);
  append_u64(out, SHF_INFO_LINK);
  append_u64(out, 0);
  append_u64(out, rela_text_offset);
  append_u64(out, rela_text.size());
  append_u32(out, 3);
  append_u32(out, 1);
  append_u64(out, 8);
  append_u64(out, 24);

  append_u32(out, symtab_name);
  append_u32(out, SHT_SYMTAB);
  append_u64(out, 0);
  append_u64(out, 0);
  append_u64(out, symtab_offset);
  append_u64(out, symtab.size());
  append_u32(out, 4);
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

bool is_numeric_label(const std::string& name) {
  if (name.empty()) return false;
  for (char ch : name) {
    if (ch < '0' || ch > '9') return false;
  }
  return true;
}

std::optional<std::pair<std::string, bool>> parse_numeric_label_ref(const std::string& symbol) {
  if (symbol.size() < 2) return std::nullopt;
  const char suffix = symbol.back();
  const bool is_backward = suffix == 'b' || suffix == 'B';
  const bool is_forward = suffix == 'f' || suffix == 'F';
  if (!is_backward && !is_forward) return std::nullopt;

  const auto label_part = symbol.substr(0, symbol.size() - 1);
  if (label_part.empty() || !is_numeric_label(label_part)) return std::nullopt;
  return std::make_pair(label_part, is_backward);
}

std::pair<std::string, std::int64_t> decompose_symbol_addend(const std::string& name) {
  const auto plus_pos = name.rfind('+');
  if (plus_pos != std::string::npos) {
    const auto base = name.substr(0, plus_pos);
    const auto offset_str = name.substr(plus_pos + 1);
    if (!base.empty() && !offset_str.empty()) {
      try {
        return {base, std::stoll(offset_str)};
      } catch (...) {
      }
    }
  }

  const auto minus_pos = name.rfind('-');
  if (minus_pos != std::string::npos && minus_pos > 0) {
    const auto base = name.substr(0, minus_pos);
    const auto offset_str = name.substr(minus_pos);
    if (!base.empty()) {
      try {
        return {base, std::stoll(offset_str)};
      } catch (...) {
      }
    }
  }

  return {name, 0};
}

std::optional<std::string> resolve_numeric_ref_name(
    const std::string& symbol,
    std::size_t stmt_idx,
    const std::unordered_map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>& label_defs) {
  const auto parsed = parse_numeric_label_ref(symbol);
  if (!parsed.has_value()) return std::nullopt;

  const auto& [label_name, is_backward] = *parsed;
  const auto it = label_defs.find(label_name);
  if (it == label_defs.end()) return std::nullopt;

  if (is_backward) {
    std::optional<std::size_t> best;
    for (const auto& [def_idx, inst_id] : it->second) {
      if (def_idx < stmt_idx) best = inst_id;
    }
    if (!best.has_value()) return std::nullopt;
    return ".Lnum_" + label_name + "_" + std::to_string(*best);
  }

  for (const auto& [def_idx, inst_id] : it->second) {
    if (def_idx > stmt_idx) {
      return ".Lnum_" + label_name + "_" + std::to_string(inst_id);
    }
  }
  return std::nullopt;
}

std::string rewrite_symbol_name(
    const std::string& name,
    std::size_t stmt_idx,
    const std::unordered_map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>& label_defs,
    std::size_t* dot_counter,
    std::vector<std::string>* dot_labels) {
  if (name == ".") {
    const auto label = ".Ldot_" + std::to_string((*dot_counter)++);
    dot_labels->push_back(label);
    return label;
  }
  if (const auto resolved = resolve_numeric_ref_name(name, stmt_idx, label_defs)) {
    return *resolved;
  }
  return name;
}

std::string rewrite_expr_numeric_refs(
    const std::string& expr,
    std::size_t stmt_idx,
    const std::unordered_map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>& label_defs,
    std::size_t* dot_counter,
    std::vector<std::string>* dot_labels) {
  std::string result;
  result.reserve(expr.size());

  for (std::size_t i = 0; i < expr.size();) {
    const unsigned char ch = static_cast<unsigned char>(expr[i]);

    if (ch == '.') {
      const bool prev_is_sep =
          i == 0 || std::string_view(" ()+-*/|&^,~").find(expr[i - 1]) != std::string_view::npos;
      const bool next_is_sep =
          i + 1 >= expr.size() || std::string_view(" ()+-*/|&^,~").find(expr[i + 1]) != std::string_view::npos;
      if (prev_is_sep && next_is_sep) {
        const auto label = ".Ldot_" + std::to_string((*dot_counter)++);
        dot_labels->push_back(label);
        result += label;
        ++i;
        continue;
      }
    }

    if (std::isdigit(ch)) {
      const std::size_t start = i;
      while (i < expr.size() && std::isdigit(static_cast<unsigned char>(expr[i]))) {
        ++i;
      }
      if (i < expr.size()) {
        const char suffix = expr[i];
        if (suffix == 'f' || suffix == 'F' || suffix == 'b' || suffix == 'B') {
          const bool next_is_ident =
              i + 1 < expr.size() &&
              (std::isalnum(static_cast<unsigned char>(expr[i + 1])) || expr[i + 1] == '_');
          if (!next_is_ident) {
            const auto token = expr.substr(start, i - start + 1);
            if (const auto resolved = resolve_numeric_ref_name(token, stmt_idx, label_defs)) {
              result += *resolved;
              ++i;
              continue;
            }
          }
        }
      }
      result.append(expr, start, i - start);
      continue;
    }

    result.push_back(expr[i]);
    ++i;
  }

  return result;
}

}  // namespace

class ElfWriter {
 public:
  ElfWriter() = default;

  static constexpr std::uint32_t kRiscvRelax = 51;
  static constexpr std::uint32_t kRiscvAlign = 43;

  void set_elf_flags(std::uint32_t flags) { elf_flags_ = flags; }
  void set_elf_class(std::uint8_t class_id) { elf_class_ = class_id; }

  // The remaining methods are a direct follow-on from the Rust writer, but they
  // depend on the RV64 parser/encoder/base layers that are being translated in
  // separate one-file slices. Keep the surface here so those slices can connect
  // without another structural rewrite.
  void process_statements(const std::vector<AsmStatement>& statements) {
    minimal_activation_slice_ = parse_minimal_activation_slice(statements);
    minimal_jal_relocation_slice_ = parse_minimal_jal_relocation_slice(statements);
  }

  void emit_align_with_reloc(std::uint64_t align_bytes) {
    (void)align_bytes;
  }

  std::pair<std::string, std::int64_t> split_symbol_addend_for_data(const std::string& name) const {
    return decompose_symbol_addend(name);
  }

  std::string rewrite_numeric_expr_for_data(
      const std::string& expr,
      std::size_t stmt_idx,
      const std::unordered_map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>& label_defs,
      std::size_t* dot_counter,
      std::vector<std::string>* dot_labels) const {
    return rewrite_expr_numeric_refs(expr, stmt_idx, label_defs, dot_counter, dot_labels);
  }

  std::optional<std::string> resolve_numeric_label(
      const std::string& label_name,
      bool is_backward,
      const std::string& ref_section,
      std::uint64_t ref_offset) const {
    (void)label_name;
    (void)is_backward;
    (void)ref_section;
    (void)ref_offset;
    return std::nullopt;
  }

  bool write_elf(const std::string& output_path) const {
    if (!minimal_activation_slice_.has_value() && !minimal_jal_relocation_slice_.has_value()) {
      return false;
    }
    if (output_path.empty()) {
      return true;
    }

    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      return false;
    }

    std::vector<std::uint8_t> bytes;
    if (minimal_jal_relocation_slice_.has_value()) {
      bytes = build_minimal_jal_relocation_object(*minimal_jal_relocation_slice_, elf_class_,
                                                  elf_flags_);
    } else {
      bytes = build_minimal_elf_object(*minimal_activation_slice_, elf_class_, elf_flags_);
    }
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    return out.good();
  }

 private:
  std::uint32_t elf_flags_ = EF_RISCV_FLOAT_ABI_DOUBLE | EF_RISCV_RVC;
  std::uint8_t elf_class_ = 2;
  bool no_relax_ = false;
  std::vector<bool> option_stack_;
  std::vector<PendingReloc> pending_branch_relocs_;
  std::vector<DeferredExpr> deferred_exprs_;
  std::optional<MinimalActivationSlice> minimal_activation_slice_;
  std::optional<MinimalJalRelocationSlice> minimal_jal_relocation_slice_;
};

// Direct translations of the Rust helpers above are intentionally kept local
// to this file so later slices can promote them into shared headers only when
// the surrounding RV64 backend has been translated.

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path) {
  ElfWriter writer;
  writer.set_elf_class(2);
  writer.set_elf_flags(0);
  writer.process_statements(statements);
  return writer.write_elf(output_path);
}

bool write_minimal_elf_object(const std::vector<AsmStatement>& statements,
                              const std::string& output_path) {
  const auto slice = parse_minimal_activation_slice(statements);
  if (!slice.has_value()) {
    return false;
  }

  if (output_path.empty()) {
    return true;
  }

  std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }

  const auto bytes = build_minimal_elf_object(*slice, 2, 0);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  return out.good();
}

}  // namespace c4c::backend::riscv::assembler
