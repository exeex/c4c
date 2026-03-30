// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/copy_propagation.rs
// Register copy propagation pass.

#include <cstddef>
#include <string>
#include <string_view>
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

void mark_nop(LineInfo& info);
bool is_valid_gp_reg(RegId reg);
bool has_implicit_reg_usage(std::string_view trimmed);
bool is_shift_or_rotate(std::string_view trimmed);
RegId get_dest_reg(const LineInfo& info);
std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id);
std::string replace_reg_family_in_source(std::string_view line, RegId old_id, RegId new_id);

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

static bool try_propagate_into(LineStore* store, LineInfo* infos, std::size_t j, RegId src_id, RegId dst_id) {
  if (infos[j].has_indirect_mem) {
    return false;
  }
  const auto trimmed = trimmed_line(store, infos[j], j);
  if ((infos[j].reg_refs & (1u << dst_id)) == 0) {
    return false;
  }
  if (has_implicit_reg_usage(trimmed)) {
    return false;
  }
  if (dst_id == 1 && is_shift_or_rotate(trimmed)) {
    return false;
  }
  if (get_dest_reg(infos[j]) == src_id) {
    return false;
  }
  const auto replaced = replace_reg_family(trimmed, dst_id, src_id);
  if (replaced != trimmed) {
    store->get(j) = "    " + replaced;
    return true;
  }
  return false;
}

bool propagate_register_copies(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::Other || infos[i].dest_reg == 255) {
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
    const auto src = line.substr(5, comma - 5);
    const auto dst = line.substr(comma + 1);
    const auto src_id = get_dest_reg(infos[i]);
    const auto dst_id = infos[i].dest_reg;
    if (!is_valid_gp_reg(src_id) || !is_valid_gp_reg(dst_id) || src_id == dst_id) {
      continue;
    }
    for (std::size_t j = i + 1; j < std::min(i + 24, len); ++j) {
      if (infos[j].kind == LineKind::Nop) {
        continue;
      }
      if (infos[j].kind == LineKind::Label || infos[j].kind == LineKind::Call || infos[j].kind == LineKind::Jmp ||
          infos[j].kind == LineKind::JmpIndirect || infos[j].kind == LineKind::CondJmp || infos[j].kind == LineKind::Ret ||
          infos[j].kind == LineKind::Directive) {
        break;
      }
      if (try_propagate_into(store, infos, j, src_id, dst_id)) {
        changed = true;
      }
      if (get_dest_reg(infos[j]) == dst_id) {
        break;
      }
    }
  }
  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
