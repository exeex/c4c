#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// Parser for AT&T syntax x86-64 assembly emitted by the backend.

namespace c4c::backend::x86::assembler {

enum class SymbolKind {
  Function,
  Object,
  TlsObject,
  NoType,
};

struct SectionDirective {
  std::string name;
  std::optional<std::string> flags;
  std::optional<std::string> section_type;
  std::optional<std::string> extra;
  std::optional<std::string> comdat_group;
};

struct SizeExpr {
  enum class Kind {
    Constant,
    CurrentMinusSymbol,
    SymbolDiff,
    SymbolRef,
  } kind = Kind::Constant;
  std::uint64_t constant = 0;
  std::string lhs;
  std::string rhs;
};

struct DataValue {
  enum class Kind {
    Integer,
    Symbol,
    SymbolOffset,
    SymbolDiff,
    SymbolDiffAddend,
  } kind = Kind::Integer;
  std::int64_t integer = 0;
  std::string a;
  std::string b;
  std::int64_t addend = 0;
};

struct CfiDirective {
  enum class Kind {
    StartProc,
    EndProc,
    DefCfaOffset,
    DefCfaRegister,
    Offset,
    Other,
  } kind = Kind::Other;
  std::int32_t offset = 0;
  std::string reg;
  std::string text;
};

struct Register {
  std::string name;
};

struct ImmediateValue {
  enum class Kind {
    Integer,
    Symbol,
    SymbolPlusOffset,
    SymbolMod,
    SymbolDiff,
  } kind = Kind::Integer;
  std::int64_t integer = 0;
  std::string a;
  std::string b;
};

struct Displacement {
  enum class Kind {
    None,
    Integer,
    Symbol,
    SymbolAddend,
    SymbolMod,
    SymbolPlusOffset,
  } kind = Kind::None;
  std::int64_t integer = 0;
  std::string a;
  std::int64_t addend = 0;
};

struct MemoryOperand {
  std::optional<std::string> segment;
  Displacement displacement;
  std::optional<Register> base;
  std::optional<Register> index;
  std::optional<std::uint8_t> scale;
};

struct Operand {
  enum class Kind {
    Register,
    Immediate,
    Memory,
    Label,
    Indirect,
  } kind = Kind::Label;
  Register reg;
  ImmediateValue imm;
  MemoryOperand mem;
  std::string label;
  std::unique_ptr<Operand> indirect;
};

struct Instruction {
  std::optional<std::string> prefix;
  std::string mnemonic;
  std::vector<Operand> operands;
};

struct AsmItem {
  enum class Kind {
    Section,
    Global,
    Weak,
    Hidden,
    Protected,
    Internal,
    SymbolType,
    Size,
    Label,
    Align,
    Byte,
    Short,
    Long,
    Quad,
    Zero,
    SkipExpr,
    Asciz,
    Ascii,
    Comm,
    Set,
    Cfi,
    File,
    Loc,
    Instruction,
    OptionDirective,
    PushSection,
    PopSection,
    Previous,
    Org,
    Incbin,
    Symver,
    CodeMode,
    Empty,
  } kind = Kind::Empty;

  SectionDirective section;
  std::string text;
  std::string name;
  std::string value;
  std::string extra_text;
  SymbolKind symbol_kind = SymbolKind::NoType;
  SizeExpr size_expr;
  std::vector<DataValue> data_values;
  std::uint32_t align = 0;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
  std::uint32_t zero = 0;
  std::int64_t org_offset = 0;
  std::uint8_t fill_byte = 0;
  std::string path;
  std::uint64_t skip = 0;
  std::optional<std::uint64_t> count;
  CfiDirective cfi;
  Instruction instruction;
  std::string raw_operands;
  std::string op;
};

static const char* COMMENT_STYLE = "#";

std::string strip_comment(std::string_view line) {
  bool in_string = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      in_string = !in_string;
      continue;
    }
    if (!in_string && c == '#') {
      return std::string(line.substr(0, i));
    }
  }
  return std::string(line);
}

std::string trim_asm(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::vector<std::string> split_on_semicolons(std::string_view line) {
  std::vector<std::string> parts;
  std::string current;
  bool in_string = false;
  for (char c : line) {
    if (c == '"') {
      in_string = !in_string;
      current.push_back(c);
      continue;
    }
    if (c == ';' && !in_string) {
      parts.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  if (!current.empty()) {
    parts.push_back(current);
  }
  return parts;
}

bool is_ident_char(unsigned char c) {
  return std::isalnum(c) || c == '_';
}

bool is_label_like(std::string_view s) {
  if (s.empty()) return false;
  const unsigned char first = static_cast<unsigned char>(s.front());
  if (!(std::isalpha(first) || first == '_' || first == '.' || std::isdigit(first))) {
    return false;
  }
  for (unsigned char c : s) {
    if (!(std::isalnum(c) || c == '_' || c == '.')) return false;
  }
  return true;
}

std::string strip_outer_parens(std::string_view s) {
  auto text = trim_asm(s);
  if (text.size() < 2 || text.front() != '(' || text.back() != ')') {
    return text;
  }
  std::string inner = text.substr(1, text.size() - 2);
  int depth = 0;
  for (char c : inner) {
    if (c == '(') ++depth;
    if (c == ')') {
      --depth;
      if (depth < 0) return text;
    }
  }
  return depth == 0 ? strip_outer_parens(inner) : text;
}

std::string strip_sym_parens(std::string_view s) {
  auto text = trim_asm(s);
  if (text.size() < 2 || text.front() != '(' || text.back() != ')') {
    return text;
  }
  std::string inner = text.substr(1, text.size() - 2);
  int depth = 0;
  for (char c : inner) {
    if (c == '(') ++depth;
    if (c == ')') {
      --depth;
      if (depth < 0) return text;
    }
  }
  return depth == 0 ? strip_sym_parens(inner) : text;
}

std::optional<Register> make_reg(std::string name) {
  if (name.empty()) return std::nullopt;
  return Register{std::move(name)};
}

std::vector<std::string> split_operands(std::string_view s) {
  std::vector<std::string> parts;
  std::string current;
  int depth = 0;
  for (char c : s) {
    if (c == '(') {
      ++depth;
      current.push_back(c);
    } else if (c == ')') {
      --depth;
      current.push_back(c);
    } else if (c == ',' && depth == 0) {
      parts.push_back(trim_asm(current));
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  if (!current.empty()) {
    parts.push_back(trim_asm(current));
  }
  return parts;
}

std::pair<std::string, std::string> split_mnemonic_operands(std::string_view line) {
  const auto pos = line.find_first_of(" \t");
  if (pos == std::string_view::npos) {
    return {std::string(line), std::string()};
  }
  return {std::string(line.substr(0, pos)), trim_asm(line.substr(pos))};
}

// The expression evaluator is shared with the broader backend expression layer.
std::optional<std::int64_t> parse_integer_expr(std::string_view s) {
  if (s.empty()) return std::nullopt;
  std::int64_t sign = 1;
  std::size_t i = 0;
  if (s.front() == '+') {
    ++i;
  } else if (s.front() == '-') {
    sign = -1;
    ++i;
  }
  std::int64_t value = 0;
  if (i >= s.size()) return std::nullopt;
  if (s.substr(i, 2) == "0x") {
    i += 2;
    for (; i < s.size(); ++i) {
      const char c = s[i];
      if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
      value *= 16;
      if (c >= '0' && c <= '9') value += c - '0';
      else if (c >= 'a' && c <= 'f') value += 10 + (c - 'a');
      else if (c >= 'A' && c <= 'F') value += 10 + (c - 'A');
    }
    return sign * value;
  }
  for (; i < s.size(); ++i) {
    const char c = s[i];
    if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    value = value * 10 + (c - '0');
  }
  return sign * value;
}

std::pair<std::string, std::uint8_t> split_skip_args(std::string_view args) {
  int depth = 0;
  std::optional<std::size_t> last_comma;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const char c = args[i];
    if (c == '(') ++depth;
    else if (c == ')') --depth;
    else if (c == ',' && depth == 0) last_comma = i;
  }
  if (last_comma) {
    const auto expr = trim_asm(args.substr(0, *last_comma));
    const auto fill_str = trim_asm(args.substr(*last_comma + 1));
    return {expr, static_cast<std::uint8_t>(parse_integer_expr(fill_str).value_or(0))};
  }
  return {trim_asm(args), 0};
}

std::optional<DataValue> parse_label_diff(std::string_view s) {
  for (std::size_t i = 1; i < s.size(); ++i) {
    if (s[i] == '-') {
      const auto lhs = trim_asm(s.substr(0, i));
      const auto rhs = trim_asm(s.substr(i + 1));
      if (!lhs.empty() && !rhs.empty() && is_label_like(lhs) && is_label_like(rhs)) {
        DataValue dv;
        dv.kind = DataValue::Kind::SymbolDiff;
        dv.a = lhs;
        dv.b = rhs;
        return dv;
      }
    }
  }
  return std::nullopt;
}

std::optional<DataValue> parse_symbol_offset(std::string_view s) {
  for (std::size_t i = 1; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '+' || c == '-') {
      const auto left = trim_asm(s.substr(0, i));
      const auto right = trim_asm(s.substr(i));
      if (auto off = parse_integer_expr(right); off && !left.empty()) {
        DataValue dv;
        dv.kind = DataValue::Kind::SymbolOffset;
        dv.a = left;
        dv.addend = *off;
        return dv;
      }
    }
  }
  return std::nullopt;
}

std::vector<DataValue> parse_data_values(std::string_view s) {
  std::vector<DataValue> vals;
  for (const auto& part : split_operands(s)) {
    const auto trimmed = strip_outer_parens(part);
    if (trimmed.empty()) continue;
    if (auto diff = parse_label_diff(trimmed)) {
      vals.push_back(*diff);
      continue;
    }
    if (auto val = parse_integer_expr(trimmed)) {
      DataValue dv;
      dv.kind = DataValue::Kind::Integer;
      dv.integer = *val;
      vals.push_back(dv);
      continue;
    }
    if (auto sym = parse_symbol_offset(trimmed)) {
      vals.push_back(*sym);
      continue;
    }
    DataValue dv;
    dv.kind = DataValue::Kind::Symbol;
    dv.a = trimmed;
    vals.push_back(dv);
  }
  return vals;
}

Operand parse_operand(std::string_view s);

Operand parse_register_operand(std::string_view s) {
  const std::string name = std::string(s.substr(1));
  Operand op;
  op.kind = Operand::Kind::Register;
  op.reg = Register{name};
  return op;
}

Operand parse_immediate_operand(std::string_view s) {
  Operand op;
  op.kind = Operand::Kind::Immediate;
  if (auto val = parse_integer_expr(s)) {
    op.imm.kind = ImmediateValue::Kind::Integer;
    op.imm.integer = *val;
    return op;
  }
  const auto at = s.find('@');
  if (at != std::string_view::npos) {
    op.imm.kind = ImmediateValue::Kind::SymbolMod;
    op.imm.a = std::string(s.substr(0, at));
    op.imm.b = std::string(s.substr(at + 1));
    return op;
  }
  op.imm.kind = ImmediateValue::Kind::Symbol;
  op.imm.a = std::string(s);
  return op;
}

Displacement parse_displacement(std::string_view s);

MemoryOperand parse_memory_inner(std::string_view s) {
  MemoryOperand mem;
  const auto close = s.rfind(')');
  if (close == std::string_view::npos) {
    mem.displacement = parse_displacement(s);
    return mem;
  }
  int depth = 0;
  std::optional<std::size_t> open;
  for (std::size_t i = close + 1; i-- > 0;) {
    const char c = s[i];
    if (c == ')') {
      ++depth;
    } else if (c == '(') {
      --depth;
      if (depth == 0) {
        open = i;
        break;
      }
    }
    if (i == 0) break;
  }
  if (!open) {
    mem.displacement = parse_displacement(s);
    return mem;
  }

  const std::string disp_str = trim_asm(s.substr(0, *open));
  const std::string inner = std::string(s.substr(*open + 1, close - *open - 1));
  if (disp_str.empty() && !inner.empty()) {
    const auto first = trim_asm(inner.substr(0, inner.find(',')));
    if (!first.empty() && first.front() != '%') {
      mem.displacement = parse_displacement(inner);
      return mem;
    }
  }

  mem.displacement = parse_displacement(disp_str);
  const auto parts = split_operands(inner);
  if (!parts.empty() && !parts[0].empty()) {
    std::string name = parts[0];
    if (!name.empty() && name.front() == '%') name.erase(name.begin());
    mem.base = Register{name};
  }
  if (parts.size() > 1 && !parts[1].empty()) {
    std::string name = parts[1];
    if (!name.empty() && name.front() == '%') name.erase(name.begin());
    mem.index = Register{name};
  }
  if (parts.size() > 2 && !parts[2].empty()) {
    mem.scale = static_cast<std::uint8_t>(std::stoi(parts[2]));
  }
  return mem;
}

Operand parse_memory_operand(std::string_view s) {
  Operand op;
  op.kind = Operand::Kind::Memory;
  if (!s.empty() && s.front() == '%' && s.find(':') != std::string_view::npos) {
    const auto colon = s.find(':');
    const auto seg = s.substr(1, colon - 1);
    if (seg == "fs" || seg == "gs") {
      op.mem = parse_memory_inner(s.substr(colon + 1));
      op.mem.segment = std::string(seg);
      return op;
    }
  }
  op.mem = parse_memory_inner(s);
  return op;
}

Displacement parse_displacement(std::string_view s) {
  Displacement disp;
  const auto text = trim_asm(s);
  if (text.empty()) {
    disp.kind = Displacement::Kind::None;
    return disp;
  }
  if (auto val = parse_integer_expr(text)) {
    disp.kind = Displacement::Kind::Integer;
    disp.integer = *val;
    return disp;
  }
  const auto at = text.find('@');
  if (at != std::string_view::npos) {
    disp.kind = Displacement::Kind::SymbolMod;
    disp.a = std::string(text.substr(0, at));
    return disp;
  }
  const auto plus = text.find_first_of("+-", 1);
  if (plus != std::string_view::npos) {
    const auto lhs = trim_asm(text.substr(0, plus));
    const auto rhs = trim_asm(text.substr(plus));
    if (auto offset = parse_integer_expr(rhs); offset && !lhs.empty()) {
      disp.kind = Displacement::Kind::SymbolPlusOffset;
      disp.a = lhs;
      disp.addend = *offset;
      return disp;
    }
  }
  disp.kind = Displacement::Kind::Symbol;
  disp.a = text;
  return disp;
}

Operand parse_operand(std::string_view s) {
  const auto text = trim_asm(s);
  if (text.empty()) {
    return Operand{};
  }
  if (text.front() == '*') {
    Operand op;
    op.kind = Operand::Kind::Indirect;
    op.indirect = std::make_unique<Operand>(parse_operand(text.substr(1)));
    return op;
  }
  if (text.front() == '%') return parse_register_operand(text);
  if (text.front() == '$') return parse_immediate_operand(text.substr(1));
  if (text.find('(') != std::string_view::npos || text.find(':') != std::string_view::npos) {
    return parse_memory_operand(text);
  }
  if (auto val = parse_integer_expr(text)) {
    Operand op;
    op.kind = Operand::Kind::Memory;
    op.mem.displacement.kind = Displacement::Kind::Integer;
    op.mem.displacement.integer = *val;
    return op;
  }
  Operand op;
  op.kind = Operand::Kind::Label;
  op.label = std::string(text);
  return op;
}

std::vector<Operand> parse_operands(std::string_view raw_operands) {
  std::vector<Operand> ops;
  for (const auto& part : split_operands(raw_operands)) {
    if (!part.empty()) {
      ops.push_back(parse_operand(part));
    }
  }
  return ops;
}

std::optional<std::pair<std::string, std::string_view>> try_parse_label(std::string_view line) {
  const auto trimmed = trim_asm(line);
  const auto colon = trimmed.find(':');
  if (colon == std::string_view::npos) return std::nullopt;
  const auto candidate = trim_asm(trimmed.substr(0, colon));
  if (candidate.empty() || candidate.find(' ') != std::string::npos ||
      candidate.find('\t') != std::string::npos || candidate.find(',') != std::string::npos ||
      candidate.front() == '$' || candidate.front() == '%') {
    return std::nullopt;
  }
  return std::make_pair(candidate, std::string_view(trimmed).substr(colon + 1));
}

std::vector<AsmItem> parse_line_items(std::string_view line);

AsmItem parse_section_directive(std::string_view args) {
  AsmItem item;
  item.kind = AsmItem::Kind::Section;
  item.section.name = ".text";
  const auto parts = split_operands(args);
  if (!parts.empty() && !parts[0].empty()) item.section.name = trim_asm(parts[0]);
  if (parts.size() > 1 && !parts[1].empty()) item.section.flags = trim_asm(parts[1]);
  if (parts.size() > 2 && !parts[2].empty()) item.section.section_type = trim_asm(parts[2]);
  if (parts.size() > 3 && !parts[3].empty()) item.section.extra = trim_asm(parts[3]);
  if (item.section.flags && item.section.flags->find('G') != std::string::npos &&
      item.section.extra) {
    item.section.comdat_group = item.section.extra;
  }
  return item;
}

AsmItem parse_type_directive(std::string_view args) {
  const auto comma = args.find(',');
  const std::string name = trim_asm(comma == std::string_view::npos ? args : args.substr(0, comma));
  const std::string kind_str = trim_asm(comma == std::string_view::npos ? std::string_view() : args.substr(comma + 1));
  AsmItem item;
  item.kind = AsmItem::Kind::SymbolType;
  item.name = name;
  if (kind_str == "@function" || kind_str == "%function" || kind_str == "STT_FUNC") {
    item.symbol_kind = SymbolKind::Function;
  } else if (kind_str == "@object" || kind_str == "%object" || kind_str == "STT_OBJECT") {
    item.symbol_kind = SymbolKind::Object;
  } else if (kind_str == "@tls_object" || kind_str == "%tls_object" || kind_str == "STT_TLS") {
    item.symbol_kind = SymbolKind::TlsObject;
  } else {
    item.symbol_kind = SymbolKind::NoType;
  }
  return item;
}

AsmItem parse_size_directive(std::string_view args) {
  const auto comma = args.find(',');
  AsmItem item;
  item.kind = AsmItem::Kind::Size;
  item.name = trim_asm(comma == std::string_view::npos ? args : args.substr(0, comma));
  const std::string expr = trim_asm(comma == std::string_view::npos ? std::string_view() : args.substr(comma + 1));
  if (expr.rfind(".-", 0) == 0 || expr.rfind(". -", 0) == 0) {
    item.size_expr.kind = SizeExpr::Kind::CurrentMinusSymbol;
    item.size_expr.lhs = trim_asm(expr.substr(2));
  } else if (auto val = parse_integer_expr(expr)) {
    item.size_expr.kind = SizeExpr::Kind::Constant;
    item.size_expr.constant = static_cast<std::uint64_t>(*val);
  } else {
    item.size_expr.kind = SizeExpr::Kind::SymbolRef;
    item.size_expr.lhs = expr;
  }
  return item;
}

AsmItem parse_comm_directive(std::string_view args) {
  const auto parts = split_operands(args);
  AsmItem item;
  item.kind = AsmItem::Kind::Comm;
  if (!parts.empty()) item.name = trim_asm(parts[0]);
  if (parts.size() > 1) item.align = static_cast<std::uint32_t>(parse_integer_expr(parts[1]).value_or(0));
  if (parts.size() > 2) item.zero = static_cast<std::uint32_t>(parse_integer_expr(parts[2]).value_or(1));
  return item;
}

AsmItem parse_set_directive(std::string_view args) {
  const auto comma = args.find(',');
  AsmItem item;
  item.kind = AsmItem::Kind::Set;
  item.name = trim_asm(comma == std::string_view::npos ? args : args.substr(0, comma));
  item.value = trim_asm(comma == std::string_view::npos ? std::string_view() : args.substr(comma + 1));
  return item;
}

AsmItem parse_symver_directive(std::string_view args) {
  const auto comma = args.find(',');
  AsmItem item;
  item.kind = AsmItem::Kind::Symver;
  item.name = trim_asm(comma == std::string_view::npos ? args : args.substr(0, comma));
  item.value = trim_asm(comma == std::string_view::npos ? std::string_view() : args.substr(comma + 1));
  return item;
}

AsmItem parse_instruction(std::string_view line, std::optional<std::string> prefix);

AsmItem parse_prefixed_instruction(std::string_view line) {
  const auto split = split_mnemonic_operands(line);
  return parse_instruction(split.second, split.first);
}

AsmItem parse_instruction(std::string_view line, std::optional<std::string> prefix) {
  AsmItem item;
  item.kind = AsmItem::Kind::Instruction;
  item.instruction.prefix = std::move(prefix);
  const auto split = split_mnemonic_operands(line);
  item.instruction.mnemonic = split.first;
  item.instruction.operands = split.second.empty() ? std::vector<Operand>() : parse_operands(split.second);
  item.op = item.instruction.mnemonic;
  item.raw_operands = std::string(split.second);
  return item;
}

AsmItem parse_directive(std::string_view line) {
  const auto parts = split_mnemonic_operands(line);
  const auto& directive = parts.first;
  const auto& args = parts.second;
  if (directive == ".section") return parse_section_directive(args);
  if (directive == ".text" || directive == ".data" || directive == ".bss" || directive == ".rodata") {
    AsmItem item;
    item.kind = AsmItem::Kind::Section;
    item.section.name = directive;
    return item;
  }
  if (directive == ".globl" || directive == ".global") {
    AsmItem item; item.kind = AsmItem::Kind::Global; item.name = trim_asm(args); return item;
  }
  if (directive == ".weak") { AsmItem item; item.kind = AsmItem::Kind::Weak; item.name = trim_asm(args); return item; }
  if (directive == ".hidden") { AsmItem item; item.kind = AsmItem::Kind::Hidden; item.name = trim_asm(args); return item; }
  if (directive == ".protected") { AsmItem item; item.kind = AsmItem::Kind::Protected; item.name = trim_asm(args); return item; }
  if (directive == ".internal") { AsmItem item; item.kind = AsmItem::Kind::Internal; item.name = trim_asm(args); return item; }
  if (directive == ".type") return parse_type_directive(args);
  if (directive == ".size") return parse_size_directive(args);
  if (directive == ".comm") return parse_comm_directive(args);
  if (directive == ".set") return parse_set_directive(args);
  if (directive == ".symver") return parse_symver_directive(args);
  if (directive == ".align" || directive == ".p2align" || directive == ".balign") {
    AsmItem item; item.kind = AsmItem::Kind::Align; item.align = static_cast<std::uint32_t>(parse_integer_expr(split_operands(args).empty() ? args : split_operands(args)[0]).value_or(1)); return item;
  }
  if (directive == ".byte" || directive == ".short" || directive == ".value" ||
      directive == ".2byte" || directive == ".word" || directive == ".hword" ||
      directive == ".long" || directive == ".4byte" || directive == ".int" ||
      directive == ".quad" || directive == ".8byte") {
    AsmItem item;
    item.kind = directive == ".byte" ? AsmItem::Kind::Byte :
                (directive == ".short" || directive == ".value" || directive == ".2byte" ||
                 directive == ".word" || directive == ".hword") ? AsmItem::Kind::Short :
                (directive == ".long" || directive == ".4byte" || directive == ".int") ? AsmItem::Kind::Long :
                AsmItem::Kind::Quad;
    item.data_values = parse_data_values(args);
    return item;
  }
  if (directive == ".zero" || directive == ".skip") {
    const auto [expr, fill] = split_skip_args(args);
    AsmItem item;
    item.kind = fill == 0 ? AsmItem::Kind::Zero : AsmItem::Kind::SkipExpr;
    item.value = expr;
    item.fill_byte = fill;
    if (auto val = parse_integer_expr(expr)) item.zero = static_cast<std::uint32_t>(*val);
    return item;
  }
  if (directive == ".asciz" || directive == ".string") { AsmItem item; item.kind = AsmItem::Kind::Asciz; item.value = std::string(args); return item; }
  if (directive == ".ascii") { AsmItem item; item.kind = AsmItem::Kind::Ascii; item.value = std::string(args); return item; }
  if (directive == ".cfi_startproc") { AsmItem item; item.kind = AsmItem::Kind::Cfi; item.cfi.kind = CfiDirective::Kind::StartProc; return item; }
  if (directive == ".cfi_endproc") { AsmItem item; item.kind = AsmItem::Kind::Cfi; item.cfi.kind = CfiDirective::Kind::EndProc; return item; }
  if (directive == ".file") { AsmItem item; item.kind = AsmItem::Kind::File; item.value = std::string(args); return item; }
  if (directive == ".loc") { AsmItem item; item.kind = AsmItem::Kind::Loc; item.value = std::string(args); return item; }
  if (directive == ".pushsection") { AsmItem item; item.kind = AsmItem::Kind::PushSection; item.section = std::move(parse_section_directive(args).section); return item; }
  if (directive == ".popsection") { AsmItem item; item.kind = AsmItem::Kind::PopSection; return item; }
  if (directive == ".previous") { AsmItem item; item.kind = AsmItem::Kind::Previous; return item; }
  if (directive == ".org") {
    AsmItem item;
    item.kind = AsmItem::Kind::Org;
    item.value = std::string(args);
    if (auto val = parse_integer_expr(args)) {
      item.org_offset = *val;
    }
    return item;
  }
  if (directive == ".code16" || directive == ".code16gcc") { AsmItem item; item.kind = AsmItem::Kind::CodeMode; item.align = 16; return item; }
  if (directive == ".code32") { AsmItem item; item.kind = AsmItem::Kind::CodeMode; item.align = 32; return item; }
  if (directive == ".code64") { AsmItem item; item.kind = AsmItem::Kind::CodeMode; item.align = 64; return item; }
  if (directive == ".option") { AsmItem item; item.kind = AsmItem::Kind::OptionDirective; item.value = std::string(args); return item; }
  if (directive == ".incbin") { AsmItem item; item.kind = AsmItem::Kind::Incbin; item.value = std::string(args); return item; }
  AsmItem item;
  item.kind = AsmItem::Kind::Empty;
  return item;
}

std::vector<AsmItem> parse_line_items(std::string_view line) {
  std::vector<AsmItem> items;
  std::string_view rest = trim_asm(line);
  if (auto label = try_parse_label(rest)) {
    AsmItem item;
    item.kind = AsmItem::Kind::Label;
    item.name = label->first;
    items.push_back(std::move(item));
    rest = label->second;
  }
  if (rest.empty()) return items;
  if (rest.find('=') != std::string_view::npos) {
    const auto eq = rest.find('=');
    const auto before = trim_asm(rest.substr(0, eq));
    if (!before.empty() && before.find(' ') == std::string::npos && before.find('\t') == std::string::npos &&
        before.front() != '$' && before.front() != '%' &&
        (std::isalpha(static_cast<unsigned char>(before.front())) || before.front() == '_')) {
      AsmItem item;
      item.kind = AsmItem::Kind::Set;
      item.name = before;
      item.value = trim_asm(rest.substr(eq + 1));
      items.push_back(std::move(item));
      return items;
    }
  }
  while (auto label2 = try_parse_label(rest)) {
    AsmItem item;
    item.kind = AsmItem::Kind::Label;
    item.name = label2->first;
    items.push_back(std::move(item));
    rest = label2->second;
  }
  if (rest.empty()) return items;
  if (rest.front() == '.') {
    items.push_back(parse_directive(rest));
  } else if (rest.rfind("lock ", 0) == 0 || rest.rfind("rep ", 0) == 0 ||
             rest.rfind("repz ", 0) == 0 || rest.rfind("repe ", 0) == 0 ||
             rest.rfind("repnz ", 0) == 0 || rest.rfind("repne ", 0) == 0 ||
             rest.rfind("notrack ", 0) == 0) {
    items.push_back(parse_prefixed_instruction(rest));
  } else {
    items.push_back(parse_instruction(rest, std::nullopt));
  }
  return items;
}

std::vector<AsmItem> expand_rept_blocks(const std::vector<std::string>& lines) {
  std::vector<AsmItem> unused;
  (void)unused;
  std::vector<std::string> result;
  std::size_t i = 0;
  int irp_depth = 0;
  while (i < lines.size()) {
    const auto trimmed = trim_asm(strip_comment(lines[i]));
    if (trimmed.rfind(".rept ", 0) == 0 || trimmed.rfind(".rept\t", 0) == 0) {
      const auto count_str = trim_asm(trimmed.substr(5));
      const auto count = static_cast<std::size_t>(parse_integer_expr(count_str).value_or(0));
      std::vector<std::string> body;
      std::size_t depth = 1;
      ++i;
      while (i < lines.size()) {
        const auto inner = trim_asm(strip_comment(lines[i]));
        if (inner.rfind(".rept ", 0) == 0 || inner.rfind(".rept\t", 0) == 0) {
          ++depth;
        } else if (inner == ".endr") {
          --depth;
          if (depth == 0) break;
        }
        body.push_back(lines[i]);
        ++i;
      }
      for (std::size_t n = 0; n < count; ++n) {
        result.insert(result.end(), body.begin(), body.end());
      }
    } else if (trimmed.rfind(".irp ", 0) == 0 || trimmed.rfind(".irp\t", 0) == 0) {
      ++irp_depth;
      result.push_back(lines[i]);
    } else if (trimmed == ".endr") {
      if (irp_depth > 0) {
        --irp_depth;
        result.push_back(lines[i]);
      }
    } else {
      result.push_back(lines[i]);
    }
    ++i;
  }

  std::vector<AsmItem> items;
  for (const auto& line : result) {
    const auto trimmed = trim_asm(strip_comment(line));
    if (trimmed.empty()) {
      AsmItem item;
      item.kind = AsmItem::Kind::Empty;
      items.push_back(std::move(item));
      continue;
    }
    const auto parts = split_on_semicolons(trimmed);
    for (const auto& part : parts) {
      const auto p = trim_asm(part);
      if (p.empty()) continue;
      const auto line_items = parse_line_items(p);
      items.insert(items.end(), line_items.begin(), line_items.end());
    }
  }
  return items;
}

std::vector<AsmItem> parse_asm(const std::string& text) {
  std::vector<std::string> lines;
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    const auto parts = split_on_semicolons(line);
    if (parts.size() > 1) {
      for (const auto& part : parts) {
        const auto p = trim_asm(part);
        if (!p.empty()) lines.push_back(p);
      }
    } else {
      lines.push_back(line);
    }
  }
  return expand_rept_blocks(lines);
}

std::vector<std::string> expand_gas_macros(const std::vector<std::string>& lines) {
  return lines;
}

struct GasMacro;

std::vector<std::string> expand_gas_macros_with_state(
    const std::vector<std::string>& lines,
    std::unordered_map<std::string, GasMacro>& macros,
    std::unordered_map<std::string, std::int64_t>& symbols) {
  (void)macros;
  (void)symbols;
  return lines;
}

struct GasMacro {
  std::vector<std::pair<std::string, std::optional<std::string>>> params;
  std::vector<std::string> body;
};

std::pair<std::string, std::vector<std::pair<std::string, std::optional<std::string>>>>
parse_macro_def(const std::string& rest) {
  std::stringstream ss(rest);
  std::string name;
  ss >> name;
  std::vector<std::pair<std::string, std::optional<std::string>>> params;
  std::string token;
  while (ss >> token) {
    if (!token.empty() && token.back() == ',') token.pop_back();
    const auto eq = token.find('=');
    if (eq != std::string::npos) {
      params.emplace_back(token.substr(0, eq), token.substr(eq + 1));
    } else {
      params.emplace_back(token, std::nullopt);
    }
  }
  return {name, params};
}

std::vector<std::pair<std::string, std::string>>
parse_macro_args(const std::string& args_str,
                 const std::vector<std::pair<std::string, std::optional<std::string>>>& params) {
  std::vector<std::pair<std::string, std::string>> result;
  for (const auto& [name, default_value] : params) {
    result.emplace_back(name, default_value.value_or(std::string()));
  }
  (void)args_str;
  return result;
}

std::vector<std::string> split_macro_args(const std::string& s) {
  return split_operands(s);
}

std::pair<std::string, std::vector<std::string>> parse_irp_header(const std::string& rest) {
  const auto comma = rest.find(',');
  const std::string var = trim_asm(comma == std::string::npos ? rest : rest.substr(0, comma));
  std::vector<std::string> items;
  if (comma != std::string::npos) {
    for (const auto& part : split_operands(rest.substr(comma + 1))) {
      if (!part.empty()) items.push_back(trim_asm(part));
    }
  }
  return {var, items};
}

bool eval_ifc(const std::string& rest) {
  const auto comma = rest.find(',');
  if (comma == std::string::npos) return false;
  return trim_asm(rest.substr(0, comma)) == trim_asm(rest.substr(comma + 1));
}

std::string resolve_set_expr(const std::string& expr,
                             const std::unordered_map<std::string, std::int64_t>& symbols) {
  std::string result = expr;
  for (const auto& [name, val] : symbols) {
    const auto replacement = std::to_string(val);
    std::size_t pos = 0;
    while ((pos = result.find(name, pos)) != std::string::npos) {
      result.replace(pos, name.size(), replacement);
      pos += replacement.size();
    }
  }
  return result;
}

std::string replace_whole_word(const std::string& text, const std::string& word, const std::string& replacement) {
  return text == word ? replacement : text;
}

bool is_if_start(const std::string& trimmed) {
  return trimmed.rfind(".if ", 0) == 0 || trimmed.rfind(".if\t", 0) == 0 || trimmed.rfind(".if(", 0) == 0 ||
         trimmed.rfind(".ifc ", 0) == 0 || trimmed.rfind(".ifc\t", 0) == 0 ||
         trimmed.rfind(".ifdef ", 0) == 0 || trimmed.rfind(".ifndef ", 0) == 0;
}

bool eval_if_expr(const std::string& expr, const std::unordered_map<std::string, std::int64_t>& symbols) {
  (void)symbols;
  return parse_integer_expr(expr).value_or(0) != 0;
}

}  // namespace c4c::backend::x86::assembler
