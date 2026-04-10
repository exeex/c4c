# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 3 continue expanding the translated x86 peephole subtree beyond the
  first live optimization round
- immediate target:
  evaluate the next bounded stack-load trampoline gap after the new 32-bit
  `%eax` spill plus `movslq` copy-back slice
  - keep the next rewrite scope limited to one explicit reload/copy-back form
    with recorded predecessor-store and fallthrough safety rules instead of
    widening into generic spill coalescing

## Next Slice

- evaluate whether the remaining stack-load trampoline cases beyond the new
  single-slot `%eax` spill plus `movslq` copy-back form should reuse the same
  detection path or stay parked until a separate bounded slice with explicit
  safety rules for wider spill shapes
- keep `frame_compact.cpp` parked until dead-store and callee-save translated
  passes are live enough for frame shrinking to be meaningful
- continue evaluating which remaining translated peephole units can be compiled
  into the real path without widening into unrelated x86 top-level ownership

## Current Iteration Notes

- this plan was activated by explicit user priority override
- idea 44 remains open as the parked shared-BIR cleanup and legacy-matcher
  consolidation lane
- the current question is not "what more should be translated"
- the current question is "which already-translated x86 codegen pieces can be
  made real and reachable first"
- current evidence shows most of `src/backend/x86/codegen/*.cpp` is still not
  in the build, while `emit.cpp` remains the practical owner of x86 emission
- the active inventory for this plan is now explicit inside idea 43:
  16 translated top-level codegen units plus the translated peephole subtree
- the translated peephole subtree is the first bounded reachable slice because
  it has a self-contained string-in/string-out seam even though the current
  translated parser helpers still need a shared header and the emitted x86 asm
  uses Intel syntax rather than the ref tree's AT&T-shaped patterns
- the active x86 route now passes through the translated peephole entrypoint in
  both the direct x86 wrapper and the shared backend assembly handoff path
- the first live optimization is syntax-agnostic redundant jump-to-immediately-
  following-label elimination; the rest of the translated local-pass inventory
  remains parked until its assumptions are reconciled with the current emitter
- the translated compare-and-branch pass needed one shared classifier seam
  before it became reachable: `types.cpp` now classifies `cmp*` lines as
  `LineKind::Cmp`, which lets the pass fuse cmp/setcc/test/jcc sequences on the
  real x86 peephole path
- the loop-trampoline coalescing slice exposed another operand-classification
  seam: many comma-separated AT&T instructions still leave `LineInfo.dest_reg`
  unset because the trailing operand is not trimmed before family lookup, so
  the live trampoline pass currently uses a local trimmed-destination fallback
  instead of broadening that shared parser mid-slice
- this iteration promotes that trailing-operand trim into the shared
  classifier/helper path because the next translated trampoline case
  (`movslq` copy-back) would otherwise duplicate the same local fallback again
- the aggregate `backend_bir_tests` runner still crashes in pre-existing typed
  LIR validation coverage before it can serve as a narrow peephole harness, so
  this iteration verified the new trampoline form with a focused standalone
  peephole harness while the full-suite regression guard remained monotonic
- the next bounded gap against the translated loop-trampoline behavior is
  mixed safe/unsafe shuffle handling: the current C++ pass still bails out of
  the whole trampoline when one sibling move cannot be coalesced, while the
  reference keeps the unsafe sibling parked and still applies the safe rewrite
- this iteration closes the simplest parked stack-load gap: the live pass now
  recognizes a single-slot `%rax` spill in the predecessor plus an immediate
  trampoline reload/copy-back pair and rewrites that shape onto the
  loop-carried register without widening into generic spill analysis
- the next bounded gap after this iteration is the remaining stack-load family:
  wider spill forms should stay parked until their predecessor-store matching
  and fallthrough safety rules are explicit instead of inferred ad hoc
- this iteration closes one more bounded stack-load shape without widening the
  shared parser or the predecessor scan into generic spill analysis: the live
  pass now accepts a single-slot `%eax` spill plus trampoline reload and
  `movslq` copy-back onto the loop-carried register
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new regression, so this slice again used a focused
  standalone peephole harness while the full-suite regression guard stayed
  monotonic

## Recently Completed

- created idea 43 to own x86 peephole pipeline completion
- parked idea 44 as a separate shared-BIR cleanup lane
- switched the active runbook and execution state to idea 43
- reprioritized idea 43 so integrating already-translated x86 codegen units is
  now ahead of peephole-only work
- added the first bounded translated x86 peephole compile-in cluster to the
  build: `peephole/mod.cpp`, `peephole/types.cpp`,
  `peephole/passes/helpers.cpp`, `peephole/passes/local_patterns.cpp`, and
  `peephole/passes/mod.cpp`
- added a shared peephole header so that cluster now compiles as a real unit
  instead of parked source inventory
- routed x86 emitted asm through `peephole_optimize(...)` on both the direct
  x86 wrapper path and the shared backend assembly handoff path
- added targeted backend tests that pin the live redundant-jump cleanup and
  confirm emitted countdown-loop asm reaches the peephole stage
- compiled the translated `peephole/passes/push_pop.cpp` unit into the real
  x86 backend and test builds
- wired the safe redundant `pushq`/`popq` elimination pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the translated push/pop pass now
  removes a redundant pair while preserving the surrounding label and return
- compiled the translated `peephole/passes/compare_branch.cpp` unit into the
  real x86 backend and test builds
- wired the translated compare-and-branch fusion pass into the live x86
  peephole optimization loop
- added a direct regression test that proves the live x86 peephole now fuses a
  cmp/setcc/test/jne boolean-materialization sequence into a direct conditional
  jump
- compiled the translated `peephole/passes/loop_trampoline.cpp` unit into the
  real x86 backend and test builds
- wired a conservative single-entry trampoline rewrite into the live x86
  peephole optimization loop before the generic adjacent-jump cleanup
- added a direct regression test that proves the live x86 peephole now
  rewrites a sole incoming branch away from a label-only trampoline block onto
  the real loop header while preserving the current emitted label syntax
- widened the live translated loop-trampoline pass to coalesce a bounded
  register-register `movq` trampoline copy back into the predecessor update
  chain before redirecting the branch onto the real loop header
- added a direct regression test that proves the live x86 peephole now rewrites
  `movq %loop_reg, %tmp` / mutate `%tmp` / trampoline `movq %tmp, %loop_reg`
  into a direct mutation of the loop-carried register
- promoted trailing-operand trimming into the shared x86 peephole
  classifier/helper path so comma-delimited destinations no longer leave
  `LineInfo.dest_reg` unset just because the trailing operand still carries
  whitespace
- widened the live translated loop-trampoline pass to recognize and coalesce a
  bounded `movslq %tmpd, %loop_reg` trampoline copy-back onto the real
  loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  a 32-bit update feeding a `movslq` trampoline copy-back into a direct update
  of the loop-carried register
- widened the live translated loop-trampoline pass to coalesce a proven-safe
  register shuffle inside a mixed trampoline block even when a sibling move
  still has to stay parked
- taught the loop-trampoline predecessor scan to reject rewrites that would
  read the loop-carried destination while also writing the temporary, avoiding
  bogus self-referential rewrites such as `addq %r10, %r10`
- added a direct regression test that proves the live x86 peephole now removes
  only the safe `%r9/%r14` shuffle from a mixed trampoline while leaving the
  unsafe `%r10/%r15` pair and trampoline branch in place
- widened the live translated loop-trampoline pass to coalesce a conservative
  single-slot stack-spill trampoline when the predecessor computes into `%rax`,
  spills to one `(%rbp)` slot, and the trampoline immediately reloads `%rax`
  before copying back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded stack-spill trampoline onto the loop-carried register and drops
  the now-dead spill, reload, and trampoline copy-back sequence
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the predecessor computes through `%eax`,
  stores to one `(%rbp)` slot, and the trampoline reloads `%eax` before
  `movslq` copy-back into the loop-carried register
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded `movl`/`movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill, reload, and trampoline copy-back
  sequence
- rebuilt and reran the full ctest suite with monotonic results:
  `181` failures before, `181` failures after, no newly failing tests
