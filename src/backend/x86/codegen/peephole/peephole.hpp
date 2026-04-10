#pragma once

#include <cstddef>
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

LineInfo classify_line(std::string_view raw);
std::uint16_t scan_register_refs(std::string_view s);
std::uint8_t register_family_fast(std::string_view reg);
std::string_view trim_spaces(std::string_view text);
std::string_view trailing_operand(std::string_view text);

inline bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

inline bool ends_with(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.substr(text.size() - suffix.size(), suffix.size()) == suffix;
}

inline bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

namespace passes {

bool is_valid_gp_reg(RegId reg);
bool is_callee_saved_reg(RegId reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool has_implicit_reg_usage(std::string_view trimmed);
bool is_shift_or_rotate(std::string_view trimmed);
std::optional<std::pair<RegId, RegId>> parse_reg_to_reg_movq(const LineInfo& info,
                                                             std::string_view trimmed);
RegId get_dest_reg(const LineInfo& info);
std::optional<std::uint32_t> parse_label_number(std::string_view label_with_colon);
std::optional<std::uint32_t> parse_dotl_number(std::string_view s);
std::optional<std::string_view> extract_jump_target(std::string_view s);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
bool is_read_modify_write(std::string_view trimmed);
std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id);
std::string replace_reg_family_in_source(std::string_view line, RegId old_id, RegId new_id);

bool combined_local_pass(LineStore* store, LineInfo* infos);
bool fuse_compare_and_branch(LineStore* store, LineInfo* infos);
bool fuse_movq_ext_truncation(LineStore* store, LineInfo* infos);
bool eliminate_loop_trampolines(LineStore* store, LineInfo* infos);
bool eliminate_push_pop_pairs(const LineStore* store, LineInfo* infos);
bool eliminate_dead_reg_moves(const LineStore* store, LineInfo* infos);
bool eliminate_dead_stores(const LineStore* store, LineInfo* infos);
std::string peephole_optimize(std::string asm_text);

}  // namespace passes

std::string peephole_optimize(std::string asm_text);

}  // namespace c4c::backend::x86::codegen::peephole
