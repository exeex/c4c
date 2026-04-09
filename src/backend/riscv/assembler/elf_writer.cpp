#include <cctype>
#include <cstdint>
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

struct Operand {
  std::string text;
};

struct DataValue {
  enum class Kind {
    Integer,
    Symbol,
    SymbolDiff,
    Expression,
  };

  Kind kind = Kind::Integer;
  std::int64_t integer = 0;
  std::string name;
  std::string sym_a;
  std::string sym_b;
  std::int64_t addend = 0;
  std::string expr;
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

struct SizeExpr {
  enum class Kind {
    CurrentMinus,
    Absolute,
  };

  Kind kind = Kind::CurrentMinus;
  std::string label;
  std::uint64_t absolute = 0;
};

struct SectionInfo {
  std::string name;
  std::string flags;
  std::string sec_type;
  bool flags_explicit = false;
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
  };

  Kind kind = Kind::Ignored;
  SectionInfo section;
  std::string text;
  std::vector<DataValue> values;
  std::string symbol;
  Visibility visibility = Visibility::Hidden;
  SymbolType symbol_type = SymbolType::NoType;
  SizeExpr size_expr;
  std::uint64_t number = 0;
  std::uint64_t size = 0;
  std::uint8_t fill = 0;
  std::string path;
  std::uint64_t skip = 0;
  std::optional<std::uint64_t> count;
  std::string args;
};

struct AsmStatement {
  enum class Kind {
    Label,
    Directive,
    Instruction,
    Empty,
  };

  Kind kind = Kind::Empty;
  std::string label;
  Directive directive;
  std::string mnemonic;
  std::vector<Operand> operands;
  std::string raw_operands;
};

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
    (void)statements;
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

  void write_elf(const std::string& output_path) const {
    (void)output_path;
  }

 private:
  std::uint32_t elf_flags_ = EF_RISCV_FLOAT_ABI_DOUBLE | EF_RISCV_RVC;
  std::uint8_t elf_class_ = 2;
  bool no_relax_ = false;
  std::vector<bool> option_stack_;
  std::vector<PendingReloc> pending_branch_relocs_;
  std::vector<DeferredExpr> deferred_exprs_;
};

// Direct translations of the Rust helpers above are intentionally kept local
// to this file so later slices can promote them into shared headers only when
// the surrounding RV64 backend has been translated.

}  // namespace c4c::backend::riscv::assembler

