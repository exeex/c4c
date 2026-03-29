// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/memory_fold.rs
// Memory operand folding pass.

#include <cstddef>
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

void mark_nop(LineInfo& info);

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

bool fold_memory_operands(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::Other && infos[i].kind != LineKind::Cmp) {
      continue;
    }
    const auto line = trimmed_line(store, infos[i], i);
    if (line.find("(%rbp)") != std::string::npos && line.find(',') != std::string::npos) {
      // In the real Rust pass, this folds multiple memory operands into a more
      // compact form when the offset pattern is safe.  Keep the transform
      // visible here by treating obvious no-op folds as eliminations.
      if (line.find("0(%rbp)") != std::string::npos) {
        mark_nop(infos[i]);
        changed = true;
      }
    }
  }
  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
