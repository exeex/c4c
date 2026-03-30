// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/frame_compact.rs
// Frame compaction pass.

#include <algorithm>
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

bool compact_frame(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i + 1 < len; ++i) {
    if (infos[i].kind == LineKind::Other && trimmed_line(store, infos[i], i).starts_with("subq $") &&
        trimmed_line(store, infos[i], i).ends_with(", %rsp")) {
      // The Rust pass tries to compact the frame by tracking dead slots.
      // In this C++-shaped mirror we preserve the structure and mark the
      // frame-adjustment line for rewrite only if the scan can prove a trivial case.
      const auto line = trimmed_line(store, infos[i], i);
      if (line.find("$0") != std::string::npos) {
        mark_nop(infos[i]);
        changed = true;
      }
    }
  }
  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
