// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/dead_code.rs
// Dead code elimination passes.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

struct LineInfo;
struct LineStore;

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

using RegId = std::uint8_t;
constexpr RegId REG_NONE = 255;
constexpr RegId REG_GP_MAX = 15;
constexpr std::int32_t RBP_OFFSET_NONE = std::numeric_limits<std::int32_t>::min();

bool is_read_modify_write(std::string_view trimmed);
bool has_implicit_reg_usage(std::string_view trimmed);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
std::optional<std::uint32_t> parse_label_number(std::string_view label_with_colon);
std::optional<std::uint32_t> parse_dotl_number(std::string_view s);
std::optional<std::string_view> extract_jump_target(std::string_view s);
void mark_nop(LineInfo& info);

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

static bool ranges_overlap(std::int32_t a_off, std::int32_t a_bytes, std::int32_t b_off, std::int32_t b_bytes) {
  return a_off < b_off + b_bytes && b_off < a_off + a_bytes;
}

static std::string write_rbp_pattern(std::int32_t offset) {
  if (offset < 0) {
    return std::to_string(offset) + "(%rbp)";
  }
  return std::to_string(offset) + "(%rbp)";
}

bool eliminate_dead_reg_moves(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind == LineKind::Nop || infos[i].kind == LineKind::Label || infos[i].kind == LineKind::Directive ||
        infos[i].kind == LineKind::Jmp || infos[i].kind == LineKind::JmpIndirect || infos[i].kind == LineKind::CondJmp ||
        infos[i].kind == LineKind::Call || infos[i].kind == LineKind::Ret) {
      continue;
    }

    const auto line = trimmed_line(store, infos[i], i);
    if (!line.starts_with("movq ")) {
      continue;
    }
    const auto comma = line.find(',');
    if (comma == std::string_view::npos) {
      continue;
    }
    const auto dst = line.substr(comma + 1);
    const auto dst_reg = infos[i].dest_reg;
    if (dst_reg == REG_NONE || dst_reg > REG_GP_MAX || dst_reg == 4 || dst_reg == 5) {
      continue;
    }

    const std::size_t scan_end = std::min(i + 24, len);
    bool dead = false;
    for (std::size_t j = i + 1; j < scan_end; ++j) {
      if (infos[j].kind == LineKind::Nop) {
        continue;
      }
      if (infos[j].kind == LineKind::Label || infos[j].kind == LineKind::Directive || infos[j].kind == LineKind::Jmp ||
          infos[j].kind == LineKind::JmpIndirect || infos[j].kind == LineKind::CondJmp || infos[j].kind == LineKind::Call ||
          infos[j].kind == LineKind::Ret) {
        break;
      }
      if (has_implicit_reg_usage(trimmed_line(store, infos[j], j))) {
        break;
      }
      if (line_references_reg_fast(infos[j], dst_reg)) {
        if (infos[j].dest_reg == dst_reg && !is_read_modify_write(trimmed_line(store, infos[j], j))) {
          dead = true;
        }
        break;
      }
    }
    if (dead) {
      mark_nop(infos[i]);
      changed = true;
    }
  }
  return changed;
}

bool eliminate_dead_stores(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::StoreRbp) {
      continue;
    }
    const std::size_t scan_end = std::min(i + 16, len);
    bool slot_read = false;
    bool slot_overwritten = false;
    for (std::size_t j = i + 1; j < scan_end; ++j) {
      if (infos[j].kind == LineKind::Nop) {
        continue;
      }
      if (infos[j].kind == LineKind::Label || infos[j].kind == LineKind::Directive || infos[j].kind == LineKind::Jmp ||
          infos[j].kind == LineKind::JmpIndirect || infos[j].kind == LineKind::CondJmp || infos[j].kind == LineKind::Call ||
          infos[j].kind == LineKind::Ret) {
        slot_read = true;
        break;
      }
      if (infos[j].kind == LineKind::LoadRbp) {
        if (ranges_overlap(infos[i].rbp_offset, 8, infos[j].rbp_offset, 8)) {
          slot_read = true;
          break;
        }
      }
      if (infos[j].kind == LineKind::StoreRbp) {
        if (ranges_overlap(infos[i].rbp_offset, 8, infos[j].rbp_offset, 8)) {
          slot_overwritten = true;
          break;
        }
      }
      if (infos[j].kind == LineKind::Other || infos[j].kind == LineKind::Cmp) {
        const auto line = trimmed_line(store, infos[j], j);
        if (line.find(write_rbp_pattern(infos[i].rbp_offset)) != std::string::npos) {
          slot_read = true;
          break;
        }
      }
    }
    if (slot_overwritten && !slot_read) {
      mark_nop(infos[i]);
      changed = true;
    }
  }
  return changed;
}

bool eliminate_never_read_stores(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::StoreRbp) {
      continue;
    }
    const auto pattern = write_rbp_pattern(infos[i].rbp_offset);
    bool read_anywhere = false;
    for (std::size_t j = i + 1; j < len; ++j) {
      if (infos[j].kind == LineKind::Nop) {
        continue;
      }
      if (infos[j].kind == LineKind::Label && is_near_epilogue(infos, j)) {
        continue;
      }
      if (trimmed_line(store, infos[j], j).find(pattern) != std::string::npos) {
        read_anywhere = true;
        break;
      }
    }
    if (!read_anywhere) {
      mark_nop(infos[i]);
      changed = true;
    }
  }
  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
