// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/peephole.rs
// RISC-V 64-bit peephole optimizer for assembly text.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::codegen {
namespace {

enum class LineKindTag {
  Nop,
  StoreS0,
  LoadS0,
  Move,
  LoadImm,
  Jump,
  Branch,
  Label,
  Ret,
  Call,
  Directive,
  Alu,
  SextW,
  LoadAddr,
  Other,
};

struct LineKind {
  LineKindTag tag = LineKindTag::Nop;
  std::uint8_t reg = 255;
  std::uint8_t dst = 255;
  std::uint8_t src = 255;
  std::int32_t offset = 0;
  bool is_word = false;
};

struct SlotMapping {
  std::uint8_t reg = 255;
  bool is_word = false;
  bool active = false;
};

constexpr std::uint8_t REG_NONE = 255;
constexpr std::uint8_t REG_T0 = 0;
constexpr std::uint8_t REG_T1 = 1;
constexpr std::uint8_t REG_T2 = 2;
constexpr std::uint8_t REG_T3 = 3;
constexpr std::uint8_t REG_T4 = 4;
constexpr std::uint8_t REG_T5 = 5;
constexpr std::uint8_t REG_T6 = 6;
constexpr std::uint8_t REG_S0 = 10;
constexpr std::uint8_t REG_S1 = 11;
constexpr std::uint8_t REG_S2 = 12;
constexpr std::uint8_t REG_S3 = 13;
constexpr std::uint8_t REG_S4 = 14;
constexpr std::uint8_t REG_S5 = 15;
constexpr std::uint8_t REG_S6 = 16;
constexpr std::uint8_t REG_S7 = 17;
constexpr std::uint8_t REG_S8 = 18;
constexpr std::uint8_t REG_S9 = 19;
constexpr std::uint8_t REG_S10 = 20;
constexpr std::uint8_t REG_S11 = 21;
constexpr std::uint8_t REG_A0 = 30;
constexpr std::uint8_t REG_A1 = 31;
constexpr std::uint8_t REG_A2 = 32;
constexpr std::uint8_t REG_A3 = 33;
constexpr std::uint8_t REG_A4 = 34;
constexpr std::uint8_t REG_A5 = 35;
constexpr std::uint8_t REG_A6 = 36;
constexpr std::uint8_t REG_A7 = 37;
constexpr std::uint8_t REG_RA = 40;
constexpr std::uint8_t REG_SP = 41;
constexpr std::size_t NUM_REGS = 42;
constexpr std::size_t MAX_SLOT_ENTRIES = 64;

bool is_word_char(char ch) {
  const unsigned char uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) != 0 || ch == '_' || ch == '.';
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool ends_with(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.substr(text.size() - suffix.size()) == suffix;
}

std::string trim_copy(std::string_view text) {
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

std::vector<std::string> split_operands(std::string_view rest) {
  std::vector<std::string> operands;
  std::size_t start = 0;
  while (start <= rest.size()) {
    const std::size_t comma = rest.find(',', start);
    const std::size_t end = comma == std::string_view::npos ? rest.size() : comma;
    operands.push_back(trim_copy(rest.substr(start, end - start)));
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
    if (start < rest.size() && rest[start] == ' ') {
      ++start;
    }
  }
  if (operands.size() == 1 && operands.front().empty()) {
    operands.clear();
  }
  return operands;
}

std::string join_operands(const std::vector<std::string>& operands) {
  std::string out;
  for (std::size_t i = 0; i < operands.size(); ++i) {
    if (i != 0) {
      out += ", ";
    }
    out += operands[i];
  }
  return out;
}

bool is_branch_like(std::string_view mnemonic) {
  return mnemonic == "beq" || mnemonic == "bne" || mnemonic == "bge" ||
         mnemonic == "blt" || mnemonic == "bgeu" || mnemonic == "bltu" ||
         mnemonic == "bnez" || mnemonic == "beqz";
}

bool is_store_like(std::string_view mnemonic) {
  return mnemonic == "sd" || mnemonic == "sw";
}

bool is_jump_like(std::string_view mnemonic) {
  return mnemonic == "jump" || mnemonic == "j";
}

bool is_call_like(std::string_view mnemonic) {
  return mnemonic == "call" || starts_with(mnemonic, "jal");
}

std::optional<std::int32_t> parse_s0_offset(std::string_view addr);
std::optional<std::int32_t> extract_s0_offset_from_line(std::string_view line);

std::uint8_t parse_reg(std::string_view name) {
  if (name == "t0") return REG_T0;
  if (name == "t1") return REG_T1;
  if (name == "t2") return REG_T2;
  if (name == "t3") return REG_T3;
  if (name == "t4") return REG_T4;
  if (name == "t5") return REG_T5;
  if (name == "t6") return REG_T6;
  if (name == "s0") return REG_S0;
  if (name == "s1") return REG_S1;
  if (name == "s2") return REG_S2;
  if (name == "s3") return REG_S3;
  if (name == "s4") return REG_S4;
  if (name == "s5") return REG_S5;
  if (name == "s6") return REG_S6;
  if (name == "s7") return REG_S7;
  if (name == "s8") return REG_S8;
  if (name == "s9") return REG_S9;
  if (name == "s10") return REG_S10;
  if (name == "s11") return REG_S11;
  if (name == "a0") return REG_A0;
  if (name == "a1") return REG_A1;
  if (name == "a2") return REG_A2;
  if (name == "a3") return REG_A3;
  if (name == "a4") return REG_A4;
  if (name == "a5") return REG_A5;
  if (name == "a6") return REG_A6;
  if (name == "a7") return REG_A7;
  if (name == "ra") return REG_RA;
  if (name == "sp") return REG_SP;
  return REG_NONE;
}

const char* reg_name(std::uint8_t id) {
  switch (id) {
    case REG_T0: return "t0";
    case REG_T1: return "t1";
    case REG_T2: return "t2";
    case REG_T3: return "t3";
    case REG_T4: return "t4";
    case REG_T5: return "t5";
    case REG_T6: return "t6";
    case REG_S0: return "s0";
    case REG_S1: return "s1";
    case REG_S2: return "s2";
    case REG_S3: return "s3";
    case REG_S4: return "s4";
    case REG_S5: return "s5";
    case REG_S6: return "s6";
    case REG_S7: return "s7";
    case REG_S8: return "s8";
    case REG_S9: return "s9";
    case REG_S10: return "s10";
    case REG_S11: return "s11";
    case REG_A0: return "a0";
    case REG_A1: return "a1";
    case REG_A2: return "a2";
    case REG_A3: return "a3";
    case REG_A4: return "a4";
    case REG_A5: return "a5";
    case REG_A6: return "a6";
    case REG_A7: return "a7";
    case REG_RA: return "ra";
    case REG_SP: return "sp";
    default: return "??";
  }
}

LineKind make_nop() { return {}; }

LineKind make_store(std::uint8_t reg, std::int32_t offset, bool is_word) {
  LineKind kind;
  kind.tag = LineKindTag::StoreS0;
  kind.reg = reg;
  kind.offset = offset;
  kind.is_word = is_word;
  return kind;
}

LineKind make_load(std::uint8_t reg, std::int32_t offset, bool is_word) {
  LineKind kind;
  kind.tag = LineKindTag::LoadS0;
  kind.reg = reg;
  kind.offset = offset;
  kind.is_word = is_word;
  return kind;
}

LineKind make_move(std::uint8_t dst, std::uint8_t src) {
  LineKind kind;
  kind.tag = LineKindTag::Move;
  kind.dst = dst;
  kind.src = src;
  return kind;
}

LineKind make_load_imm(std::uint8_t dst) {
  LineKind kind;
  kind.tag = LineKindTag::LoadImm;
  kind.dst = dst;
  return kind;
}

LineKind make_jump() {
  LineKind kind;
  kind.tag = LineKindTag::Jump;
  return kind;
}

LineKind make_branch() {
  LineKind kind;
  kind.tag = LineKindTag::Branch;
  return kind;
}

LineKind make_label() {
  LineKind kind;
  kind.tag = LineKindTag::Label;
  return kind;
}

LineKind make_ret() {
  LineKind kind;
  kind.tag = LineKindTag::Ret;
  return kind;
}

LineKind make_call() {
  LineKind kind;
  kind.tag = LineKindTag::Call;
  return kind;
}

LineKind make_directive() {
  LineKind kind;
  kind.tag = LineKindTag::Directive;
  return kind;
}

LineKind make_alu() {
  LineKind kind;
  kind.tag = LineKindTag::Alu;
  return kind;
}

LineKind make_sext_w(std::uint8_t dst, std::uint8_t src) {
  LineKind kind;
  kind.tag = LineKindTag::SextW;
  kind.dst = dst;
  kind.src = src;
  return kind;
}

LineKind make_load_addr(std::uint8_t dst) {
  LineKind kind;
  kind.tag = LineKindTag::LoadAddr;
  kind.dst = dst;
  return kind;
}

LineKind make_other() {
  LineKind kind;
  kind.tag = LineKindTag::Other;
  return kind;
}

bool is_nop(const LineKind& kind) { return kind.tag == LineKindTag::Nop; }

bool has_whole_word(std::string_view text, std::string_view needle) {
  if (needle.empty()) {
    return false;
  }

  std::size_t pos = text.find(needle);
  while (pos != std::string_view::npos) {
    const bool left_ok = pos == 0 || !is_word_char(text[pos - 1]);
    const std::size_t after = pos + needle.size();
    const bool right_ok = after >= text.size() || !is_word_char(text[after]);
    if (left_ok && right_ok) {
      return true;
    }
    pos = text.find(needle, pos + 1);
  }
  return false;
}

std::string replace_whole_word(std::string_view text,
                               std::string_view needle,
                               std::string_view replacement) {
  if (needle.empty()) {
    return std::string(text);
  }

  std::string out;
  std::size_t cursor = 0;
  while (cursor < text.size()) {
    const std::size_t pos = text.find(needle, cursor);
    if (pos == std::string_view::npos) {
      out.append(text.substr(cursor));
      break;
    }

    const bool left_ok = pos == 0 || !is_word_char(text[pos - 1]);
    const std::size_t after = pos + needle.size();
    const bool right_ok = after >= text.size() || !is_word_char(text[after]);
    if (left_ok && right_ok) {
      out.append(text.substr(cursor, pos - cursor));
      out.append(replacement);
      cursor = after;
    } else {
      out.append(text.substr(cursor, pos - cursor + 1));
      cursor = pos + 1;
    }
  }
  return out;
}

std::optional<std::string> replace_source_reg_in_instruction(std::string_view line,
                                                             std::string_view dst_name,
                                                             std::string_view src_name) {
  const std::size_t first_non_ws = line.find_first_not_of(" \t\r");
  if (first_non_ws == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t prefix_len = first_non_ws;
  std::string_view trimmed = line.substr(first_non_ws);
  const std::size_t space_pos = trimmed.find(' ');
  if (space_pos == std::string_view::npos) {
    return std::nullopt;
  }

  const std::string_view mnemonic = trimmed.substr(0, space_pos);
  const std::string_view rest = trimmed.substr(space_pos + 1);
  auto operands = split_operands(rest);
  if (operands.empty()) {
    return std::nullopt;
  }

  const bool replace_all = is_store_like(mnemonic) || is_branch_like(mnemonic) ||
                           is_jump_like(mnemonic) || is_call_like(mnemonic);
  bool changed = false;
  for (std::size_t i = 0; i < operands.size(); ++i) {
    if (!replace_all && i == 0) {
      continue;
    }
    const std::string replaced = replace_whole_word(operands[i], dst_name, src_name);
    if (replaced != operands[i]) {
      operands[i] = replaced;
      changed = true;
    }
  }

  if (!changed) {
    return std::nullopt;
  }

  std::string out(line.substr(0, prefix_len));
  out.append(mnemonic);
  out.push_back(' ');
  out.append(join_operands(operands));
  return out;
}

LineKind classify_line(std::string_view line) {
  const std::string trimmed = trim_copy(line);
  if (trimmed.empty()) {
    return make_nop();
  }

  if (trimmed.front() == '.') {
    if (!trimmed.empty() && trimmed.back() == ':') {
      return make_label();
    }
    return make_directive();
  }

  if (trimmed.back() == ':' && !trimmed.empty() && trimmed.front() != ' ' &&
      trimmed.front() != '\t') {
    return make_label();
  }

  if (starts_with(trimmed, "sd ") || starts_with(trimmed, "sw ")) {
    const bool is_word = starts_with(trimmed, "sw ");
    const std::string_view rest(trimmed.data() + 3, trimmed.size() - 3);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      const std::uint8_t reg = parse_reg(trim_copy(rest.substr(0, comma_pos)));
      if (reg != REG_NONE) {
        const std::optional<std::int32_t> offset =
            parse_s0_offset(trim_copy(rest.substr(comma_pos + 2)));
        if (offset.has_value()) {
          return make_store(reg, *offset, is_word);
        }
      }
    }
  }

  if (starts_with(trimmed, "ld ") || starts_with(trimmed, "lw ")) {
    const bool is_word = starts_with(trimmed, "lw ");
    const std::string_view rest(trimmed.data() + 3, trimmed.size() - 3);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      const std::uint8_t reg = parse_reg(trim_copy(rest.substr(0, comma_pos)));
      if (reg != REG_NONE) {
        const std::optional<std::int32_t> offset =
            parse_s0_offset(trim_copy(rest.substr(comma_pos + 2)));
        if (offset.has_value()) {
          return make_load(reg, *offset, is_word);
        }
      }
    }
  }

  if (starts_with(trimmed, "mv ")) {
    const std::string_view rest(trimmed.data() + 3, trimmed.size() - 3);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      const std::uint8_t dst = parse_reg(trim_copy(rest.substr(0, comma_pos)));
      const std::uint8_t src = parse_reg(trim_copy(rest.substr(comma_pos + 2)));
      if (dst != REG_NONE && src != REG_NONE) {
        return make_move(dst, src);
      }
    }
  }

  if (starts_with(trimmed, "li ")) {
    const std::string_view rest(trimmed.data() + 3, trimmed.size() - 3);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      const std::uint8_t dst = parse_reg(trim_copy(rest.substr(0, comma_pos)));
      if (dst != REG_NONE) {
        return make_load_imm(dst);
      }
    }
  }

  if (starts_with(trimmed, "sext.w ")) {
    const std::string_view rest(trimmed.data() + 7, trimmed.size() - 7);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      const std::uint8_t dst = parse_reg(trim_copy(rest.substr(0, comma_pos)));
      const std::uint8_t src = parse_reg(trim_copy(rest.substr(comma_pos + 2)));
      if (dst != REG_NONE && src != REG_NONE) {
        return make_sext_w(dst, src);
      }
    }
  }

  if (starts_with(trimmed, "jump ") || starts_with(trimmed, "j ")) {
    return make_jump();
  }

  if (starts_with(trimmed, "beq ") || starts_with(trimmed, "bne ") ||
      starts_with(trimmed, "bge ") || starts_with(trimmed, "blt ") ||
      starts_with(trimmed, "bgeu ") || starts_with(trimmed, "bltu ") ||
      starts_with(trimmed, "bnez ") || starts_with(trimmed, "beqz ")) {
    return make_branch();
  }

  if (trimmed == "ret") {
    return make_ret();
  }

  if (starts_with(trimmed, "call ") || starts_with(trimmed, "jal ra,")) {
    return make_call();
  }

  if (starts_with(trimmed, "add ") || starts_with(trimmed, "addi ") ||
      starts_with(trimmed, "addw ") || starts_with(trimmed, "addiw ") ||
      starts_with(trimmed, "sub ") || starts_with(trimmed, "subw ") ||
      starts_with(trimmed, "and ") || starts_with(trimmed, "andi ") ||
      starts_with(trimmed, "or ") || starts_with(trimmed, "ori ") ||
      starts_with(trimmed, "xor ") || starts_with(trimmed, "xori ") ||
      starts_with(trimmed, "sll ") || starts_with(trimmed, "slli ") ||
      starts_with(trimmed, "srl ") || starts_with(trimmed, "srli ") ||
      starts_with(trimmed, "sra ") || starts_with(trimmed, "srai ") ||
      starts_with(trimmed, "sllw ") || starts_with(trimmed, "slliw ") ||
      starts_with(trimmed, "srlw ") || starts_with(trimmed, "srliw ") ||
      starts_with(trimmed, "sraw ") || starts_with(trimmed, "sraiw ") ||
      starts_with(trimmed, "mul ") || starts_with(trimmed, "mulw ") ||
      starts_with(trimmed, "div ") || starts_with(trimmed, "divu ") ||
      starts_with(trimmed, "divw ") || starts_with(trimmed, "divuw ") ||
      starts_with(trimmed, "rem ") || starts_with(trimmed, "remu ") ||
      starts_with(trimmed, "remw ") || starts_with(trimmed, "remuw ") ||
      starts_with(trimmed, "slt ") || starts_with(trimmed, "sltu ") ||
      starts_with(trimmed, "slti ") || starts_with(trimmed, "sltiu ") ||
      starts_with(trimmed, "neg ") || starts_with(trimmed, "negw ") ||
      starts_with(trimmed, "not ") || starts_with(trimmed, "snez ") ||
      starts_with(trimmed, "seqz ") || starts_with(trimmed, "lui ")) {
    return make_alu();
  }

  if (starts_with(trimmed, "lla ") || starts_with(trimmed, "la ")) {
    const std::size_t mnemonic_len = starts_with(trimmed, "lla ") ? 4 : 3;
    const std::size_t comma_pos = trimmed.find(", ");
    if (comma_pos != std::string::npos && comma_pos > mnemonic_len) {
      const std::uint8_t dst =
          parse_reg(trim_copy(std::string_view(trimmed).substr(mnemonic_len, comma_pos - mnemonic_len)));
      if (dst != REG_NONE) {
        return make_load_addr(dst);
      }
    }
  }

  return make_other();
}

std::optional<std::int32_t> parse_s0_offset(std::string_view addr) {
  if (!ends_with(addr, "(s0)")) {
    return std::nullopt;
  }
  const std::string_view offset = addr.substr(0, addr.size() - 4);
  std::size_t idx = 0;
  try {
    const int value = std::stoi(std::string(offset), &idx, 10);
    if (idx == offset.size()) {
      return static_cast<std::int32_t>(value);
    }
  } catch (...) {
  }
  return std::nullopt;
}

std::optional<std::string> jump_target(std::string_view line) {
  const std::string trimmed = trim_copy(line);
  if (starts_with(trimmed, "jump ")) {
    const std::string_view rest(trimmed.data() + 5, trimmed.size() - 5);
    const std::size_t comma_pos = rest.find(", ");
    if (comma_pos != std::string_view::npos) {
      return std::string(trimmed.substr(5, comma_pos));
    }
  }
  if (starts_with(trimmed, "j ")) {
    return trimmed.substr(2);
  }
  return std::nullopt;
}

std::optional<std::string> label_name(std::string_view line) {
  const std::string trimmed = trim_copy(line);
  if (!trimmed.empty() && trimmed.back() == ':') {
    return trimmed.substr(0, trimmed.size() - 1);
  }
  return std::nullopt;
}

std::optional<std::uint8_t> parse_alu_dest(std::string_view line) {
  const std::string trimmed = trim_copy(line);
  const std::size_t space_pos = trimmed.find(' ');
  if (space_pos == std::string::npos) {
    return std::nullopt;
  }
  const std::string_view args(trimmed.data() + space_pos + 1, trimmed.size() - space_pos - 1);
  const std::size_t comma_pos = args.find(',');
  const std::string_view first =
      comma_pos == std::string_view::npos ? args : args.substr(0, comma_pos);
  const std::uint8_t reg = parse_reg(trim_copy(first));
  if (reg != REG_NONE) {
    return reg;
  }
  return std::nullopt;
}

std::optional<std::string> retarget_li(std::string_view line, std::uint8_t new_dst) {
  const std::string trimmed = trim_copy(line);
  if (!starts_with(trimmed, "li ")) {
    return std::nullopt;
  }
  const std::size_t comma_pos = trimmed.find(", ");
  if (comma_pos == std::string::npos) {
    return std::nullopt;
  }
  std::string out = "li ";
  out += reg_name(new_dst);
  out += trimmed.substr(comma_pos);
  return out;
}

bool eliminate_adjacent_store_load(std::vector<std::string>& lines,
                                   std::vector<LineKind>& kinds,
                                   std::size_t n) {
  bool changed = false;
  std::size_t i = 0;
  while (i + 1 < n) {
    if (kinds[i].tag == LineKindTag::StoreS0) {
      std::size_t j = i + 1;
      while (j < n && is_nop(kinds[j])) {
        ++j;
      }
      if (j < n && kinds[j].tag == LineKindTag::LoadS0 &&
          kinds[i].offset == kinds[j].offset &&
          kinds[i].is_word == kinds[j].is_word) {
        if (kinds[i].reg == kinds[j].reg) {
          kinds[j] = make_nop();
          changed = true;
        } else {
          lines[j] = "    mv ";
          lines[j] += reg_name(kinds[j].reg);
          lines[j] += ", ";
          lines[j] += reg_name(kinds[i].reg);
          kinds[j] = make_move(kinds[j].reg, kinds[i].reg);
          changed = true;
        }
      }
    }
    ++i;
  }
  return changed;
}

bool eliminate_redundant_jumps(const std::vector<std::string>& lines,
                               std::vector<LineKind>& kinds,
                               std::size_t n) {
  bool changed = false;
  for (std::size_t i = 0; i < n; ++i) {
    if (kinds[i].tag != LineKindTag::Jump) {
      continue;
    }
    const auto target = jump_target(lines[i]);
    if (!target.has_value()) {
      continue;
    }
    std::size_t j = i + 1;
    while (j < n && is_nop(kinds[j])) {
      ++j;
    }
    if (j < n && kinds[j].tag == LineKindTag::Label) {
      const auto lbl = label_name(lines[j]);
      if (lbl.has_value() && *target == *lbl) {
        kinds[i] = make_nop();
        changed = true;
      }
    }
  }
  return changed;
}

bool eliminate_self_moves(std::vector<LineKind>& kinds, std::size_t n) {
  bool changed = false;
  for (std::size_t i = 0; i < n; ++i) {
    if (kinds[i].tag == LineKindTag::Move && kinds[i].dst == kinds[i].src) {
      kinds[i] = make_nop();
      changed = true;
    }
  }
  return changed;
}

bool eliminate_redundant_mv_chain(std::vector<std::string>& lines,
                                  std::vector<LineKind>& kinds,
                                  std::size_t n) {
  bool changed = false;
  std::size_t i = 0;
  while (i + 1 < n) {
    if (kinds[i].tag == LineKindTag::Move) {
      std::size_t j = i + 1;
      while (j < n && is_nop(kinds[j])) {
        ++j;
      }
      if (j < n && kinds[j].tag == LineKindTag::Move &&
          kinds[j].src == kinds[i].dst && kinds[j].dst != kinds[i].src) {
        lines[j] = "    mv ";
        lines[j] += reg_name(kinds[j].dst);
        lines[j] += ", ";
        lines[j] += reg_name(kinds[i].src);
        kinds[j] = make_move(kinds[j].dst, kinds[i].src);
        changed = true;
      }
    }
    ++i;
  }
  return changed;
}

bool eliminate_li_mv_chain(std::vector<std::string>& lines,
                           std::vector<LineKind>& kinds,
                           std::size_t n) {
  bool changed = false;
  std::size_t i = 0;
  while (i + 1 < n) {
    if (kinds[i].tag == LineKindTag::LoadImm && kinds[i].dst <= REG_T6) {
      std::size_t j = i + 1;
      while (j < n && is_nop(kinds[j])) {
        ++j;
      }
      if (j < n && kinds[j].tag == LineKindTag::Move && kinds[j].src == kinds[i].dst) {
        const auto new_line = retarget_li(lines[i], kinds[j].dst);
        if (new_line.has_value()) {
          lines[j] = "    " + *new_line;
          kinds[j] = make_load_imm(kinds[j].dst);
          changed = true;
        }
      }
    }
    ++i;
  }
  return changed;
}

void gsf_invalidate_all(std::vector<std::pair<std::int32_t, SlotMapping>>& slots,
                        std::vector<std::vector<std::int32_t>>& reg_slots) {
  slots.clear();
  for (auto& rs : reg_slots) {
    rs.clear();
  }
}

void gsf_invalidate_overlapping(std::vector<std::pair<std::int32_t, SlotMapping>>& slots,
                                std::vector<std::vector<std::int32_t>>& reg_slots,
                                std::int32_t store_off,
                                std::int32_t store_size) {
  const std::int32_t store_end = store_off + store_size;
  for (auto& entry : slots) {
    auto& [off, mapping] = entry;
    if (!mapping.active) {
      continue;
    }
    const std::int32_t mapping_size = mapping.is_word ? 4 : 8;
    const std::int32_t mapping_end = off + mapping_size;
    if (off < store_end && store_off < mapping_end) {
      const std::size_t r = mapping.reg;
      if (r < reg_slots.size()) {
        auto& offsets = reg_slots[r];
        offsets.erase(std::remove(offsets.begin(), offsets.end(), off), offsets.end());
      }
      mapping.active = false;
    }
  }
}

void gsf_invalidate_reg(std::vector<std::pair<std::int32_t, SlotMapping>>& slots,
                        std::vector<std::vector<std::int32_t>>& reg_slots,
                        std::uint8_t reg) {
  const std::size_t r = reg;
  if (r >= reg_slots.size()) {
    return;
  }
  for (const std::int32_t offset : reg_slots[r]) {
    for (auto it = slots.rbegin(); it != slots.rend(); ++it) {
      auto& [off, mapping] = *it;
      if (off == offset && mapping.active && mapping.reg == reg) {
        mapping.active = false;
        break;
      }
    }
  }
  reg_slots[r].clear();
}

bool global_store_forwarding(std::vector<std::string>& lines,
                             std::vector<LineKind>& kinds,
                             std::size_t n) {
  if (n == 0) {
    return false;
  }

  std::vector<std::pair<std::int32_t, SlotMapping>> slots;
  std::vector<std::vector<std::int32_t>> reg_slots(NUM_REGS);
  bool changed = false;

  for (std::size_t i = 0; i < n; ++i) {
    if (is_nop(kinds[i])) {
      continue;
    }

    switch (kinds[i].tag) {
      case LineKindTag::Label:
        gsf_invalidate_all(slots, reg_slots);
        break;
      case LineKindTag::StoreS0: {
        const std::int32_t store_size = kinds[i].is_word ? 4 : 8;
        gsf_invalidate_overlapping(slots, reg_slots, kinds[i].offset, store_size);
        if (kinds[i].reg < NUM_REGS) {
          slots.push_back({kinds[i].offset, SlotMapping{kinds[i].reg, kinds[i].is_word, true}});
          reg_slots[kinds[i].reg].push_back(kinds[i].offset);
        }
        if (slots.size() > MAX_SLOT_ENTRIES) {
          slots.erase(std::remove_if(slots.begin(), slots.end(),
                                     [](const auto& item) { return !item.second.active; }),
                      slots.end());
        }
        break;
      }
      case LineKindTag::LoadS0: {
        SlotMapping mapping;
        bool found = false;
        for (auto it = slots.rbegin(); it != slots.rend(); ++it) {
          if (it->second.active && it->first == kinds[i].offset &&
              it->second.is_word == kinds[i].is_word) {
            mapping = it->second;
            found = true;
            break;
          }
        }
        if (found) {
          if (kinds[i].reg == mapping.reg) {
            kinds[i] = make_nop();
            changed = true;
          } else {
            lines[i] = "    mv ";
            lines[i] += reg_name(kinds[i].reg);
            lines[i] += ", ";
            lines[i] += reg_name(mapping.reg);
            kinds[i] = make_move(kinds[i].reg, mapping.reg);
            changed = true;
          }
        }
        if (kinds[i].reg < NUM_REGS) {
          gsf_invalidate_reg(slots, reg_slots, kinds[i].reg);
        }
        break;
      }
      case LineKindTag::Jump:
      case LineKindTag::Branch:
      case LineKindTag::Ret:
        gsf_invalidate_all(slots, reg_slots);
        break;
      case LineKindTag::Call:
        gsf_invalidate_all(slots, reg_slots);
        break;
      case LineKindTag::Move:
        if (kinds[i].dst < NUM_REGS) {
          gsf_invalidate_reg(slots, reg_slots, kinds[i].dst);
        }
        break;
      case LineKindTag::LoadImm:
      case LineKindTag::SextW:
      case LineKindTag::LoadAddr:
        if (kinds[i].dst < NUM_REGS) {
          gsf_invalidate_reg(slots, reg_slots, kinds[i].dst);
        }
        break;
      case LineKindTag::Alu:
        if (const auto dst = parse_alu_dest(lines[i]); dst.has_value() && *dst < NUM_REGS) {
          gsf_invalidate_reg(slots, reg_slots, *dst);
        }
        break;
      case LineKindTag::Directive:
      case LineKindTag::Nop:
        break;
      case LineKindTag::Other:
        break;
    }
  }

  return changed;
}

bool propagate_register_copies(std::vector<std::string>& lines,
                               std::vector<LineKind>& kinds,
                               std::size_t n) {
  bool changed = false;

  for (std::size_t i = 0; i < n; ++i) {
    const std::uint8_t dst = kinds[i].dst;
    const std::uint8_t src = kinds[i].src;
    if (kinds[i].tag != LineKindTag::Move) {
      continue;
    }
    if (dst == REG_S0 || src == REG_S0 || dst == REG_SP || src == REG_SP) {
      continue;
    }

    std::size_t j = i + 1;
    while (j < n && is_nop(kinds[j])) {
      ++j;
    }
    if (j >= n) {
      continue;
    }

    switch (kinds[j].tag) {
      case LineKindTag::Label:
      case LineKindTag::Jump:
      case LineKindTag::Ret:
      case LineKindTag::Directive:
      case LineKindTag::Call:
      case LineKindTag::LoadAddr:
        continue;
      default:
        break;
    }

    const std::string dst_name = reg_name(dst);
    if (lines[j].find(dst_name) == std::string::npos) {
      continue;
    }
    const std::string src_name = reg_name(src);

    if (kinds[j].tag == LineKindTag::Move && kinds[j].src == dst) {
      if (kinds[j].dst != src) {
        lines[j] = "    mv ";
        lines[j] += reg_name(kinds[j].dst);
        lines[j] += ", ";
        lines[j] += src_name;
        kinds[j] = make_move(kinds[j].dst, src);
        changed = true;
      }
    } else {
      const auto new_line =
          replace_source_reg_in_instruction(lines[j], dst_name, src_name);
      if (new_line.has_value()) {
        lines[j] = *new_line;
        kinds[j] = classify_line(lines[j]);
        changed = true;
      }
    }
  }
  return changed;
}

bool eliminate_dead_reg_moves(const std::vector<std::string>& lines,
                              std::vector<LineKind>& kinds,
                              std::size_t n) {
  bool changed = false;
  constexpr std::size_t window = 16;

  for (std::size_t i = 0; i < n; ++i) {
    if (kinds[i].tag != LineKindTag::Move) {
      continue;
    }
    const std::uint8_t dst = kinds[i].dst;
    if (dst > REG_T6) {
      continue;
    }

    const std::string dst_name = reg_name(dst);
    bool is_dead = false;
    std::size_t count = 0;
    std::size_t j = i + 1;
    while (j < n && count < window) {
      if (is_nop(kinds[j])) {
        ++j;
        continue;
      }
      switch (kinds[j].tag) {
        case LineKindTag::Label:
        case LineKindTag::Jump:
        case LineKindTag::Branch:
        case LineKindTag::Ret:
        case LineKindTag::Call:
        case LineKindTag::Directive:
          j = n;
          break;
        case LineKindTag::Other:
          j = n;
          break;
        default:
          break;
      }
      if (j >= n) {
        break;
      }

      bool reads_dst = false;
      switch (kinds[j].tag) {
        case LineKindTag::Move:
          reads_dst = kinds[j].src == dst;
          break;
        case LineKindTag::SextW:
          reads_dst = kinds[j].src == dst;
          break;
        case LineKindTag::StoreS0:
          reads_dst = kinds[j].reg == dst;
          break;
        case LineKindTag::LoadS0:
        case LineKindTag::LoadImm:
        case LineKindTag::LoadAddr:
          reads_dst = false;
          break;
        case LineKindTag::Alu: {
          const std::string trimmed = trim_copy(lines[j]);
          const std::size_t comma_pos = trimmed.find(',');
          if (comma_pos == std::string::npos) {
            reads_dst = true;
          } else {
            const std::string_view after = std::string_view(trimmed).substr(comma_pos + 1);
            reads_dst = has_whole_word(after, dst_name);
          }
          break;
        }
        default:
          reads_dst = true;
          break;
      }

      if (reads_dst) {
        break;
      }

      std::optional<std::uint8_t> dest_of_j;
      switch (kinds[j].tag) {
        case LineKindTag::Move:
          dest_of_j = kinds[j].dst;
          break;
        case LineKindTag::LoadImm:
          dest_of_j = kinds[j].dst;
          break;
        case LineKindTag::SextW:
          dest_of_j = kinds[j].dst;
          break;
        case LineKindTag::LoadS0:
          dest_of_j = kinds[j].reg;
          break;
        case LineKindTag::LoadAddr:
          dest_of_j = kinds[j].dst;
          break;
        case LineKindTag::Alu:
          dest_of_j = parse_alu_dest(lines[j]);
          break;
        default:
          break;
      }

      if (dest_of_j.has_value() && *dest_of_j == dst) {
        is_dead = true;
        break;
      }

      ++count;
      ++j;
    }

    if (is_dead) {
      kinds[i] = make_nop();
      changed = true;
    }
  }
  return changed;
}

bool global_dead_store_elimination(const std::vector<std::string>& lines,
                                   std::vector<LineKind>& kinds,
                                   std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) {
    if (is_nop(kinds[i])) {
      continue;
    }
    const std::string trimmed = trim_copy(lines[i]);
    if (starts_with(trimmed, "addi ") && trimmed.find(", s0,") != std::string::npos) {
      const auto dest = parse_alu_dest(trimmed);
      if (dest.has_value() && *dest != REG_S0) {
        return false;
      }
    }
    if (starts_with(trimmed, "mv ") && trimmed.find(", s0") != std::string::npos) {
      const std::size_t pos = trimmed.find(", ");
      if (pos != std::string::npos) {
        const std::string_view src = std::string_view(trimmed).substr(pos + 2);
        if (trim_copy(src) == "s0") {
          return false;
        }
      }
    }
  }

  std::vector<std::pair<std::int32_t, std::int32_t>> loaded_ranges;
  for (std::size_t i = 0; i < n; ++i) {
    if (kinds[i].tag == LineKindTag::LoadS0) {
      const std::int32_t size = kinds[i].is_word ? 4 : 8;
      loaded_ranges.push_back({kinds[i].offset, size});
      continue;
    }

    const std::string trimmed = trim_copy(lines[i]);
    std::optional<std::int32_t> load_size;
    if (starts_with(trimmed, "ld ")) {
      load_size = 8;
    } else if (starts_with(trimmed, "lw ") || starts_with(trimmed, "lwu ")) {
      load_size = 4;
    } else if (starts_with(trimmed, "lh ") || starts_with(trimmed, "lhu ")) {
      load_size = 2;
    } else if (starts_with(trimmed, "lb ") || starts_with(trimmed, "lbu ")) {
      load_size = 1;
    }
    if (load_size.has_value() && trimmed.find("(s0)") != std::string::npos) {
      const auto off = extract_s0_offset_from_line(trimmed);
      if (off.has_value()) {
        loaded_ranges.push_back({*off, *load_size});
      }
    }
  }

  bool changed = false;
  for (std::size_t i = 0; i < n; ++i) {
    if (kinds[i].tag != LineKindTag::StoreS0) {
      continue;
    }
    const std::int32_t store_size = kinds[i].is_word ? 4 : 8;
    const bool overlaps_any_load = std::any_of(
        loaded_ranges.begin(), loaded_ranges.end(),
        [&](const auto& entry) {
          const auto [load_off, load_sz] = entry;
          return kinds[i].offset < load_off + load_sz &&
                 load_off < kinds[i].offset + store_size;
        });
    if (!overlaps_any_load) {
      kinds[i] = make_nop();
      changed = true;
    }
  }
  return changed;
}

std::optional<std::int32_t> extract_s0_offset_from_line(std::string_view line) {
  const std::size_t paren_pos = line.find("(s0)");
  if (paren_pos == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view before = line.substr(0, paren_pos);
  const std::size_t start = before.find_last_of(", ");
  const std::string_view offset = start == std::string_view::npos
                                      ? before
                                      : before.substr(start + 1);
  try {
    std::size_t idx = 0;
    const int value = std::stoi(std::string(trim_copy(offset)), &idx, 10);
    if (idx == trim_copy(offset).size()) {
      return static_cast<std::int32_t>(value);
    }
  } catch (...) {
  }
  return std::nullopt;
}

}  // namespace

std::string peephole_optimize(std::string asm_text) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start <= asm_text.size()) {
    const std::size_t end = asm_text.find('\n', start);
    if (end == std::string::npos) {
      lines.emplace_back(asm_text.substr(start));
      break;
    }
    lines.emplace_back(asm_text.substr(start, end - start));
    start = end + 1;
  }

  std::vector<LineKind> kinds;
  kinds.reserve(lines.size());
  for (const auto& line : lines) {
    kinds.push_back(classify_line(line));
  }

  const std::size_t n = lines.size();
  if (n == 0) {
    return asm_text;
  }

  bool changed = true;
  std::size_t rounds = 0;
  while (changed && rounds < 8) {
    changed = false;
    changed |= eliminate_adjacent_store_load(lines, kinds, n);
    changed |= eliminate_redundant_jumps(lines, kinds, n);
    changed |= eliminate_self_moves(kinds, n);
    changed |= eliminate_redundant_mv_chain(lines, kinds, n);
    changed |= eliminate_li_mv_chain(lines, kinds, n);
    ++rounds;
  }

  bool global_changed = false;
  global_changed |= global_store_forwarding(lines, kinds, n);
  global_changed |= propagate_register_copies(lines, kinds, n);
  global_changed |= eliminate_dead_reg_moves(lines, kinds, n);
  global_changed |= global_dead_store_elimination(lines, kinds, n);

  if (global_changed) {
    bool changed2 = true;
    std::size_t rounds2 = 0;
    while (changed2 && rounds2 < 4) {
      changed2 = false;
      changed2 |= eliminate_adjacent_store_load(lines, kinds, n);
      changed2 |= eliminate_redundant_jumps(lines, kinds, n);
      changed2 |= eliminate_self_moves(kinds, n);
      changed2 |= eliminate_redundant_mv_chain(lines, kinds, n);
      changed2 |= eliminate_li_mv_chain(lines, kinds, n);
      changed2 |= eliminate_dead_reg_moves(lines, kinds, n);
      ++rounds2;
    }
  }

  std::string result;
  result.reserve(asm_text.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (!is_nop(kinds[i])) {
      result.append(lines[i]);
      result.push_back('\n');
    }
  }
  return result;
}

}  // namespace c4c::backend::riscv::codegen
