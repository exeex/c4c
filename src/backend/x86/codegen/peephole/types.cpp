// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/types.rs
// Core line-classification types and helper constructors for the peephole pipeline.

#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::x86::codegen::peephole {

using RegId = std::uint8_t;
constexpr RegId REG_NONE = 255;
constexpr RegId REG_GP_MAX = 15;
constexpr std::int32_t RBP_OFFSET_NONE = std::numeric_limits<std::int32_t>::min();

struct LineInfo;

enum class LineKind {
  Nop,
  Empty,
  StoreRbp,
  LoadRbp,
  SelfMove,
  Label,
  Jmp,
  JmpIndirect,
  CondJmp,
  Call,
  Ret,
  Push,
  Pop,
  SetCC,
  Cmp,
  Directive,
  Other,
};

enum class ExtKind {
  None,
  MovzbqAlRax,
  MovzwqAxRax,
  MovsbqAlRax,
  MovslqEaxRax,
  MovlEaxEax,
  Cltq,
  ProducerMovzbqToRax,
  ProducerMovzwqToRax,
  ProducerMovsbqToRax,
  ProducerMovslqToRax,
  ProducerMovqConstRax,
  ProducerArith32,
  ProducerMovlToEax,
  ProducerMovzbToEax,
  ProducerMovzwToEax,
  ProducerDiv32,
  ProducerMovqRegToRax,
};

enum class MoveSize {
  Q,
  L,
  W,
  B,
  SLQ,
};

struct StoreRbpParts {
  std::string_view reg;
  std::string_view offset;
  MoveSize size;
};

struct LoadRbpParts {
  std::string_view offset;
  std::string_view reg;
  MoveSize size;
};

struct LineInfo {
  LineKind kind = LineKind::Nop;
  ExtKind ext_kind = ExtKind::None;
  std::uint16_t trim_start = 0;
  RegId dest_reg = REG_NONE;
  bool has_indirect_mem = false;
  std::int32_t rbp_offset = RBP_OFFSET_NONE;
  std::uint16_t reg_refs = 0;

  bool is_nop() const { return kind == LineKind::Nop; }
  bool is_barrier() const {
    return kind == LineKind::Label || kind == LineKind::Call || kind == LineKind::Jmp ||
           kind == LineKind::JmpIndirect || kind == LineKind::CondJmp || kind == LineKind::Ret ||
           kind == LineKind::Directive;
  }
  bool is_push() const { return kind == LineKind::Push; }
};

struct LineStore {
  std::vector<std::string> lines;

  std::size_t len() const { return lines.size(); }
  const std::string& get(std::size_t i) const { return lines[i]; }
  std::string& get(std::size_t i) { return lines[i]; }
};

inline LineInfo line_info(LineKind kind, std::uint16_t ts) {
  LineInfo info;
  info.kind = kind;
  info.trim_start = ts;
  return info;
}

inline LineInfo line_info_with_regs(LineKind kind, std::uint16_t ts, std::uint16_t reg_refs) {
  LineInfo info;
  info.kind = kind;
  info.trim_start = ts;
  info.reg_refs = reg_refs;
  return info;
}

inline LineInfo line_info_ext(LineKind kind, ExtKind ext, std::uint16_t ts) {
  LineInfo info;
  info.kind = kind;
  info.ext_kind = ext;
  info.trim_start = ts;
  return info;
}

inline bool is_nop(const LineInfo& info) { return info.is_nop(); }

static std::size_t compute_trim_offset(std::string_view raw) {
  std::size_t i = 0;
  while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\r')) {
    ++i;
  }
  return i;
}

static std::int32_t fast_parse_i32(std::string_view text) {
  std::int32_t sign = 1;
  std::size_t i = 0;
  if (!text.empty() && text.front() == '-') {
    sign = -1;
    ++i;
  }
  std::int32_t value = 0;
  for (; i < text.size(); ++i) {
    if (text[i] < '0' || text[i] > '9') {
      break;
    }
    value = value * 10 + static_cast<std::int32_t>(text[i] - '0');
  }
  return sign * value;
}

static bool is_delim(char c) {
  return c == ',' || c == ')' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0';
}

static bool is_conditional_jump(std::string_view s) {
  return s.starts_with("je ") || s.starts_with("jne ") || s.starts_with("jg ") || s.starts_with("jge ") ||
         s.starts_with("jl ") || s.starts_with("jle ") || s.starts_with("ja ") || s.starts_with("jae ") ||
         s.starts_with("jb ") || s.starts_with("jbe ") || s.starts_with("js ") || s.starts_with("jns ") ||
         s.starts_with("jo ") || s.starts_with("jno ") || s.starts_with("jp ") || s.starts_with("jnp ");
}

std::uint16_t scan_register_refs(std::string_view s) {
  constexpr std::array<std::string_view, 16> regs = {
      "%rax","%rcx","%rdx","%rbx","%rsp","%rbp","%rsi","%rdi",
      "%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15",
  };
  std::uint16_t mask = 0;
  for (std::uint8_t i = 0; i < regs.size(); ++i) {
    if (s.find(regs[i]) != std::string_view::npos ||
        s.find("%e" + std::string(regs[i].substr(2))) != std::string_view::npos ||
        s.find("%" + std::string(regs[i].substr(2, 1))) != std::string_view::npos) {
      mask |= static_cast<std::uint16_t>(1u << i);
    }
  }
  return mask;
}

std::uint8_t register_family_fast(std::string_view reg) {
  if (reg.starts_with('%')) {
    reg.remove_prefix(1);
  }
  if (reg == "rax" || reg == "eax" || reg == "ax" || reg == "al" || reg == "ah") return 0;
  if (reg == "rcx" || reg == "ecx" || reg == "cx" || reg == "cl" || reg == "ch") return 1;
  if (reg == "rdx" || reg == "edx" || reg == "dx" || reg == "dl" || reg == "dh") return 2;
  if (reg == "rbx" || reg == "ebx" || reg == "bx" || reg == "bl" || reg == "bh") return 3;
  if (reg == "rsp" || reg == "esp" || reg == "sp" || reg == "spl") return 4;
  if (reg == "rbp" || reg == "ebp" || reg == "bp" || reg == "bpl") return 5;
  if (reg == "rsi" || reg == "esi" || reg == "si" || reg == "sil") return 6;
  if (reg == "rdi" || reg == "edi" || reg == "di" || reg == "dil") return 7;
  if (reg.starts_with("r8")) return 8;
  if (reg.starts_with("r9")) return 9;
  if (reg.starts_with("r10")) return 10;
  if (reg.starts_with("r11")) return 11;
  if (reg.starts_with("r12")) return 12;
  if (reg.starts_with("r13")) return 13;
  if (reg.starts_with("r14")) return 14;
  if (reg.starts_with("r15")) return 15;
  return REG_NONE;
}

bool has_indirect_memory_access(std::string_view s) {
  return s.find("(%") != std::string_view::npos && s.find("%rbp") == std::string_view::npos;
}

std::int32_t parse_rbp_offset(std::string_view s) {
  const auto pos = s.find("(%rbp)");
  if (pos == std::string_view::npos) {
    return RBP_OFFSET_NONE;
  }
  std::string_view prefix = s.substr(0, pos);
  const auto last_space = prefix.rfind(' ');
  if (last_space != std::string_view::npos) {
    prefix = prefix.substr(last_space + 1);
  }
  if (prefix.empty()) {
    return 0;
  }
  return fast_parse_i32(prefix);
}

std::optional<std::string_view> parse_setcc(std::string_view s) {
  if (!s.starts_with("set") || s.size() < 5) {
    return std::nullopt;
  }
  const auto space = s.find(' ');
  if (space == std::string_view::npos) {
    return std::nullopt;
  }
  return s.substr(3, space - 3);
}

std::optional<StoreRbpParts> parse_store_to_rbp_str(std::string_view s) {
  if (!s.starts_with("mov")) {
    return std::nullopt;
  }
  const auto comma = s.rfind(',');
  const auto lb = s.find("(%rbp)");
  if (comma == std::string_view::npos || lb == std::string_view::npos || comma > lb) {
    return std::nullopt;
  }
  std::string_view reg = s.substr(s.find(' ') + 1, comma - (s.find(' ') + 1));
  std::string_view offset = s.substr(s.find(' ') + 1, lb - (s.find(' ') + 1));
  if (s.starts_with("movq ")) return StoreRbpParts{reg, offset, MoveSize::Q};
  if (s.starts_with("movl ")) return StoreRbpParts{reg, offset, MoveSize::L};
  if (s.starts_with("movw ")) return StoreRbpParts{reg, offset, MoveSize::W};
  if (s.starts_with("movb ")) return StoreRbpParts{reg, offset, MoveSize::B};
  return std::nullopt;
}

std::optional<LoadRbpParts> parse_load_from_rbp_str(std::string_view s) {
  const auto lb = s.find("(%rbp)");
  const auto comma = s.rfind(',');
  if (lb == std::string_view::npos || comma == std::string_view::npos || lb > comma) {
    return std::nullopt;
  }
  std::string_view offset = s.substr(s.find(' ') + 1, lb - (s.find(' ') + 1));
  std::string_view reg = s.substr(comma + 1);
  if (s.starts_with("movq ")) return LoadRbpParts{offset, reg, MoveSize::Q};
  if (s.starts_with("movl ")) return LoadRbpParts{offset, reg, MoveSize::L};
  if (s.starts_with("movw ")) return LoadRbpParts{offset, reg, MoveSize::W};
  if (s.starts_with("movb ")) return LoadRbpParts{offset, reg, MoveSize::B};
  if (s.starts_with("movslq ")) return LoadRbpParts{offset, reg, MoveSize::SLQ};
  return std::nullopt;
}

LineInfo classify_line(std::string_view raw) {
  const auto trim_start = compute_trim_offset(raw);
  raw.remove_prefix(trim_start);
  if (raw.empty()) {
    return line_info(LineKind::Empty, static_cast<std::uint16_t>(trim_start));
  }

  const auto first = raw.front();
  if (raw.back() == ':') {
    return line_info(LineKind::Label, static_cast<std::uint16_t>(trim_start));
  }
  if (first == '.') {
    return line_info(LineKind::Directive, static_cast<std::uint16_t>(trim_start));
  }
  if (auto store = parse_store_to_rbp_str(raw); store.has_value()) {
    LineInfo info = line_info(LineKind::StoreRbp, static_cast<std::uint16_t>(trim_start));
    info.dest_reg = register_family_fast(store->reg);
    info.rbp_offset = fast_parse_i32(store->offset);
    info.reg_refs = scan_register_refs(raw);
    return info;
  }
  if (auto load = parse_load_from_rbp_str(raw); load.has_value()) {
    LineInfo info = line_info(LineKind::LoadRbp, static_cast<std::uint16_t>(trim_start));
    info.dest_reg = register_family_fast(load->reg);
    info.rbp_offset = fast_parse_i32(load->offset);
    info.reg_refs = scan_register_refs(raw);
    return info;
  }
  if (raw.starts_with("movq ") && raw.find(',') != std::string_view::npos) {
    const auto comma = raw.rfind(',');
    const auto src = raw.substr(5, comma - 5);
    const auto dst = raw.substr(comma + 1);
    if (src == dst) {
      return line_info(LineKind::SelfMove, static_cast<std::uint16_t>(trim_start));
    }
  }
  if (raw.starts_with("ret")) {
    return line_info(LineKind::Ret, static_cast<std::uint16_t>(trim_start));
  }
  if (raw.starts_with("jmp ")) {
    return line_info(LineKind::Jmp, static_cast<std::uint16_t>(trim_start));
  }
  if (is_conditional_jump(raw)) {
    return line_info(LineKind::CondJmp, static_cast<std::uint16_t>(trim_start));
  }
  if (raw.starts_with("call ")) {
    return line_info_with_regs(LineKind::Call, static_cast<std::uint16_t>(trim_start), scan_register_refs(raw));
  }
  if (raw.starts_with("pushq ")) {
    LineInfo info = line_info_with_regs(LineKind::Push, static_cast<std::uint16_t>(trim_start), scan_register_refs(raw));
    info.dest_reg = register_family_fast(raw.substr(6));
    return info;
  }
  if (raw.starts_with("popq ")) {
    LineInfo info = line_info_with_regs(LineKind::Pop, static_cast<std::uint16_t>(trim_start), scan_register_refs(raw));
    info.dest_reg = register_family_fast(raw.substr(5));
    return info;
  }
  if (raw.starts_with("set") && parse_setcc(raw).has_value()) {
    LineInfo info = line_info_with_regs(LineKind::SetCC, static_cast<std::uint16_t>(trim_start), scan_register_refs(raw));
    info.dest_reg = register_family_fast(raw.substr(raw.rfind(' ') + 1));
    return info;
  }

  const auto reg_refs = scan_register_refs(raw);
  const auto dest_reg = register_family_fast(raw.substr(raw.rfind(',') != std::string_view::npos ? raw.rfind(',') + 1 : 0));
  const auto has_indirect = has_indirect_memory_access(raw);
  const auto rbp_off = has_indirect ? RBP_OFFSET_NONE : parse_rbp_offset(raw);
  LineInfo info = line_info(LineKind::Other, static_cast<std::uint16_t>(trim_start));
  info.dest_reg = dest_reg;
  info.has_indirect_mem = has_indirect;
  info.rbp_offset = rbp_off;
  info.reg_refs = reg_refs;
  return info;
}

}  // namespace c4c::backend::x86::codegen::peephole
