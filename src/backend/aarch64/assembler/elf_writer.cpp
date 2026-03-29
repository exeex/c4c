#include "parser.hpp"
#include "encoder/mod.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::assembler {

static constexpr std::uint8_t AARCH64_NOP[4] = {0x1f, 0x20, 0x03, 0xd5};
static constexpr std::uint32_t ET_REL = 1;
static constexpr std::uint16_t EM_AARCH64 = 183;
static constexpr std::uint32_t SHT_NULL = 0;
static constexpr std::uint32_t SHT_PROGBITS = 1;
static constexpr std::uint32_t SHT_SYMTAB = 2;
static constexpr std::uint32_t SHT_STRTAB = 3;
static constexpr std::uint64_t SHF_ALLOC = 0x2;
static constexpr std::uint64_t SHF_EXECINSTR = 0x4;
static constexpr std::uint8_t STB_LOCAL = 0;
static constexpr std::uint8_t STB_GLOBAL = 1;
static constexpr std::uint8_t STT_NOTYPE = 0;
static constexpr std::uint8_t STT_FUNC = 2;
static constexpr std::uint8_t STT_SECTION = 3;

struct PendingReloc {
  std::string section;
  std::uint64_t offset = 0;
  std::uint32_t reloc_type = 0;
  std::string symbol;
  std::int64_t addend = 0;
};

struct PendingExpr {
  std::string section;
  std::uint64_t offset = 0;
  std::string expr;
  std::size_t size = 0;
};

struct PendingSymDiff {
  std::string section;
  std::uint64_t offset = 0;
  std::string sym_a;
  std::string sym_b;
  std::int64_t extra_addend = 0;
  std::size_t size = 0;
};

struct PendingInstruction {
  std::string section;
  std::uint64_t offset = 0;
  std::string mnemonic;
  std::vector<Operand> operands;
  std::string raw_operands;
};

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

void append_bytes(std::vector<std::uint8_t>& out, const std::string& bytes) {
  out.insert(out.end(), bytes.begin(), bytes.end());
}

void append_zeroes(std::vector<std::uint8_t>& out, std::size_t count) {
  out.insert(out.end(), count, 0);
}

std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) return value;
  const auto mask = alignment - 1;
  return (value + mask) & ~mask;
}

std::uint32_t info(std::uint8_t bind, std::uint8_t type) {
  return static_cast<std::uint32_t>((bind << 4) | type);
}

struct MinimalObjectSlice {
  std::string symbol;
  std::vector<std::uint32_t> words;
};

class ElfWriter {
 public:
  ElfWriter() = default;

  void process_statements(const std::vector<AsmStatement>& statements) {
    slice_ = parse_minimal_text_slice(statements);
  }

  void write_elf(const std::string& output_path) {
    if (!slice_.has_value()) {
      emitted_object_ = false;
      return;
    }

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
      emitted_object_ = false;
      return;
    }

    const auto bytes = build_elf_object(*slice_);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    emitted_object_ = out.good();
  }

  bool emitted_object() const { return emitted_object_; }

 private:
  static std::optional<MinimalObjectSlice> parse_minimal_text_slice(
      const std::vector<AsmStatement>& statements) {
    bool in_text = false;
    std::string global_symbol;
    std::string function_symbol;
    std::string active_label;
    std::vector<std::uint32_t> words;

    for (const auto& stmt : statements) {
      if (stmt.kind == AsmStatementKind::Directive) {
        if (stmt.op == ".text") {
          in_text = true;
          continue;
        }
        if (stmt.op == ".globl" && !stmt.raw_operands.empty()) {
          global_symbol = trim_asm(stmt.raw_operands);
          continue;
        }
        if (stmt.op == ".type") {
          const auto comma = stmt.raw_operands.find(',');
          if (comma != std::string::npos &&
              trim_asm(stmt.raw_operands.substr(comma + 1)) == "%function") {
            function_symbol = trim_asm(stmt.raw_operands.substr(0, comma));
          }
          continue;
        }
        continue;
      }

      if (!in_text) {
        return std::nullopt;
      }

      if (stmt.kind == AsmStatementKind::Label) {
        active_label = stmt.op;
        continue;
      }

      if (stmt.kind != AsmStatementKind::Instruction) {
        return std::nullopt;
      }

      const auto encoded =
          encoder::encode_instruction(stmt.op, stmt.operands, stmt.raw_operands);
      if (!encoded.encoded) {
        return std::nullopt;
      }
      words.push_back(encoded.word);
    }

    if (words.empty() || global_symbol.empty() || function_symbol.empty() ||
        active_label.empty() || global_symbol != function_symbol ||
        function_symbol != active_label) {
      return std::nullopt;
    }

    return MinimalObjectSlice{function_symbol, std::move(words)};
  }

  static std::vector<std::uint8_t> build_elf_object(const MinimalObjectSlice& slice) {
    std::vector<std::uint8_t> text_bytes;
    for (const auto word : slice.words) {
      append_u32(text_bytes, word);
    }

    std::string strtab;
    strtab.push_back('\0');
    const std::uint32_t main_name = static_cast<std::uint32_t>(strtab.size());
    strtab += slice.symbol;
    strtab.push_back('\0');

    std::string shstrtab;
    shstrtab.push_back('\0');
    const std::uint32_t text_name = static_cast<std::uint32_t>(shstrtab.size());
    shstrtab += ".text";
    shstrtab.push_back('\0');
    const std::uint32_t symtab_name = static_cast<std::uint32_t>(shstrtab.size());
    shstrtab += ".symtab";
    shstrtab.push_back('\0');
    const std::uint32_t strtab_name = static_cast<std::uint32_t>(shstrtab.size());
    shstrtab += ".strtab";
    shstrtab.push_back('\0');
    const std::uint32_t shstrtab_name = static_cast<std::uint32_t>(shstrtab.size());
    shstrtab += ".shstrtab";
    shstrtab.push_back('\0');

    std::vector<std::uint8_t> symtab;
    append_zeroes(symtab, 24);
    append_u32(symtab, 0);
    symtab.push_back(static_cast<std::uint8_t>(info(STB_LOCAL, STT_SECTION)));
    symtab.push_back(0);
    append_u16(symtab, 1);
    append_u64(symtab, 0);
    append_u64(symtab, 0);
    append_u32(symtab, main_name);
    symtab.push_back(static_cast<std::uint8_t>(info(STB_GLOBAL, STT_FUNC)));
    symtab.push_back(0);
    append_u16(symtab, 1);
    append_u64(symtab, 0);
    append_u64(symtab, text_bytes.size());

    const std::size_t ehdr_size = 64;
    const std::size_t shdr_size = 64;
    const std::size_t shnum = 5;
    std::size_t offset = ehdr_size;

    const std::size_t text_offset = offset;
    offset += text_bytes.size();
    offset = align_up(offset, 8);

    const std::size_t symtab_offset = offset;
    offset += symtab.size();

    const std::size_t strtab_offset = offset;
    offset += strtab.size();

    const std::size_t shstrtab_offset = offset;
    offset += shstrtab.size();
    offset = align_up(offset, 8);

    const std::size_t section_header_offset = offset;

    std::vector<std::uint8_t> out;
    out.reserve(section_header_offset + shnum * shdr_size);
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
    append_u16(out, EM_AARCH64);
    append_u32(out, 1);
    append_u64(out, 0);
    append_u64(out, 0);
    append_u64(out, section_header_offset);
    append_u32(out, 0);
    append_u16(out, ehdr_size);
    append_u16(out, 0);
    append_u16(out, 0);
    append_u16(out, shdr_size);
    append_u16(out, shnum);
    append_u16(out, 4);

    out.insert(out.end(), text_bytes.begin(), text_bytes.end());
    append_zeroes(out, align_up(out.size(), 8) - out.size());
    out.insert(out.end(), symtab.begin(), symtab.end());
    append_bytes(out, strtab);
    append_bytes(out, shstrtab);
    append_zeroes(out, align_up(out.size(), 8) - out.size());

    append_zeroes(out, shdr_size);

    append_u32(out, text_name);
    append_u32(out, SHT_PROGBITS);
    append_u64(out, SHF_ALLOC | SHF_EXECINSTR);
    append_u64(out, 0);
    append_u64(out, text_offset);
    append_u64(out, text_bytes.size());
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
    append_u32(out, 3);
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

  void resolve_sym_diffs() {}
  void resolve_pending_exprs() {}
  void resolve_pending_instructions() {}
  void process_statement(const AsmStatement& stmt) { (void)stmt; }

  std::optional<MinimalObjectSlice> slice_;
  bool emitted_object_ = false;
};

bool is_branch_reloc_type(std::uint32_t elf_type) {
  return elf_type == 279 || elf_type == 280 || elf_type == 282 || elf_type == 283;
}

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path) {
  ElfWriter writer;
  writer.process_statements(statements);
  writer.write_elf(output_path);
  return writer.emitted_object();
}

}  // namespace c4c::backend::aarch64::assembler
