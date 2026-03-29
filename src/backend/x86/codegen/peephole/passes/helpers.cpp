// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/helpers.rs
// Shared helper functions used across peephole optimization passes.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::x86::codegen::peephole::passes {

using RegId = std::uint8_t;
constexpr RegId REG_NONE = 255;
constexpr RegId REG_GP_MAX = 15;

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

struct LineInfo {
  LineKind kind = LineKind::Nop;
  RegId dest_reg = REG_NONE;
  std::uint16_t reg_refs = 0;
  std::uint16_t trim_start = 0;
  bool has_indirect_mem = false;
};

struct LineStore {
  std::vector<std::string> lines;

  std::size_t len() const { return lines.size(); }
  const std::string& get(std::size_t i) const { return lines[i]; }
  std::string& get(std::size_t i) { return lines[i]; }
};

constexpr const char* REG_NAMES[4][16] = {
    {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"},
    {"eax","ecx","edx","ebx","esp","ebp","esi","edi","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"},
    {"ax","cx","dx","bx","sp","bp","si","di","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"},
    {"al","cl","dl","bl","spl","bpl","sil","dil","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"},
};

static bool is_delim(char c) {
  return c == ',' || c == ')' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0';
}

bool is_valid_gp_reg(RegId reg) { return reg != REG_NONE && reg <= REG_GP_MAX; }

bool is_callee_saved_reg(RegId reg) { return reg == 3 || reg == 12 || reg == 13 || reg == 14 || reg == 15; }

bool line_references_reg_fast(const LineInfo& info, RegId reg) {
  return (info.reg_refs & (1u << reg)) != 0;
}

std::string replace_reg_name_exact(std::string_view line, std::string_view old_reg, std::string_view new_reg) {
  std::string result;
  result.reserve(line.size());
  const std::size_t old_len = old_reg.size();
  std::size_t pos = 0;
  while (pos < line.size()) {
    if (pos + old_len <= line.size() && line.substr(pos, old_len) == old_reg) {
      const std::size_t after = pos + old_len;
      if (after >= line.size() || is_delim(line[after])) {
        result.append(new_reg);
        pos += old_len;
        continue;
      }
    }
    result.push_back(line[pos]);
    ++pos;
  }
  return result;
}

std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id) {
  std::string result(line);
  for (std::size_t size_idx = 0; size_idx < 4; ++size_idx) {
    const char* old_name = REG_NAMES[size_idx][old_id];
    const char* new_name = REG_NAMES[size_idx][new_id];
    if (std::string_view(old_name) == std::string_view(new_name)) {
      continue;
    }
    result = replace_reg_name_exact(result, old_name, new_name);
  }
  return result;
}

std::string replace_reg_family_in_source(std::string_view line, RegId old_id, RegId new_id) {
  const auto comma = line.rfind(',');
  if (comma == std::string_view::npos) {
    return replace_reg_family(line, old_id, new_id);
  }
  return replace_reg_family(line.substr(0, comma), old_id, new_id) + std::string(line.substr(comma));
}

bool has_implicit_reg_usage(std::string_view trimmed) {
  return trimmed.starts_with("div") || trimmed.starts_with("idiv") || trimmed.starts_with("mul") ||
         trimmed.starts_with("cltq") || trimmed.starts_with("cbw") || trimmed.starts_with("cqto") ||
         trimmed.starts_with("cdq") || trimmed.starts_with("cqo") || trimmed.starts_with("cwd") ||
         trimmed.starts_with("rep ") || trimmed.starts_with("repne ") || trimmed.starts_with("cpuid") ||
         trimmed.starts_with("syscall") || trimmed.starts_with("rdtsc") || trimmed.starts_with("rdmsr") ||
         trimmed.starts_with("wrmsr") || trimmed.starts_with("xchg") || trimmed.starts_with("cmpxchg") ||
         trimmed.starts_with("lock ") || trimmed.starts_with("fnstsw") || trimmed.starts_with("fnstcw") ||
         trimmed.starts_with("fstcw") || trimmed.starts_with("fnstenv") || trimmed.starts_with("fldenv") ||
         trimmed.starts_with("fldcw");
}

bool is_shift_or_rotate(std::string_view trimmed) {
  return trimmed.starts_with("sh") || trimmed.starts_with("sa") || trimmed.starts_with("ro") || trimmed.starts_with("rc");
}

RegId register_family_fast(std::string_view reg) {
  for (RegId fam = 0; fam <= REG_GP_MAX; ++fam) {
    if (reg == REG_NAMES[0][fam] || reg == std::string("%") + REG_NAMES[0][fam] ||
        reg == REG_NAMES[1][fam] || reg == std::string("%") + REG_NAMES[1][fam] ||
        reg == REG_NAMES[2][fam] || reg == std::string("%") + REG_NAMES[2][fam] ||
        reg == REG_NAMES[3][fam] || reg == std::string("%") + REG_NAMES[3][fam]) {
      return fam;
    }
  }
  return REG_NONE;
}

std::optional<std::pair<RegId, RegId>> parse_reg_to_reg_movq(const LineInfo& info, std::string_view trimmed) {
  if (info.kind != LineKind::Other || info.dest_reg == REG_NONE || info.dest_reg > REG_GP_MAX) {
    return std::nullopt;
  }
  if (!trimmed.starts_with("movq ")) {
    return std::nullopt;
  }
  const auto rest = trimmed.substr(5);
  const auto comma = rest.find(',');
  if (comma == std::string_view::npos) {
    return std::nullopt;
  }
  const auto src = rest.substr(0, comma);
  const auto dst = rest.substr(comma + 1);
  const auto src_fam = register_family_fast(src);
  const auto dst_fam = register_family_fast(dst);
  if (!is_valid_gp_reg(src_fam) || !is_valid_gp_reg(dst_fam) || src_fam == dst_fam || src_fam == 4 || src_fam == 5 ||
      dst_fam == 4 || dst_fam == 5) {
    return std::nullopt;
  }
  return std::pair<RegId, RegId>{src_fam, dst_fam};
}

RegId get_dest_reg(const LineInfo& info) { return info.dest_reg; }

std::optional<std::uint32_t> parse_dotl_number(std::string_view s) {
  if (!s.starts_with(".LBB")) {
    return std::nullopt;
  }
  s.remove_prefix(4);
  if (s.empty() || s.front() < '0' || s.front() > '9') {
    return std::nullopt;
  }
  std::uint32_t value = 0;
  for (char c : s) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<std::uint32_t>(c - '0');
  }
  return value;
}

std::optional<std::uint32_t> parse_label_number(std::string_view label_with_colon) {
  if (!label_with_colon.ends_with(':')) {
    return std::nullopt;
  }
  return parse_dotl_number(label_with_colon.substr(0, label_with_colon.size() - 1));
}

std::optional<std::string_view> extract_jump_target(std::string_view s) {
  if (s.starts_with("jmp ")) {
    return std::string_view(s).substr(4);
  }
  if (!s.empty() && s.front() == 'j') {
    const auto space = s.find(' ');
    if (space != std::string_view::npos) {
      return std::string_view(s).substr(space + 1);
    }
  }
  return std::nullopt;
}

bool is_near_epilogue(const LineInfo* infos, std::size_t pos) {
  const std::size_t limit = pos + 20;
  for (std::size_t j = pos + 1; j < limit; ++j) {
    if (infos[j].kind == LineKind::Nop) {
      continue;
    }
    switch (infos[j].kind) {
      case LineKind::LoadRbp:
      case LineKind::Pop:
      case LineKind::SelfMove:
      case LineKind::Other:
        continue;
      case LineKind::Ret:
      case LineKind::Jmp:
      case LineKind::JmpIndirect:
        return true;
      default:
        return false;
    }
  }
  return false;
}

bool is_read_modify_write(std::string_view trimmed) {
  if (trimmed.size() < 3) {
    return true;
  }
  if (trimmed.starts_with("mov")) {
    return false;
  }
  if (trimmed.starts_with("lea")) {
    const auto comma = trimmed.rfind(',');
    if (comma != std::string_view::npos) {
      const auto dst = trimmed.substr(comma + 1);
      const auto src = trimmed.substr(0, comma);
      return src.find(dst) != std::string_view::npos;
    }
    return false;
  }
  if (trimmed.starts_with("cmov")) {
    return true;
  }
  if (trimmed.starts_with("set")) {
    return false;
  }
  return true;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
