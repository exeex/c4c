// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/store_forwarding.rs
// Global store forwarding pass.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

bool is_callee_saved_reg(RegId reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
void mark_nop(LineInfo& info);
std::optional<std::uint32_t> parse_label_number(std::string_view label_with_colon);
std::optional<std::uint32_t> parse_dotl_number(std::string_view s);
std::optional<std::string_view> extract_jump_target(std::string_view s);

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

bool global_store_forwarding(LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  bool changed = false;
  std::unordered_map<std::int32_t, std::pair<RegId, std::size_t>> slot_map;
  bool prev_was_jump = false;

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }
    if (infos[i].kind == LineKind::Label) {
      if (prev_was_jump) {
        slot_map.clear();
      }
      prev_was_jump = false;
      continue;
    }
    if (infos[i].kind == LineKind::Jmp || infos[i].kind == LineKind::JmpIndirect || infos[i].kind == LineKind::CondJmp) {
      prev_was_jump = true;
      continue;
    }

    if (infos[i].kind == LineKind::StoreRbp) {
      slot_map[infos[i].rbp_offset] = {infos[i].dest_reg, i};
      continue;
    }

    if (infos[i].kind == LineKind::LoadRbp) {
      auto it = slot_map.find(infos[i].rbp_offset);
      if (it != slot_map.end() && it->second.first == infos[i].dest_reg) {
        mark_nop(infos[i]);
        changed = true;
      }
      continue;
    }

    if (infos[i].kind == LineKind::Call || infos[i].kind == LineKind::Ret || infos[i].kind == LineKind::Directive) {
      slot_map.clear();
      continue;
    }

    if (infos[i].kind == LineKind::Other || infos[i].kind == LineKind::Cmp) {
      const auto line = trimmed_line(store, infos[i], i);
      if (line.find("(%rbp)") != std::string::npos) {
        // Treat the reference as a barrier in this mechanical mirror.
        slot_map.clear();
      }
    }
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
