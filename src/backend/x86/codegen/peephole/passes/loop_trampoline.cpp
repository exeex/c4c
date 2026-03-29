// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/loop_trampoline.rs
// Loop trampoline elimination pass.

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

bool eliminate_loop_trampolines(const LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  bool changed = false;
  for (std::size_t i = 0; i + 1 < len; ++i) {
    if (infos[i].kind != LineKind::Label || infos[i + 1].kind != LineKind::Jmp) {
      continue;
    }
    const auto label = trimmed_line(store, infos[i], i);
    const auto jmp = trimmed_line(store, infos[i + 1], i + 1);
    if (label.starts_with(".L") && jmp.find(label.substr(0, label.size() - 1)) != std::string::npos) {
      mark_nop(infos[i + 1]);
      changed = true;
    }
  }
  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
