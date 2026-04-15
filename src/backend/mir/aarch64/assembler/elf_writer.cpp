#include "parser.hpp"
#include "encoder/mod.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::assembler {

static constexpr std::uint8_t AARCH64_NOP[4] = {0x1f, 0x20, 0x03, 0xd5};
static constexpr std::uint16_t ET_REL = 1;
static constexpr std::uint16_t EM_AARCH64 = 183;
static constexpr std::uint32_t SHT_NULL = 0;
static constexpr std::uint32_t SHT_PROGBITS = 1;
static constexpr std::uint32_t SHT_SYMTAB = 2;
static constexpr std::uint32_t SHT_STRTAB = 3;
static constexpr std::uint32_t SHT_RELA = 4;
static constexpr std::uint32_t SHT_NOBITS = 8;
static constexpr std::uint64_t SHF_WRITE = 0x1;
static constexpr std::uint64_t SHF_ALLOC = 0x2;
static constexpr std::uint64_t SHF_EXECINSTR = 0x4;
static constexpr std::uint8_t STB_LOCAL = 0;
static constexpr std::uint8_t STB_GLOBAL = 1;
static constexpr std::uint8_t STT_NOTYPE = 0;
static constexpr std::uint8_t STT_FUNC = 2;
static constexpr std::uint8_t STT_SECTION = 3;
static constexpr std::uint16_t SHN_UNDEF = 0;

std::uint8_t symbol_info(std::uint8_t binding, std::uint8_t type) {
  return static_cast<std::uint8_t>((binding << 4) | type);
}

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

struct Section {
  std::string name;
  std::uint32_t sh_type = SHT_PROGBITS;
  std::uint64_t flags = 0;
  std::uint64_t alignment = 1;
  std::vector<std::uint8_t> data;
  std::vector<PendingReloc> relocs;
};

struct Symbol {
  std::string name;
  std::uint8_t bind = STB_LOCAL;
  std::uint8_t type = STT_NOTYPE;
  std::uint16_t shndx = SHN_UNDEF;
  std::uint64_t value = 0;
  bool defined = false;
  bool global = false;
};

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

void append_u16_le(std::vector<std::uint8_t>& out, std::uint16_t value) { append_u16(out, value); }
void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t value) { append_u32(out, value); }
void append_u64_le(std::vector<std::uint8_t>& out, std::uint64_t value) { append_u64(out, value); }

void append_u32_be(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

std::string trim_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::uint32_t reloc_type_from_encoder(encoder::RelocType type) {
  using encoder::RelocType;
  switch (type) {
    case RelocType::Call26:
      return 283;
    case RelocType::Jump26:
      return 282;
    case RelocType::AdrpPage21:
      return 275;
    case RelocType::AddAbsLo12:
      return 277;
    case RelocType::Ldst8AbsLo12:
      return 278;
    case RelocType::Ldst16AbsLo12:
      return 284;
    case RelocType::Ldst32AbsLo12:
      return 285;
    case RelocType::Ldst64AbsLo12:
      return 286;
    case RelocType::Ldst128AbsLo12:
      return 299;
    case RelocType::AdrGotPage21:
      return 311;
    case RelocType::Ld64GotLo12:
      return 312;
    case RelocType::TlsLeAddTprelLo12:
      return 0x21d;  // best-effort fallback
    case RelocType::TlsLeAddTprelHi12:
      return 0x21c;  // best-effort fallback
    case RelocType::CondBr19:
      return 280;
    case RelocType::TstBr14:
      return 279;
    case RelocType::AdrPrelLo21:
      return 274;
    case RelocType::Abs64:
      return 257;
    case RelocType::Abs32:
      return 258;
    case RelocType::Prel32:
      return 261;
    case RelocType::Prel64:
      return 260;
    case RelocType::Ldr19:
      return 0;
    default:
      return 0;
  }
}

void append_le_bytes(std::vector<std::uint8_t>& bytes,
                    std::size_t width,
                    std::uint64_t value) {
  if (width == 1) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
    return;
  }
  if (width == 2) {
    append_u16(bytes, static_cast<std::uint16_t>(value));
    return;
  }
  if (width == 4) {
    append_u32(bytes, static_cast<std::uint32_t>(value));
    return;
  }
  if (width == 8) {
    append_u64(bytes, value);
    return;
  }
}

std::string unquote(std::string_view text) {
  const auto value = trim_copy(text);
  if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                            (value.front() == '\'' && value.back() == '\''))) {
    return std::string(value.substr(1, value.size() - 2));
  }
  return value;
}

bool parse_u64(std::string_view text, std::int64_t* out) {
  const auto value = trim_copy(text);
  if (value.empty()) return false;

  auto parse_decimal = [](std::string_view digits, std::int64_t sign, std::int64_t* out) {
    std::int64_t result = 0;
    if (digits.empty()) return false;
    for (char ch : digits) {
      if (ch < '0' || ch > '9') return false;
      result = result * 10 + (ch - '0');
    }
    *out = sign * result;
    return true;
  };

  std::int64_t sign = 1;
  std::size_t pos = 0;
  if (value[0] == '+') {
    pos = 1;
  } else if (value[0] == '-') {
    sign = -1;
    pos = 1;
  }
  if (pos >= value.size()) return false;

  const auto payload = value.substr(pos);
  if (payload.size() > 2 && payload[0] == '0' &&
      (payload[1] == 'x' || payload[1] == 'X')) {
    std::int64_t parsed = 0;
    for (std::size_t i = 2; i < payload.size(); ++i) {
      const char ch = payload[i];
      int digit;
      if (ch >= '0' && ch <= '9') {
        digit = ch - '0';
      } else if (ch >= 'a' && ch <= 'f') {
        digit = 10 + (ch - 'a');
      } else if (ch >= 'A' && ch <= 'F') {
        digit = 10 + (ch - 'A');
      } else {
        return false;
      }
      parsed = (parsed << 4) | digit;
    }
    *out = sign * parsed;
    return true;
  }

  return parse_decimal(payload, sign, out);
}

std::uint8_t detect_asm_flags(std::string_view flags_token) {
  std::uint64_t flags = 0;
  const auto flags_str = trim_copy(flags_token);
  for (char c : flags_str) {
    if (c == 'a' || c == 'A') flags |= SHF_ALLOC;
    if (c == 'x' || c == 'X') flags |= SHF_EXECINSTR;
    if (c == 'w' || c == 'W') flags |= SHF_WRITE;
  }
  return flags;
}

std::uint32_t detect_elf_section_type(std::string_view type_token) {
  const auto value = trim_copy(type_token);
  if (value.empty()) return SHT_PROGBITS;
  if (value.find("nobits") != std::string::npos || value.find("SHT_NOBITS") != std::string::npos) {
    return SHT_NOBITS;
  }
  return SHT_PROGBITS;
}

std::vector<std::uint8_t> bytes_for_data_directive(const std::string& op,
                                                  const std::vector<Operand>& operands,
                                                  std::size_t data_width) {
  std::vector<std::uint8_t> out;
  for (const auto& operand : operands) {
    const auto text = trim_copy(operand.text);
    if (text.empty()) continue;

    if (op == ".byte") {
      std::int64_t parsed = 0;
      if (parse_u64(text, &parsed)) {
        append_le_bytes(out, 1, static_cast<std::uint64_t>(parsed));
        continue;
      }
      if (!text.empty() && text.front() == '"' && text.back() == '"') {
        const auto s = unquote(text);
        out.insert(out.end(), s.begin(), s.end());
      }
      continue;
    }

    if ((op == ".short" || op == ".word" || op == ".int" || op == ".long" || op == ".quad") &&
        !text.empty()) {
      std::int64_t parsed = 0;
      if (!parse_u64(text, &parsed)) continue;
      const auto width = data_width;
      if (op == ".short") {
        append_le_bytes(out, 2, static_cast<std::uint64_t>(parsed));
      } else if (op == ".word" || op == ".int" || op == ".long") {
        append_le_bytes(out, 4, static_cast<std::uint64_t>(parsed));
      } else if (op == ".quad") {
        append_le_bytes(out, 8, static_cast<std::uint64_t>(parsed));
      } else {
        append_le_bytes(out, width, static_cast<std::uint64_t>(parsed));
      }
    }
  }
  return out;
}

bool is_global_directive(const std::string& op) {
  return op == ".globl" || op == ".global";
}

bool parse_alignment_directive(std::string_view op, std::int64_t raw_align, std::size_t* alignment_out) {
  if (raw_align <= 0 || alignment_out == nullptr) return false;
  if (op == ".p2align") {
    if (raw_align >= 63) return false;
    *alignment_out = static_cast<std::size_t>(1ull << raw_align);
    return true;
  }
  *alignment_out = static_cast<std::size_t>(raw_align);
  return true;
}

struct ParsedSlice {
  std::vector<Section> sections;
  std::vector<Symbol> symbols;
  std::vector<PendingReloc> pending_relocs;
  std::string active_section;
  bool has_entry_candidate = false;
  std::string default_entry;
};

std::optional<ParsedSlice> parse_slice(const std::vector<AsmStatement>& statements) {
  ParsedSlice slice;

  auto ensure_section = [&](const std::string& name) -> std::size_t {
    for (std::size_t index = 0; index < slice.sections.size(); ++index) {
      if (slice.sections[index].name == name) return index;
    }
    Section sec;
    sec.name = name;
    if (name == ".text") {
      sec.flags = SHF_ALLOC | SHF_EXECINSTR;
      sec.alignment = 4;
    } else if (name == ".data" || name == ".rodata") {
      sec.flags = SHF_ALLOC;
      sec.alignment = 4;
    } else if (name == ".bss") {
      sec.flags = SHF_ALLOC;
      sec.alignment = 4;
      sec.sh_type = SHT_NOBITS;
    } else {
      sec.alignment = 4;
    }
    slice.sections.push_back(std::move(sec));
    return slice.sections.size() - 1;
  };

  auto find_global = [](const std::vector<std::string>& globals, const std::string& name) {
    for (const auto& g : globals) {
      if (g == name) return true;
    }
    return false;
  };

  std::size_t active_index = ensure_section(".text");
  slice.active_section = ".text";

  for (const auto& statement : statements) {
    if (statement.kind == AsmStatementKind::Directive) {
      const auto& op = statement.op;
      if (op == ".text" || op == ".data" || op == ".rodata" || op == ".bss") {
        active_index = ensure_section(op);
        slice.active_section = op;
        continue;
      }

      if (op == ".section") {
        if (statement.operands.empty()) return std::nullopt;
        const auto name = trim_copy(statement.operands[0].text);
        if (name.empty()) return std::nullopt;

        active_index = ensure_section(name);
        slice.active_section = name;
        if (statement.operands.size() > 1) {
          slice.sections[active_index].flags = detect_asm_flags(statement.operands[1].text);
        }
        if (statement.operands.size() > 2) {
          slice.sections[active_index].sh_type = detect_elf_section_type(statement.operands[2].text);
        }
        if (slice.sections[active_index].sh_type == SHT_NOBITS) {
          slice.sections[active_index].data.clear();
        }
        continue;
      }

      if (op == ".globl" || op == ".global") {
        for (const auto& operand : statement.operands) {
          const auto symbol = trim_copy(operand.text);
          if (symbol.empty()) continue;
          bool exists = false;
          for (auto& sym : slice.symbols) {
            if (sym.name == symbol) {
              sym.global = true;
              sym.bind = STB_GLOBAL;
              exists = true;
              break;
            }
          }
          if (!exists) {
            slice.symbols.push_back(Symbol{.name = symbol, .bind = STB_GLOBAL, .global = true});
          }
        }
        continue;
      }

      if (op == ".type") {
        if (statement.operands.size() >= 2) {
          const auto symbol = trim_copy(statement.operands[0].text);
          const auto type_text = trim_copy(statement.operands[1].text);
          for (auto& sym : slice.symbols) {
            if (sym.name == symbol || (symbol.empty() && sym.global)) {
              if (type_text == "%function") {
                sym.type = STT_FUNC;
              } else {
                sym.type = STT_NOTYPE;
              }
              break;
            }
          }
          bool exists = false;
          for (const auto& existing : slice.symbols) {
            if (existing.name == symbol) {
              exists = true;
              break;
            }
          }
          if (!exists) {
            Symbol symbol_entry;
            symbol_entry.name = symbol;
            symbol_entry.type = (type_text == "%function") ? STT_FUNC : STT_NOTYPE;
            symbol_entry.global = false;
            slice.symbols.push_back(std::move(symbol_entry));
          }
        }
        continue;
      }

      if ((op == ".align") || (op == ".balign") || (op == ".p2align")) {
        if (statement.operands.empty()) return std::nullopt;
        auto& section = slice.sections[active_index];
        std::int64_t raw_align = 0;
        if (!parse_u64(statement.operands[0].text, &raw_align)) return std::nullopt;
        std::size_t alignment = 0;
        if (!parse_alignment_directive(op, raw_align, &alignment)) return std::nullopt;
        const auto target = align_up(section.data.size(), alignment);
        if (target >= section.data.size()) append_zeroes(section.data, target - section.data.size());
        section.alignment = std::max<std::uint64_t>(section.alignment, alignment);
        continue;
      }

      if (op == ".space" || op == ".skip" || op == ".zero") {
        if (statement.operands.empty()) return std::nullopt;
        std::int64_t count = 0;
        if (!parse_u64(statement.operands[0].text, &count) || count < 0) return std::nullopt;
        append_zeroes(slice.sections[active_index].data, static_cast<std::size_t>(count));
        continue;
      }

      if (op == ".byte" || op == ".short" || op == ".word" || op == ".long" || op == ".int" ||
          op == ".quad") {
        auto chunk = bytes_for_data_directive(op, statement.operands, 4);
        slice.sections[active_index].data.insert(slice.sections[active_index].data.end(),
                                                chunk.begin(),
                                                chunk.end());
        continue;
      }

      if (op == ".ascii" || op == ".asciz") {
        if (statement.operands.empty()) return std::nullopt;
        for (const auto& operand : statement.operands) {
          const auto unquoted = unquote(operand.text);
          slice.sections[active_index].data.insert(slice.sections[active_index].data.end(),
                                                  unquoted.begin(),
                                                  unquoted.end());
          if (op == ".asciz") slice.sections[active_index].data.push_back('\0');
        }
        continue;
      }
      if (op == ".text" || op == ".data" || op == ".bss" || op == ".rodata" ||
          op == ".section") {
        continue;
      }
      return std::nullopt;
    }

    if (statement.kind == AsmStatementKind::Label) {
      const auto label = trim_copy(statement.op);
      if (label.empty()) return std::nullopt;

      bool exists = false;
      for (auto& sym : slice.symbols) {
        if (sym.name == label) {
          sym.defined = true;
          sym.shndx = static_cast<std::uint16_t>(active_index + 1);
          sym.value = slice.sections[active_index].data.size();
          exists = true;
          break;
        }
      }
      if (!exists) {
        Symbol sym;
        sym.name = label;
        sym.shndx = static_cast<std::uint16_t>(active_index + 1);
        sym.value = slice.sections[active_index].data.size();
        slice.symbols.push_back(std::move(sym));
      }
      continue;
    }

    if (statement.kind != AsmStatementKind::Instruction) continue;

    auto& section = slice.sections[active_index];
    if (section.sh_type == SHT_NOBITS) {
      return std::nullopt;
    }

    const auto encoded = encoder::encode_instruction(statement.op, statement.operands, statement.raw_operands);
    if (!encoded.encoded) return std::nullopt;

    const auto insn_offset = section.data.size();

    switch (encoded.kind) {
      case encoder::EncodeResultKind::Word: {
        append_u32(section.data, encoded.word);
        break;
      }
      case encoder::EncodeResultKind::Words: {
        for (const auto word : encoded.words) {
          append_u32(section.data, word);
        }
        break;
      }
      case encoder::EncodeResultKind::WordWithReloc: {
        append_u32(section.data, encoded.word);
        const auto reloc_type = reloc_type_from_encoder(encoded.reloc.reloc_type);
        if (reloc_type == 0) return std::nullopt;

        slice.pending_relocs.push_back(PendingReloc{
            .section = section.name,
            .offset = static_cast<std::uint64_t>(insn_offset),
            .reloc_type = reloc_type,
            .symbol = encoded.reloc.symbol,
            .addend = encoded.reloc.addend,
        });
        break;
      }
      case encoder::EncodeResultKind::Skip:
      default:
        return std::nullopt;
    }
  }

  bool has_content = false;
  for (const auto& section : slice.sections) {
    if (!section.data.empty()) {
      has_content = true;
      break;
    }
    for (const auto& relocation : slice.pending_relocs) {
      if (relocation.section == section.name) {
        has_content = true;
        break;
      }
    }
    if (has_content) break;
  }

  if (!has_content) return std::nullopt;
  return slice;
}

std::uint64_t section_size(const Section& section) {
  if (section.sh_type == SHT_NOBITS) return section.data.size();
  return section.data.size();
}

std::vector<std::uint8_t> build_elf_object(const ParsedSlice& slice) {
  struct OutputSection {
    std::string name;
    std::uint32_t type = SHT_PROGBITS;
    std::uint64_t flags = 0;
    std::uint64_t alignment = 1;
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    std::uint32_t link = 0;
    std::uint32_t info = 0;
    std::uint64_t addralign = 1;
    std::uint64_t entsize = 0;
    std::vector<std::uint8_t> bytes;
    bool has_rela = false;
    std::size_t target_section_index = 0;
  };

  struct SymbolEntry {
    std::string name;
    std::uint8_t bind = STB_LOCAL;
    std::uint8_t type = STT_NOTYPE;
    std::uint16_t shndx = SHN_UNDEF;
    std::uint64_t value = 0;
    std::uint64_t size = 0;
    bool local = true;
  };

  const auto& sections = slice.sections;

  std::vector<OutputSection> out_sections;
  std::vector<PendingReloc> out_relocs;

  for (const auto& sec : sections) {
    OutputSection out_sec;
    out_sec.name = sec.name;
    out_sec.type = sec.sh_type;
    out_sec.flags = sec.flags;
    out_sec.alignment = std::max<std::uint64_t>(1, sec.alignment);
    out_sec.size = section_size(sec);
    out_sec.addralign = out_sec.alignment;
    if (out_sec.type != SHT_NOBITS) out_sec.bytes = sec.data;

    const auto header_index = out_sections.size();
    out_sections.push_back(out_sec);

    bool has_reloc = false;
    for (const auto& relocation : slice.pending_relocs) {
      if (relocation.section == sec.name) {
        has_reloc = true;
        out_relocs.push_back(relocation);
      }
    }

    if (has_reloc) {
      OutputSection rela_sec;
      rela_sec.name = ".rela." + sec.name;
      rela_sec.type = SHT_RELA;
      rela_sec.flags = 0;
      rela_sec.alignment = 8;
      rela_sec.addralign = 8;
      rela_sec.entsize = 24;
      rela_sec.has_rela = true;
      rela_sec.target_section_index = header_index;
      out_sections.push_back(rela_sec);
    }
  }

  std::vector<SymbolEntry> symbol_entries;
  std::vector<std::string> strtab_names;
  std::vector<std::pair<std::string, std::size_t>> symbol_to_index;
  std::size_t local_symbol_end = 1;

  symbol_entries.push_back({});
  strtab_names.push_back("");

  auto add_symbol = [&](const std::string& name,
                        std::uint8_t bind,
                        std::uint8_t type,
                        std::uint16_t shndx,
                        std::uint64_t value,
                        std::uint64_t size,
                        bool local) -> std::size_t {
    if (name.empty()) return 0;

    for (std::size_t idx = 0; idx < symbol_entries.size(); ++idx) {
      if (symbol_entries[idx].name == name) return idx;
    }

    SymbolEntry entry;
    entry.name = name;
    entry.bind = bind;
    entry.type = type;
    entry.shndx = shndx;
    entry.value = value;
    entry.size = size;
    entry.local = local;

    const auto index = symbol_entries.size();
    symbol_entries.push_back(std::move(entry));
    if (local) {
      ++local_symbol_end;
    }
    strtab_names.push_back(name);
    symbol_to_index.push_back({name, index});
    return index;
  };

  for (std::size_t index = 0; index < out_sections.size(); ++index) {
    const auto& sec = out_sections[index];
    add_symbol(sec.name, STB_LOCAL, STT_SECTION,
               static_cast<std::uint16_t>(index + 1), 0, 0, true);
  }

  for (const auto& symbol : slice.symbols) {
    std::uint8_t bind = symbol.bind;
    const auto is_global = bind == STB_GLOBAL;
    auto type = STT_NOTYPE;
    if (symbol.type != STT_NOTYPE) type = symbol.type;
    if (symbol.name.empty()) continue;

    std::uint16_t shndx = symbol.shndx;
    std::uint64_t value = symbol.value;
    bool local = !is_global;
    if (!symbol.defined) shndx = SHN_UNDEF;
    if (symbol.type != STT_NOTYPE) {
      type = symbol.type;
    }
    add_symbol(symbol.name, is_global ? STB_GLOBAL : STB_LOCAL, type, shndx, value, 0, !is_global);
  }

  for (const auto& relocation : slice.pending_relocs) {
    const auto sym_name = trim_copy(relocation.symbol);
    if (sym_name.empty()) continue;
    bool already = false;
    for (const auto& entry : symbol_entries) {
      if (entry.name == sym_name) {
        already = true;
        break;
      }
    }
    if (!already) {
      add_symbol(sym_name, STB_GLOBAL, STT_NOTYPE,
                 SHN_UNDEF, 0, 0, false);
    }
  }

  // Build symbol string table
  std::string strtab;
  strtab.push_back('\0');
  std::string shstrtab;
  shstrtab.push_back('\0');

  std::vector<std::uint32_t> symbol_name_offsets;
  symbol_name_offsets.reserve(symbol_entries.size());
  for (const auto& symbol : symbol_entries) {
    const auto existing_name = symbol.name;
    if (existing_name.empty()) {
      symbol_name_offsets.push_back(0);
      continue;
    }
    symbol_name_offsets.push_back(static_cast<std::uint32_t>(strtab.size()));
    strtab += existing_name;
    strtab.push_back('\0');
  }

  std::vector<std::uint32_t> section_name_offsets;
  for (const auto& section : out_sections) {
    section_name_offsets.push_back(static_cast<std::uint32_t>(shstrtab.size()));
    shstrtab += section.name;
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

  for (std::size_t index = 0; index < symbol_entries.size(); ++index) {
    const auto& symbol = symbol_entries[index];
    if (index == 0) continue;

    append_u32(symtab, symbol_name_offsets[index]);
    symtab.push_back(symbol_info(symbol.bind, symbol.type));
    symtab.push_back(0);
    append_u16(symtab, symbol.shndx);
    append_u64(symtab, symbol.value);
    append_u64(symtab, symbol.size);
  }

  std::vector<std::uint8_t> bytes;
  const auto elf_header_size = 64u;
  const auto section_header_size = 64u;

  std::size_t offset = 64;
  for (auto& section : out_sections) {
    if (section.type == SHT_NOBITS) {
      section.offset = align_up(offset, section.alignment);
      offset = section.offset;
      continue;
    }

    offset = align_up(offset, section.alignment);
    section.offset = offset;
    offset += section.bytes.size();
  }

  for (auto& section : out_sections) {
    if (section.type != SHT_RELA) continue;
    section.offset = align_up(offset, section.alignment);
    if (section.target_section_index >= out_sections.size()) {
      return {};
    }

    section.bytes.clear();

    for (const auto& relocation : out_relocs) {
      if (relocation.section != out_sections[section.target_section_index].name) continue;
      std::size_t symbol_index = 0;
      for (std::size_t idx = 0; idx < symbol_entries.size(); ++idx) {
        if (symbol_entries[idx].name == relocation.symbol) {
          symbol_index = idx;
          break;
        }
      }

      const std::uint64_t info = (static_cast<std::uint64_t>(symbol_index) << 32) |
                                 static_cast<std::uint64_t>(relocation.reloc_type);
      append_u64(section.bytes, relocation.offset);
      append_u64(section.bytes, info);
      append_i64(section.bytes, relocation.addend);
    }

    section.size = section.bytes.size();
    section.entsize = 24;
    offset = section.offset + section.size;
  }

  offset = align_up(offset, 8);
  const auto symtab_offset = align_up(offset, 8);
  const auto strtab_offset = symtab_offset + symtab.size();
  const auto shstrtab_offset = strtab_offset + strtab.size();
  offset = align_up(shstrtab_offset + shstrtab.size(), 8);
  const auto section_header_offset = offset;

  std::size_t section_count = 1 + out_sections.size() + 3;

  bytes.reserve(section_header_offset + section_count * section_header_size);
  bytes.insert(bytes.end(), 4, 0);
  bytes[0] = 0x7f;
  bytes[1] = 'E';
  bytes[2] = 'L';
  bytes[3] = 'F';
  bytes.push_back(2);
  bytes.push_back(1);
  bytes.push_back(1);
  bytes.push_back(0);
  append_zeroes(bytes, 7);

  append_u16(bytes, ET_REL);
  append_u16(bytes, EM_AARCH64);
  append_u32(bytes, 1);
  append_u64(bytes, 0);
  append_u64(bytes, 0);
  append_u64(bytes, section_header_offset);
  append_u32(bytes, 0);
  append_u16(bytes, elf_header_size);
  append_u16(bytes, 0);
  append_u16(bytes, 0);
  append_u16(bytes, section_header_size);
  append_u16(bytes, static_cast<std::uint16_t>(section_count));
  append_u16(bytes, static_cast<std::uint16_t>(section_count - 1));

  bytes.resize(section_header_offset, 0);

  for (const auto& section : out_sections) {
    if (section.type == SHT_RELA || section.type == SHT_NULL) continue;
    if (section.offset > bytes.size()) bytes.resize(section.offset, 0);
    bytes.insert(bytes.end(), section.bytes.begin(), section.bytes.end());
  }
  for (const auto& section : out_sections) {
    if (section.type != SHT_RELA) continue;
    if (section.offset > bytes.size()) bytes.resize(section.offset, 0);
    bytes.insert(bytes.end(), section.bytes.begin(), section.bytes.end());
  }

  if (bytes.size() < symtab_offset) bytes.resize(symtab_offset, 0);
  bytes.insert(bytes.end(), symtab.begin(), symtab.end());
  if (bytes.size() < strtab_offset) bytes.resize(strtab_offset, 0);
  bytes.insert(bytes.end(), strtab.begin(), strtab.end());
  if (bytes.size() < shstrtab_offset) bytes.resize(shstrtab_offset, 0);
  bytes.insert(bytes.end(), shstrtab.begin(), shstrtab.end());
  bytes.resize(section_header_offset, 0);

  append_zeroes(bytes, section_header_size);

  for (std::size_t index = 0; index < out_sections.size(); ++index) {
    const auto& section = out_sections[index];
    append_u32(bytes, static_cast<std::uint32_t>(section_name_offsets[index]));
    append_u32(bytes, section.type);
    append_u64(bytes, section.flags);
    append_u64(bytes, 0);
    append_u64(bytes, section.offset);
    append_u64(bytes, section.size);
    append_u32(bytes, section.link);
    append_u32(bytes, section.info);
    append_u64(bytes, section.addralign);
    append_u64(bytes, section.entsize);
  }

  std::uint32_t shstrtab_index = static_cast<std::uint32_t>(section_count - 1);
  const std::uint32_t symtab_index = static_cast<std::uint32_t>(section_count - 3);
  const std::uint32_t strtab_index = static_cast<std::uint32_t>(section_count - 2);

  for (auto& section : out_sections) {
    if (section.type == SHT_RELA) {
      section.link = symtab_index;
      section.info = static_cast<std::uint32_t>(section.target_section_index + 1);
    }
  }

  append_u32(bytes, static_cast<std::uint32_t>(symtab_name));
  append_u32(bytes, SHT_SYMTAB);
  append_u64(bytes, 0);
  append_u64(bytes, 0);
  append_u64(bytes, symtab_offset);
  append_u64(bytes, symtab.size());
  append_u32(bytes, symtab_index);
  append_u32(bytes, local_symbol_end);
  append_u64(bytes, 8);
  append_u64(bytes, 24);

  append_u32(bytes, static_cast<std::uint32_t>(strtab_name));
  append_u32(bytes, SHT_STRTAB);
  append_u64(bytes, 0);
  append_u64(bytes, 0);
  append_u64(bytes, strtab_offset);
  append_u64(bytes, strtab.size());
  append_u32(bytes, 0);
  append_u32(bytes, 0);
  append_u64(bytes, 1);
  append_u64(bytes, 0);

  append_u32(bytes, shstrtab_name);
  append_u32(bytes, SHT_STRTAB);
  append_u64(bytes, 0);
  append_u64(bytes, 0);
  append_u64(bytes, shstrtab_offset);
  append_u64(bytes, shstrtab.size());
  append_u32(bytes, 0);
  append_u32(bytes, 0);
  append_u64(bytes, 1);
  append_u64(bytes, 0);

  return bytes;
}

void resolve_sym_diffs() {}
void resolve_pending_exprs() {}
void resolve_pending_instructions() {}

}  // namespace

bool is_branch_reloc_type(std::uint32_t elf_type) {
  return elf_type == 279 || elf_type == 280 || elf_type == 282 || elf_type == 283;
}

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path) {
  const auto slice = parse_slice(statements);
  if (!slice.has_value()) return false;

  std::ofstream out(output_path, std::ios::binary);
  if (!out) return false;

  const auto bytes = build_elf_object(*slice);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return out.good();
}

}  // namespace c4c::backend::aarch64::assembler
