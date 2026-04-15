#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::riscv::assembler {

struct Operand {
  enum class Kind {
    Reg,
    Imm,
    Symbol,
    SymbolOffset,
    Mem,
    MemSymbol,
    Label,
    FenceArg,
    Csr,
    RoundingMode,
  } kind = Kind::Symbol;

  std::string text;
  std::int64_t value = 0;
  std::string base;
  std::string symbol;
  std::string modifier;
};

enum class SymbolType {
  Function,
  Object,
  TlsObject,
  NoType,
};

enum class Visibility {
  Hidden,
  Protected,
  Internal,
};

struct SectionInfo {
  std::string name;
  std::string flags;
  std::string sec_type;
  bool flags_explicit = false;
};

struct SizeExpr {
  enum class Kind {
    CurrentMinus,
    Absolute,
  } kind = Kind::Absolute;

  std::string symbol;
  std::uint64_t absolute = 0;
};

struct DataValue {
  enum class Kind {
    Integer,
    Symbol,
    SymbolDiff,
    Expression,
  } kind = Kind::Integer;

  std::int64_t integer = 0;
  std::string name;
  std::string sym_a;
  std::string sym_b;
  std::int64_t addend = 0;
  std::string expr;
};

struct Directive {
  enum class Kind {
    Section,
    Text,
    Data,
    Bss,
    Rodata,
    Globl,
    Weak,
    SymVisibility,
    Type,
    Size,
    Align,
    Balign,
    Byte,
    Short,
    Long,
    Quad,
    Zero,
    Asciz,
    Ascii,
    Comm,
    Local,
    Set,
    ArchOption,
    Attribute,
    Cfi,
    Ignored,
    PushSection,
    PopSection,
    Previous,
    Insn,
    Incbin,
    Subsection,
    Unknown,
  } kind = Kind::Ignored;

  SectionInfo section;
  std::string text;
  std::string name;
  Visibility visibility = Visibility::Hidden;
  SymbolType symbol_type = SymbolType::NoType;
  SizeExpr size;
  std::uint64_t integer = 0;
  std::vector<DataValue> data_values;
  std::size_t zero_size = 0;
  std::uint8_t zero_fill = 0;
  std::vector<std::uint8_t> bytes;
  std::string sym;
  std::string value;
  std::uint64_t align = 0;
  std::uint64_t comm_size = 0;
  std::uint64_t comm_align = 1;
  std::string path;
  std::uint64_t skip = 0;
  std::optional<std::uint64_t> count;
  std::string unknown_name;
  std::string unknown_args;
};

struct AsmStatement {
  enum class Kind {
    Label,
    Directive,
    Instruction,
    Empty,
  } kind = Kind::Empty;

  std::string text;
  std::string mnemonic;
  std::vector<Operand> operands;
  std::string raw_operands;
  Directive directive;
};

std::vector<AsmStatement> parse_asm(const std::string& text);

}  // namespace c4c::backend::riscv::assembler
