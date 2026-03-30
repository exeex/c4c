// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/callee_saves.rs
// Unused callee-saved register elimination pass.

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

bool is_callee_saved_reg(RegId reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
void mark_nop(LineInfo& info);

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

void eliminate_unused_callee_saves(const LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  std::size_t i = 0;
  while (i < len) {
    if (infos[i].kind != LineKind::Push || infos[i].dest_reg != 5) {
      ++i;
      continue;
    }

    std::size_t j = i + 1;
    while (j < len && infos[j].kind == LineKind::Nop) {
      ++j;
    }
    if (j >= len || trimmed_line(store, infos[j], j) != "movq %rsp, %rbp") {
      ++i;
      continue;
    }

    ++j;
    while (j < len && infos[j].kind == LineKind::Nop) {
      ++j;
    }
    if (j >= len || !trimmed_line(store, infos[j], j).starts_with("subq $")) {
      ++i;
      continue;
    }

    struct CalleeSave {
      RegId reg;
      std::int32_t offset;
      std::size_t save_line_idx;
    };

    std::vector<CalleeSave> saves;
    ++j;
    while (j < len) {
      if (infos[j].kind == LineKind::Nop) {
        ++j;
        continue;
      }
      if (infos[j].kind == LineKind::StoreRbp && is_callee_saved_reg(infos[j].dest_reg) && infos[j].rbp_offset < 0) {
        saves.push_back(CalleeSave{infos[j].dest_reg, infos[j].rbp_offset, j});
        ++j;
        continue;
      }
      break;
    }
    if (saves.empty()) {
      i = j;
      continue;
    }

    std::size_t func_end = len;
    for (std::size_t k = j; k < len; ++k) {
      if (infos[k].kind == LineKind::Nop) {
        continue;
      }
      if (trimmed_line(store, infos[k], k).starts_with(".size ")) {
        func_end = k + 1;
        break;
      }
    }

    for (const auto& save : saves) {
      bool body_has_reference = false;
      std::vector<std::size_t> restore_indices;
      for (std::size_t k = j; k < func_end; ++k) {
        if (infos[k].kind == LineKind::Nop) {
          continue;
        }
        if (infos[k].kind == LineKind::LoadRbp && infos[k].dest_reg == save.reg && infos[k].rbp_offset == save.offset &&
            is_near_epilogue(infos, k)) {
          restore_indices.push_back(k);
          continue;
        }
        if (line_references_reg_fast(infos[k], save.reg)) {
          body_has_reference = true;
          break;
        }
      }
      if (!body_has_reference && !restore_indices.empty()) {
        mark_nop(infos[save.save_line_idx]);
        for (auto ri : restore_indices) {
          mark_nop(infos[ri]);
        }
      }
    }

    i = func_end;
  }
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
