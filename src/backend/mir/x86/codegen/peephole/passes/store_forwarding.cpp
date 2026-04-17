// Mechanical C++-shaped translation of
// ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/store_forwarding.rs
// Global store forwarding pass.

#include "../peephole.hpp"

#include <cstddef>
#include <unordered_map>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
  info.reg_refs = 0;
  info.rbp_offset = RBP_OFFSET_NONE;
  info.has_indirect_mem = false;
}

std::string_view trimmed_line(const LineStore* store, const LineInfo& info,
                              std::size_t index) {
  return std::string_view(store->get(index)).substr(info.trim_start);
}

}  // namespace

bool global_store_forwarding(LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  bool changed = false;
  std::unordered_map<std::int32_t, RegId> slot_map;
  bool prev_was_jump = false;

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].is_nop()) {
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
      slot_map[infos[i].rbp_offset] = infos[i].dest_reg;
      continue;
    }

    if (infos[i].kind == LineKind::LoadRbp) {
      auto it = slot_map.find(infos[i].rbp_offset);
      if (it != slot_map.end() && it->second == infos[i].dest_reg) {
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
