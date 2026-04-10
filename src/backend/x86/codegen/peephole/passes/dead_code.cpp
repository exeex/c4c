// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/dead_code.rs
// Dead code elimination passes.

#include "../peephole.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

constexpr std::size_t kDeadMoveWindow = 24;
constexpr std::size_t kDeadStoreWindow = 16;

std::string_view trimmed_line(const LineStore* store, const LineInfo& info,
                              std::size_t index) {
  return std::string_view(store->get(index)).substr(info.trim_start);
}

bool ranges_overlap(std::int32_t a_off, std::int32_t a_bytes, std::int32_t b_off,
                    std::int32_t b_bytes) {
  return a_off < b_off + b_bytes && b_off < a_off + a_bytes;
}

void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
  info.reg_refs = 0;
  info.rbp_offset = RBP_OFFSET_NONE;
  info.has_indirect_mem = false;
}

}  // namespace

bool eliminate_dead_reg_moves(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].is_nop() || infos[i].is_barrier()) {
      continue;
    }

    const auto trimmed = trimmed_line(store, infos[i], i);
    const auto move = parse_reg_to_reg_movq(infos[i], trimmed);
    if (!move.has_value()) {
      continue;
    }

    const RegId dst_reg = move->second;
    if (dst_reg == REG_NONE || dst_reg > REG_GP_MAX || dst_reg == 4 || dst_reg == 5) {
      continue;
    }

    const std::uint16_t dst_mask = static_cast<std::uint16_t>(1u << dst_reg);
    bool dead = false;
    const std::size_t scan_end = std::min(i + kDeadMoveWindow, len);

    for (std::size_t j = i + 1; j < scan_end; ++j) {
      if (infos[j].is_nop()) {
        continue;
      }
      if (infos[j].is_barrier()) {
        break;
      }

      const auto scan_trimmed = trimmed_line(store, infos[j], j);
      if (has_implicit_reg_usage(scan_trimmed)) {
        break;
      }

      const bool refs_dst = (infos[j].reg_refs & dst_mask) != 0;
      const bool writes_dst = get_dest_reg(infos[j]) == dst_reg;

      if (writes_dst) {
        const bool also_reads =
            refs_dst &&
            ((infos[j].kind == LineKind::Other && is_read_modify_write(scan_trimmed)) ||
             infos[j].kind == LineKind::Cmp);
        if (also_reads) {
          break;
        }
        dead = true;
        break;
      }

      if (refs_dst) {
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
    if (infos[i].kind != LineKind::StoreRbp || infos[i].rbp_offset == RBP_OFFSET_NONE) {
      continue;
    }

    bool slot_read = false;
    bool slot_overwritten = false;
    const std::size_t scan_end = std::min(i + kDeadStoreWindow, len);

    for (std::size_t j = i + 1; j < scan_end; ++j) {
      if (infos[j].is_nop()) {
        continue;
      }
      if (infos[j].is_barrier()) {
        slot_read = true;
        break;
      }

      if (infos[j].kind == LineKind::LoadRbp && infos[j].rbp_offset != RBP_OFFSET_NONE &&
          ranges_overlap(infos[i].rbp_offset, 8, infos[j].rbp_offset, 8)) {
        slot_read = true;
        break;
      }

      if (infos[j].kind == LineKind::StoreRbp && infos[j].rbp_offset != RBP_OFFSET_NONE &&
          ranges_overlap(infos[i].rbp_offset, 8, infos[j].rbp_offset, 8)) {
        slot_overwritten = true;
        break;
      }

      if ((infos[j].kind == LineKind::Other || infos[j].kind == LineKind::Cmp) &&
          trimmed_line(store, infos[j], j).find("(%rbp)") != std::string_view::npos) {
        slot_read = true;
        break;
      }
    }

    if (slot_overwritten && !slot_read) {
      mark_nop(infos[i]);
      changed = true;
    }
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
