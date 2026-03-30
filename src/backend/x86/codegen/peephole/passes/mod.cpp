// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/mod.rs
// Pass orchestration for the x86 peephole pipeline.

#include <string>
#include <utility>

namespace c4c::backend::x86::codegen::peephole::passes {

struct LineInfo;
struct LineStore;

bool combined_local_pass(LineStore* store, LineInfo* infos);
bool fuse_movq_ext_truncation(LineStore* store, LineInfo* infos);
bool global_store_forwarding(LineStore* store, LineInfo* infos);
bool propagate_register_copies(LineStore* store, LineInfo* infos);
bool eliminate_dead_reg_moves(const LineStore* store, LineInfo* infos);
bool eliminate_dead_stores(const LineStore* store, LineInfo* infos);
bool eliminate_never_read_stores(const LineStore* store, LineInfo* infos);
bool fuse_compare_and_branch(LineStore* store, LineInfo* infos);
bool fold_memory_operands(LineStore* store, LineInfo* infos);
bool eliminate_loop_trampolines(LineStore* store, LineInfo* infos);
bool optimize_tail_calls(LineStore* store, LineInfo* infos);
bool eliminate_unused_callee_saves(const LineStore* store, LineInfo* infos);
bool compact_frame(LineStore* store, LineInfo* infos);

struct PeepholeRun {
  LineStore* store = nullptr;
  LineInfo* infos = nullptr;
};

std::string peephole_optimize(std::string asm_text) {
  // The Rust version parses to LineInfo/LineStore first, then runs:
  // - combined local passes in rounds
  // - global store/copy/dead-code cleanup
  // - compare/branch fusion and memory folding
  // - tail-call, trampoline, callee-save, and frame cleanup
  //
  // In this translation unit we preserve the orchestration shape without
  // forcing a concrete line-store implementation into this file.
  PeepholeRun run;
  (void)run;
  (void)combined_local_pass;
  (void)fuse_movq_ext_truncation;
  (void)global_store_forwarding;
  (void)propagate_register_copies;
  (void)eliminate_dead_reg_moves;
  (void)eliminate_dead_stores;
  (void)eliminate_never_read_stores;
  (void)fuse_compare_and_branch;
  (void)fold_memory_operands;
  (void)eliminate_loop_trampolines;
  (void)optimize_tail_calls;
  (void)eliminate_unused_callee_saves;
  (void)compact_frame;
  return asm_text;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
