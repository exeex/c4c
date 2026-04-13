#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace c4c::backend::riscv::assembler {

namespace {

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

std::string strip_c_comments(const std::string& text) {
  std::string out;
  out.reserve(text.size());

  bool in_block = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (!in_block && i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*') {
      in_block = true;
      ++i;
      continue;
    }
    if (in_block && i + 1 < text.size() && text[i] == '*' && text[i + 1] == '/') {
      in_block = false;
      ++i;
      continue;
    }
    if (!in_block) {
      out.push_back(text[i]);
    }
  }
  return out;
}

std::string strip_comment(std::string_view line) {
  std::size_t hash = line.find('#');
  std::size_t slash = line.find("//");
  std::size_t cut = std::min(hash == std::string_view::npos ? line.size() : hash,
                             slash == std::string_view::npos ? line.size() : slash);
  return std::string(line.substr(0, cut));
}

std::vector<std::string> split_on_semicolons(std::string_view line) {
  std::vector<std::string> parts;
  std::string current;
  bool in_quote = false;
  for (char ch : line) {
    if (ch == '"') {
      in_quote = !in_quote;
      current.push_back(ch);
      continue;
    }
    if (ch == ';' && !in_quote) {
      parts.push_back(trim_asm(current));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  parts.push_back(trim_asm(current));
  return parts;
}

std::string to_lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return text;
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool is_numeric_label_ref(std::string_view text) {
  if (text.size() < 2) return false;
  const char suffix = text.back();
  if (suffix != 'f' && suffix != 'F' && suffix != 'b' && suffix != 'B') return false;
  return std::all_of(text.begin(), text.end() - 1, [](unsigned char c) {
    return std::isdigit(c) != 0;
  });
}

bool is_register(std::string_view text) {
  const auto s = to_lower(std::string(text));
  return s == "zero" || s == "ra" || s == "sp" || s == "gp" || s == "tp" ||
         s == "t0" || s == "t1" || s == "t2" || s == "t3" || s == "t4" ||
         s == "t5" || s == "t6" || s == "s0" || s == "s1" || s == "s2" ||
         s == "s3" || s == "s4" || s == "s5" || s == "s6" || s == "s7" ||
         s == "s8" || s == "s9" || s == "s10" || s == "s11" || s == "a0" ||
         s == "a1" || s == "a2" || s == "a3" || s == "a4" || s == "a5" ||
         s == "a6" || s == "a7" || s == "fp" || s == "ft0" || s == "ft1" ||
         s == "ft2" || s == "ft3" || s == "ft4" || s == "ft5" || s == "ft6" ||
         s == "ft7" || s == "ft8" || s == "ft9" || s == "ft10" || s == "ft11" ||
         s == "fs0" || s == "fs1" || s == "fs2" || s == "fs3" || s == "fs4" ||
         s == "fs5" || s == "fs6" || s == "fs7" || s == "fs8" || s == "fs9" ||
         s == "fs10" || s == "fs11" || s == "fa0" || s == "fa1" || s == "fa2" ||
         s == "fa3" || s == "fa4" || s == "fa5" || s == "fa6" || s == "fa7" ||
         (s.size() >= 2 && s.front() == 'x' &&
          std::all_of(s.begin() + 1, s.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) &&
          std::stoul(s.substr(1)) <= 31) ||
         (s.size() >= 2 && s.front() == 'f' && !starts_with(s, "ft") && !starts_with(s, "fs") &&
          !starts_with(s, "fa") &&
          std::all_of(s.begin() + 1, s.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) &&
          std::stoul(s.substr(1)) <= 31) ||
         (s.size() >= 2 && s.front() == 'v' &&
          std::all_of(s.begin() + 1, s.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) &&
          std::stoul(s.substr(1)) <= 31);
}

bool is_fence_arg(std::string_view text) {
  const auto s = to_lower(std::string(text));
  if (s.empty() || s.size() > 4) return false;
  return std::all_of(s.begin(), s.end(), [](char c) {
    return c == 'i' || c == 'o' || c == 'r' || c == 'w';
  });
}

bool is_rounding_mode(std::string_view text) {
  const auto s = to_lower(std::string(text));
  return s == "rne" || s == "rtz" || s == "rdn" || s == "rup" || s == "rmm" || s == "dyn";
}

std::int64_t parse_int_literal(std::string_view text) {
  const auto s = trim_asm(text);
  if (s.empty()) {
    throw std::runtime_error("empty integer literal");
  }

  if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
    std::int64_t value = 0;
    for (char ch : s.substr(2)) {
      if (ch != '0' && ch != '1') {
        throw std::runtime_error("invalid binary integer literal: " + s);
      }
      value = (value << 1) | (ch - '0');
    }
    return value;
  }

  std::size_t parsed = 0;
  std::int64_t value = 0;
  try {
    value = std::stoll(s, &parsed, 0);
  } catch (const std::exception&) {
    throw std::runtime_error("invalid integer literal: " + s);
  }
  if (parsed != s.size()) {
    throw std::runtime_error("invalid integer literal: " + s);
  }
  return value;
}

std::pair<std::string, std::int64_t> parse_symbol_addend(std::string_view text) {
  const auto s = trim_asm(text);
  const auto plus_pos = s.find('+');
  if (plus_pos != std::string::npos) {
    return {trim_asm(s.substr(0, plus_pos)), parse_int_literal(s.substr(plus_pos + 1))};
  }

  const auto minus_pos = s.find('-', 1);
  if (minus_pos != std::string::npos) {
    return {trim_asm(s.substr(0, minus_pos)), -parse_int_literal(s.substr(minus_pos + 1))};
  }

  return {s, 0};
}

std::optional<std::size_t> find_symbol_diff_minus(std::string_view expr) {
  int depth = 0;
  for (std::size_t i = 0; i < expr.size(); ++i) {
    const char ch = expr[i];
    if (ch == '(') {
      ++depth;
    } else if (ch == ')') {
      --depth;
    } else if (ch == '-' && depth == 0 && i > 0 && i + 1 < expr.size()) {
      return i;
    }
  }
  return std::nullopt;
}

std::string strip_outer_parens(std::string_view text) {
  auto s = trim_asm(text);
  while (s.size() >= 2 && s.front() == '(' && s.back() == ')') {
    std::string_view inner(s.data() + 1, s.size() - 2);
    int depth = 0;
    bool balanced = true;
    for (char ch : inner) {
      if (ch == '(') {
        ++depth;
      } else if (ch == ')') {
        --depth;
        if (depth < 0) {
          balanced = false;
          break;
        }
      }
    }
    if (!balanced || depth != 0) break;
    s = trim_asm(inner);
  }
  return s;
}

DataValue parse_single_data_value(std::string_view text) {
  auto s = trim_asm(text);
  if (s.empty()) {
    return DataValue{.kind = DataValue::Kind::Integer, .integer = 0};
  }

  s = strip_outer_parens(s);
  const char first = s.empty() ? '\0' : s.front();
  const bool is_sym_start = std::isalpha(static_cast<unsigned char>(first)) || first == '_' ||
                            first == '.';
  const bool could_be_numeric_ref = std::isdigit(static_cast<unsigned char>(first)) != 0;
  const bool starts_with_paren = first == '(';
  const bool has_complex_ops = s.find('/') != std::string::npos || s.find('*') != std::string::npos ||
                               s.find("<<") != std::string::npos || s.find(">>") != std::string::npos;

  if (!has_complex_ops && (is_sym_start || could_be_numeric_ref || starts_with_paren)) {
    if (const auto minus_pos = find_symbol_diff_minus(s)) {
      const auto sym_a_raw = strip_outer_parens(trim_asm(s.substr(0, *minus_pos)));
      const auto rest = trim_asm(s.substr(*minus_pos + 1));

      std::string sym_b_raw = rest;
      std::int64_t addend = 0;
      if (const auto plus_pos = rest.find('+'); plus_pos != std::string::npos) {
        sym_b_raw = trim_asm(rest.substr(0, plus_pos));
        addend = parse_int_literal(rest.substr(plus_pos + 1));
      }

      const auto sym_b = strip_outer_parens(sym_b_raw);
      if (!sym_b.empty()) {
        const char b_first = sym_b.front();
        const bool b_is_sym = std::isalpha(static_cast<unsigned char>(b_first)) || b_first == '_' ||
                              b_first == '.';
        if (b_is_sym || is_numeric_label_ref(sym_b)) {
          return DataValue{.kind = DataValue::Kind::SymbolDiff,
                           .sym_a = sym_a_raw,
                           .sym_b = sym_b,
                           .addend = addend};
        }
      }
    }

    if (is_sym_start) {
      const auto [sym, addend] = parse_symbol_addend(s);
      return DataValue{.kind = DataValue::Kind::Symbol, .name = sym, .addend = addend};
    }
    if (could_be_numeric_ref && is_numeric_label_ref(s)) {
      return DataValue{.kind = DataValue::Kind::Symbol, .name = std::string(s), .addend = 0};
    }
    if (starts_with_paren) {
      const auto inner = strip_outer_parens(s);
      if (inner != s) {
        return parse_single_data_value(inner);
      }
    }
  }

  if (could_be_numeric_ref || first == '(') {
    for (std::size_t i = 1; i < s.size(); ++i) {
      if (s[i] != '+') continue;
      const auto left = trim_asm(s.substr(0, i));
      const auto right = trim_asm(s.substr(i + 1));
      try {
        const auto offset = parse_int_literal(left);
        const char r_first = right.empty() ? '\0' : right.front();
        if (std::isalpha(static_cast<unsigned char>(r_first)) || r_first == '_' || r_first == '.') {
          const auto [sym, extra_addend] = parse_symbol_addend(right);
          return DataValue{.kind = DataValue::Kind::Symbol,
                           .name = sym,
                           .addend = offset + extra_addend};
        }
      } catch (const std::exception&) {
      }
    }
  }

  try {
    return DataValue{.kind = DataValue::Kind::Integer, .integer = parse_int_literal(s)};
  } catch (const std::exception&) {
    return DataValue{.kind = DataValue::Kind::Expression, .expr = std::string(s)};
  }
}

std::vector<DataValue> parse_data_values(std::string_view text) {
  std::vector<DataValue> values;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const auto comma = text.find(',', pos);
    const auto token = text.substr(pos, comma == std::string_view::npos ? std::string_view::npos
                                                                        : comma - pos);
    const auto trimmed = trim_asm(token);
    if (!trimmed.empty()) {
      values.push_back(parse_single_data_value(trimmed));
    }
    if (comma == std::string_view::npos) break;
    pos = comma + 1;
  }
  return values;
}

Operand make_operand(Operand::Kind kind,
                     std::string text,
                     std::int64_t value = 0,
                     std::string base = {},
                     std::string symbol = {},
                     std::string modifier = {});

Operand parse_single_operand(std::string_view text, bool is_fence, bool force_symbol) {
  const auto s = trim_asm(text);
  if (s.empty()) {
    throw std::runtime_error("empty operand");
  }

  if (!force_symbol) {
    const auto last_close = s.rfind(')');
    if (last_close == s.size() - 1 && last_close != std::string::npos) {
      int depth = 0;
      std::optional<std::size_t> last_open;
      for (std::size_t i = s.size(); i-- > 0;) {
        const char ch = s[i];
        if (ch == ')') {
          ++depth;
        } else if (ch == '(') {
          --depth;
          if (depth == 0) {
            last_open = i;
            break;
          }
        }
      }
      if (last_open) {
        const auto base = trim_asm(s.substr(*last_open + 1, last_close - *last_open - 1));
        const auto offset_part = trim_asm(s.substr(0, *last_open));
        if (is_register(base)) {
          if (!offset_part.empty() && offset_part.front() == '%') {
            return make_operand(Operand::Kind::MemSymbol, std::string(s), 0, base, offset_part, "");
          }
          if (offset_part.empty()) {
            return make_operand(Operand::Kind::Mem, std::string(s), 0, base);
          }
          try {
            return make_operand(Operand::Kind::Mem,
                                std::string(s),
                                parse_int_literal(offset_part),
                                base);
          } catch (const std::exception&) {
            return make_operand(Operand::Kind::MemSymbol, std::string(s), 0, base, offset_part, "");
          }
        }
      }
    }
  }

  if (starts_with(s, "%hi(") || starts_with(s, "%lo(") || starts_with(s, "%pcrel_hi(") ||
      starts_with(s, "%pcrel_lo(") || starts_with(s, "%tprel_hi(") ||
      starts_with(s, "%tprel_lo(") || starts_with(s, "%tprel_add(") ||
      starts_with(s, "%got_pcrel_hi(") || starts_with(s, "%tls_ie_pcrel_hi(") ||
      starts_with(s, "%tls_gd_pcrel_hi(")) {
    return make_operand(Operand::Kind::Symbol, std::string(s));
  }

  if (is_fence && is_fence_arg(s)) {
    return make_operand(Operand::Kind::FenceArg, std::string(s));
  }

  if (!force_symbol && is_rounding_mode(s)) {
    return make_operand(Operand::Kind::RoundingMode, std::string(s));
  }

  if (!force_symbol && is_register(s)) {
    return make_operand(Operand::Kind::Reg, std::string(s));
  }

  if (!force_symbol) {
    try {
      return make_operand(Operand::Kind::Imm, std::string(s), parse_int_literal(s));
    } catch (const std::exception&) {
    }
  }

  if (const auto plus_pos = s.find('+'); plus_pos != std::string::npos) {
    try {
      const auto off = parse_int_literal(s.substr(plus_pos + 1));
      return make_operand(Operand::Kind::SymbolOffset,
                          std::string(s),
                          off,
                          "",
                          trim_asm(s.substr(0, plus_pos)));
    } catch (const std::exception&) {
    }
  }
  if (const auto minus_pos = s.rfind('-'); minus_pos != std::string::npos && minus_pos > 0) {
    try {
      const auto off = parse_int_literal(s.substr(minus_pos));
      return make_operand(Operand::Kind::SymbolOffset,
                          std::string(s),
                          off,
                          "",
                          trim_asm(s.substr(0, minus_pos)));
    } catch (const std::exception&) {
    }
  }

  return make_operand(Operand::Kind::Symbol, std::string(s));
}

Operand make_operand(Operand::Kind kind,
                     std::string text,
                     std::int64_t value,
                     std::string base,
                     std::string symbol,
                     std::string modifier) {
  Operand operand;
  operand.kind = kind;
  operand.text = std::move(text);
  operand.value = value;
  operand.base = std::move(base);
  operand.symbol = std::move(symbol);
  operand.modifier = std::move(modifier);
  return operand;
}

std::vector<Operand> parse_operands(std::string_view s, std::string_view mnemonic) {
  if (s.empty()) return {};
  const bool is_fence = mnemonic == "fence";
  const auto lower = to_lower(std::string(mnemonic));
  auto symbol_operand_mask = [&]() -> std::uint8_t {
    if (lower == "call" || lower == "tail") return 0b0000'0001;
    if (lower == "la" || lower == "lla") return 0b0000'0010;
    if (lower == "jump") return 0b0000'0001;
    return 0;
  }();

  std::vector<Operand> operands;
  std::string current;
  int paren_depth = 0;
  std::uint8_t operand_idx = 0;
  for (char ch : s) {
    if (ch == '(') {
      ++paren_depth;
      current.push_back(ch);
    } else if (ch == ')') {
      --paren_depth;
      current.push_back(ch);
    } else if (ch == ',' && paren_depth == 0) {
      const bool force_symbol = (symbol_operand_mask & (1u << operand_idx)) != 0;
      operands.push_back(parse_single_operand(trim_asm(current), is_fence, force_symbol));
      current.clear();
      ++operand_idx;
    } else {
      current.push_back(ch);
    }
  }
  const auto trimmed = trim_asm(current);
  if (!trimmed.empty()) {
    const bool force_symbol = (symbol_operand_mask & (1u << operand_idx)) != 0;
    operands.push_back(parse_single_operand(trimmed, is_fence, force_symbol));
  }
  return operands;
}

SectionInfo parse_section_args(std::string_view args) {
  std::vector<std::string> parts;
  std::size_t pos = 0;
  while (pos <= args.size()) {
    const auto comma = args.find(',', pos);
    const auto token = args.substr(pos, comma == std::string_view::npos ? std::string_view::npos
                                                                        : comma - pos);
    parts.push_back(trim_asm(token));
    if (comma == std::string_view::npos) break;
    pos = comma + 1;
  }

  const auto name = parts.empty() ? std::string() : trim_asm(parts[0]);
  SectionInfo info;
  info.name = name.empty() ? name : std::string(name.begin(), name.end());
  if (!info.name.empty() && info.name.front() == '"' && info.name.back() == '"') {
    info.name = info.name.substr(1, info.name.size() - 2);
  }
  info.flags_explicit = parts.size() > 1;
  if (info.flags_explicit) {
    info.flags = parts[1];
    if (info.flags.size() >= 2 && info.flags.front() == '"' && info.flags.back() == '"') {
      info.flags = info.flags.substr(1, info.flags.size() - 2);
    }
  }
  if (parts.size() > 2) {
    info.sec_type = parts[2];
  } else if (info.name == ".bss" || starts_with(info.name, ".bss.") || starts_with(info.name, ".tbss")) {
    info.sec_type = "@nobits";
  } else {
    info.sec_type = "@progbits";
  }
  return info;
}

Directive parse_type_directive(std::string_view args) {
  Directive out;
  out.kind = Directive::Kind::Type;
  const auto comma = args.find(',');
  if (comma == std::string_view::npos) {
    out.sym = trim_asm(args);
    out.symbol_type = SymbolType::NoType;
    return out;
  }
  out.sym = trim_asm(args.substr(0, comma));
  const auto ty = trim_asm(args.substr(comma + 1));
  if (ty == "%function" || ty == "@function") {
    out.symbol_type = SymbolType::Function;
  } else if (ty == "%object" || ty == "@object") {
    out.symbol_type = SymbolType::Object;
  } else if (ty == "@tls_object") {
    out.symbol_type = SymbolType::TlsObject;
  } else {
    out.symbol_type = SymbolType::NoType;
  }
  return out;
}

Directive parse_size_directive(std::string_view args) {
  Directive out;
  out.kind = Directive::Kind::Size;
  const auto comma = args.find(',');
  if (comma == std::string_view::npos) {
    out.sym = trim_asm(args);
    out.size.kind = SizeExpr::Kind::Absolute;
    out.size.absolute = 0;
    return out;
  }
  out.sym = trim_asm(args.substr(0, comma));
  const auto expr = trim_asm(args.substr(comma + 1));
  out.size.kind = SizeExpr::Kind::Absolute;
  if (starts_with(expr, ".-")) {
    out.size.kind = SizeExpr::Kind::CurrentMinus;
    out.size.symbol = trim_asm(expr.substr(2));
    return out;
  }
  try {
    out.size.absolute = static_cast<std::uint64_t>(parse_int_literal(expr));
  } catch (const std::exception&) {
    out.size.absolute = 0;
  }
  return out;
}

Directive parse_directive(std::string_view line) {
  const auto split = line.find_first_of(" \t");
  const auto name = split == std::string_view::npos ? line : line.substr(0, split);
  const std::string args =
      split == std::string_view::npos ? std::string() : trim_asm(line.substr(split + 1));

  Directive out;
  if (name == ".section") {
    out.kind = Directive::Kind::Section;
    out.section = parse_section_args(args);
  } else if (name == ".text") {
    out.kind = Directive::Kind::Text;
  } else if (name == ".data") {
    out.kind = Directive::Kind::Data;
  } else if (name == ".bss") {
    out.kind = Directive::Kind::Bss;
  } else if (name == ".rodata") {
    out.kind = Directive::Kind::Rodata;
  } else if (name == ".globl" || name == ".global") {
    out.kind = Directive::Kind::Globl;
    out.sym = trim_asm(args);
  } else if (name == ".weak") {
    out.kind = Directive::Kind::Weak;
    out.sym = trim_asm(args);
  } else if (name == ".hidden" || name == ".protected" || name == ".internal") {
    out.kind = Directive::Kind::SymVisibility;
    out.sym = trim_asm(args);
    out.visibility = name == ".hidden" ? Visibility::Hidden
                         : name == ".protected" ? Visibility::Protected
                                                : Visibility::Internal;
  } else if (name == ".type") {
    return parse_type_directive(args);
  } else if (name == ".size") {
    return parse_size_directive(args);
  } else if (name == ".align" || name == ".p2align") {
    out.kind = Directive::Kind::Align;
    const auto first = trim_asm(args.substr(0, args.find(',')));
    try {
      out.align = static_cast<std::uint64_t>(parse_int_literal(first));
    } catch (const std::exception&) {
      out.align = 0;
    }
  } else if (name == ".balign") {
    out.kind = Directive::Kind::Balign;
    const auto first = trim_asm(args.substr(0, args.find(',')));
    try {
      out.align = static_cast<std::uint64_t>(parse_int_literal(first));
    } catch (const std::exception&) {
      out.align = 1;
    }
  } else if (name == ".byte") {
    out.kind = Directive::Kind::Byte;
    out.data_values = parse_data_values(args);
  } else if (name == ".short" || name == ".hword" || name == ".2byte" || name == ".half") {
    out.kind = Directive::Kind::Short;
    out.data_values = parse_data_values(args);
  } else if (name == ".long" || name == ".4byte" || name == ".word" || name == ".int") {
    out.kind = Directive::Kind::Long;
    out.data_values = parse_data_values(args);
  } else if (name == ".quad" || name == ".8byte" || name == ".xword" || name == ".dword") {
    out.kind = Directive::Kind::Quad;
    out.data_values = parse_data_values(args);
  } else if (name == ".zero" || name == ".space") {
    out.kind = Directive::Kind::Zero;
    const auto comma = args.find(',');
    try {
      out.zero_size = static_cast<std::size_t>(parse_int_literal(trim_asm(args.substr(0, comma))));
    } catch (const std::exception&) {
      throw std::runtime_error("invalid .zero size: " + std::string(args));
    }
    if (comma != std::string_view::npos) {
      out.zero_fill = static_cast<std::uint8_t>(parse_int_literal(trim_asm(args.substr(comma + 1))));
    }
  } else if (name == ".fill") {
    out.kind = Directive::Kind::Ascii;
    std::vector<std::string> parts;
    std::size_t pos = 0;
    while (pos <= args.size()) {
      const auto comma = args.find(',', pos);
      const auto token = args.substr(pos, comma == std::string_view::npos ? std::string_view::npos
                                                                          : comma - pos);
      parts.push_back(trim_asm(token));
      if (comma == std::string_view::npos) break;
      pos = comma + 1;
    }
    const auto repeat = parts.empty() ? 0 : static_cast<std::size_t>(parse_int_literal(parts[0]));
    const auto size = parts.size() > 1 ? static_cast<std::size_t>(parse_int_literal(parts[1])) : 1;
    const auto value = parts.size() > 2 ? static_cast<std::uint64_t>(parse_int_literal(parts[2])) : 0;
    const auto total_bytes = repeat * std::min<std::size_t>(size, 8);
    if (value == 0) {
      out.kind = Directive::Kind::Zero;
      out.zero_size = total_bytes;
      out.zero_fill = 0;
    } else {
      out.bytes.reserve(total_bytes);
      const auto value_bytes = value;
      for (std::size_t r = 0; r < repeat; ++r) {
        for (std::size_t j = 0; j < std::min<std::size_t>(size, 8); ++j) {
          out.bytes.push_back(static_cast<std::uint8_t>((value_bytes >> (j * 8)) & 0xff));
        }
      }
    }
  } else if (name == ".asciz" || name == ".string") {
    out.kind = Directive::Kind::Asciz;
    out.text = std::string(args);
  } else if (name == ".ascii") {
    out.kind = Directive::Kind::Ascii;
    out.text = std::string(args);
  } else if (name == ".comm") {
    out.kind = Directive::Kind::Comm;
    std::vector<std::string> parts;
    std::size_t pos = 0;
    while (pos <= args.size()) {
      const auto comma = args.find(',', pos);
      const auto token = args.substr(pos, comma == std::string_view::npos ? std::string_view::npos
                                                                          : comma - pos);
      parts.push_back(trim_asm(token));
      if (comma == std::string_view::npos) break;
      pos = comma + 1;
    }
    out.sym = parts.empty() ? std::string() : parts[0];
    out.comm_size = parts.size() >= 2 ? static_cast<std::uint64_t>(parse_int_literal(parts[1])) : 0;
    out.comm_align = parts.size() > 2 ? static_cast<std::uint64_t>(parse_int_literal(parts[2])) : 1;
  } else if (name == ".local") {
    out.kind = Directive::Kind::Local;
    out.sym = trim_asm(args);
  } else if (name == ".set" || name == ".equ") {
    out.kind = Directive::Kind::Set;
    const auto comma = args.find(',');
    out.sym = trim_asm(args.substr(0, comma));
    out.value = comma == std::string_view::npos ? std::string() : trim_asm(args.substr(comma + 1));
  } else if (name == ".symver") {
    const auto comma = args.find(',');
    if (comma != std::string_view::npos) {
      const auto sym_name = trim_asm(args.substr(0, comma));
      const auto ver_string = trim_asm(args.substr(comma + 1));
      const auto at = ver_string.find('@');
      if (at != std::string::npos && !trim_asm(ver_string.substr(0, at)).empty()) {
        out.kind = Directive::Kind::Set;
        out.sym = trim_asm(ver_string.substr(0, at));
        out.value = sym_name;
      } else {
        out.kind = Directive::Kind::Ignored;
      }
    } else {
      out.kind = Directive::Kind::Ignored;
    }
  } else if (name == ".option") {
    out.kind = Directive::Kind::ArchOption;
    out.text = std::string(args);
  } else if (name == ".attribute") {
    out.kind = Directive::Kind::Attribute;
    out.text = std::string(args);
  } else if (name == ".insn") {
    out.kind = Directive::Kind::Insn;
    out.text = std::string(args);
  } else if (name == ".cfi_startproc" || name == ".cfi_endproc" || name == ".cfi_def_cfa_offset" ||
             name == ".cfi_offset" || name == ".cfi_def_cfa_register" || name == ".cfi_restore" ||
             name == ".cfi_remember_state" || name == ".cfi_restore_state" ||
             name == ".cfi_adjust_cfa_offset" || name == ".cfi_def_cfa" ||
             name == ".cfi_sections" || name == ".cfi_personality" || name == ".cfi_lsda" ||
             name == ".cfi_rel_offset" || name == ".cfi_register" || name == ".cfi_return_column" ||
             name == ".cfi_undefined" || name == ".cfi_same_value" || name == ".cfi_escape" ||
             name == ".cfi_signal_frame") {
    out.kind = Directive::Kind::Cfi;
  } else if (name == ".pushsection") {
    out.kind = Directive::Kind::PushSection;
    out.section = parse_section_args(args);
  } else if (name == ".popsection") {
    out.kind = Directive::Kind::PopSection;
  } else if (name == ".previous") {
    out.kind = Directive::Kind::Previous;
  } else if (name == ".org" || name == ".file" || name == ".loc" || name == ".ident" ||
             name == ".addrsig" || name == ".addrsig_sym" || name == ".build_attributes" ||
             name == ".eabi_attribute" || name == ".end" || name == ".altmacro" ||
             name == ".noaltmacro" || name == ".purgem") {
    out.kind = Directive::Kind::Ignored;
  } else if (name == ".incbin") {
    out.kind = Directive::Kind::Incbin;
    std::vector<std::string> parts;
    std::size_t pos = 0;
    while (pos <= args.size()) {
      const auto comma = args.find(',', pos);
      const auto token = args.substr(pos, comma == std::string_view::npos ? std::string_view::npos
                                                                          : comma - pos);
      parts.push_back(trim_asm(token));
      if (comma == std::string_view::npos) break;
      pos = comma + 1;
    }
    out.path = parts.empty() ? std::string() : trim_asm(parts[0]);
    out.skip = parts.size() > 1 ? static_cast<std::uint64_t>(parse_int_literal(parts[1])) : 0;
    if (parts.size() > 2) {
      out.count = static_cast<std::uint64_t>(parse_int_literal(parts[2]));
    }
  } else if (name == ".subsection") {
    out.kind = Directive::Kind::Subsection;
    out.integer = static_cast<std::uint64_t>(parse_int_literal(args));
  } else {
    out.kind = Directive::Kind::Unknown;
    out.unknown_name = std::string(name);
    out.unknown_args = std::string(args);
  }

  return out;
}

std::vector<AsmStatement> parse_line(std::string_view line);

std::vector<AsmStatement> parse_line(std::string_view line) {
  if (const auto colon_pos = line.find(':'); colon_pos != std::string_view::npos) {
    const auto potential_label = trim_asm(line.substr(0, colon_pos));
    if (!potential_label.empty() && potential_label.find(' ') == std::string::npos &&
        potential_label.find('\t') == std::string::npos &&
        (!starts_with(potential_label, ".") || starts_with(potential_label, ".L") ||
         starts_with(potential_label, ".l"))) {
      if (!starts_with(potential_label, ".") || starts_with(potential_label, ".L") ||
          starts_with(potential_label, ".l")) {
        std::vector<AsmStatement> result;
        result.push_back(AsmStatement{.kind = AsmStatement::Kind::Label,
                                      .text = std::string(potential_label),
                                      .mnemonic = std::string(potential_label)});
        const auto rest = trim_asm(line.substr(colon_pos + 1));
        if (!rest.empty()) {
          const auto tail = parse_line(rest);
          result.insert(result.end(), tail.begin(), tail.end());
        }
        return result;
      }
    }
  }

  const auto trimmed = trim_asm(line);
  if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
    const auto inner = trim_asm(trimmed.substr(1, trimmed.size() - 2));
    if (inner.empty()) {
      return {AsmStatement{.kind = AsmStatement::Kind::Empty}};
    }
    return parse_line(inner);
  }

  if (!trimmed.empty() && trimmed.front() == '.') {
    return {AsmStatement{.kind = AsmStatement::Kind::Directive,
                         .text = std::string(trimmed),
                         .mnemonic = std::string(trimmed.substr(0, trimmed.find_first_of(" \t"))),
                         .directive = parse_directive(trimmed)}};
  }

  const auto split = trimmed.find_first_of(" \t");
  const auto mnemonic = split == std::string_view::npos ? trimmed : trimmed.substr(0, split);
  const std::string operands_str =
      split == std::string_view::npos ? std::string() : trim_asm(trimmed.substr(split + 1));
  return {AsmStatement{.kind = AsmStatement::Kind::Instruction,
                       .text = std::string(trimmed),
                       .mnemonic = to_lower(std::string(mnemonic)),
                       .operands = parse_operands(operands_str, mnemonic),
                       .raw_operands = std::string(operands_str)}};
}

}  // namespace

std::vector<AsmStatement> parse_asm(const std::string& text) {
  const auto stripped = strip_c_comments(text);
  std::vector<AsmStatement> statements;
  std::stringstream stream(stripped);
  std::string line;

  while (std::getline(stream, line)) {
    const auto trimmed = trim_asm(strip_comment(line));
    if (trimmed.empty()) {
      statements.push_back(AsmStatement{.kind = AsmStatement::Kind::Empty});
      continue;
    }

    const auto parts = split_on_semicolons(trimmed);
    for (const auto& part : parts) {
      if (part.empty()) continue;
      const auto stmts = parse_line(part);
      statements.insert(statements.end(), stmts.begin(), stmts.end());
    }
  }

  return statements;
}

}  // namespace c4c::backend::riscv::assembler
