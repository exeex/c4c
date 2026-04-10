# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 3 continue expanding the translated x86 peephole subtree beyond the
  first live optimization round
- immediate target:
  evaluate the next bounded translated peephole pass after the
  copy-propagation slice
  - prefer store-forwarding or another non-ABI-sensitive pass before
    reopening tail-call, callee-save, or frame-compaction work

## Next Slice

- if the translated copy-propagation slice lands cleanly, evaluate whether the
  next bounded candidate should be store-forwarding or tail-call
  orchestration instead of widening loop-trampoline matching again
- keep the remaining stack-load family intentionally parked behind the current
  `%rax`/`%eax` predecessor-store and fallthrough rules unless a future slice
  proves one more explicit safe shape
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
- this iteration intentionally pivots away from that matcher growth lane:
  `dead_code.cpp` is already translated but still disconnected from the real
  orchestration/build, so Step 3 can make progress by activating another
  existing pass file with focused peephole-only coverage
- the translated dead-code slice landed as a bounded orchestration/build step:
  `peephole/passes/dead_code.cpp` now compiles into the real x86 backend and
  runs inside the live peephole round without widening unrelated matcher logic
- the aggregate `backend_bir_tests` runner still times out on pre-existing
  typed-LIR validation failures, so this iteration validated the new pass with
  direct filtered backend test invocations plus the full-suite regression guard
- the full-suite regression guard stayed monotonic for this slice:
  `2723` passed / `181` failed before and after, with no newly failing tests;
  the guard reported one `>30s` test (`backend_bir_tests`), matching the
  existing harness-timeout situation rather than a new regression
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
- this iteration closes the next bounded stack-load family member without
  widening into generic spill analysis: the live pass now accepts a single-slot
  `%rax` spill whose trampoline reload writes the loop-carried register
  directly via `movq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-reload regression, so this slice again used the
  standalone backend test filter plus the full-suite regression guard
- this iteration closes the direct sign-extending stack-reload variant without
  widening predecessor analysis: the live pass now accepts a single-slot
  `%eax` spill whose trampoline reload writes the loop-carried register
  directly via `movslq -N(%rbp), %loop_reg`
- the aggregate `backend_bir_tests` runner still aborts in the same
  pre-existing typed LIR validation coverage before it can serve as a narrow
  harness for the new direct-`movslq` regression, so this slice again uses the
  standalone backend test filter/subset plus the full-suite regression guard
- this iteration deliberately adds a negative regression instead of widening
  stack-load matching again: the live x86 peephole must keep the spill,
  reload, and trampoline branch parked when the branch fallthrough still
  consumes `%rax` after the bounded predecessor spill
- the new stack-load safety regression passed on the existing implementation,
  confirming the current fallthrough guard already enforces that `%rax`/`%eax`
  boundary
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_typedef_owned_functional_cast_perf`; rerunning that test in isolation
  passed, and a second full-suite rerun returned to the monotonic baseline
- this iteration promotes copy propagation because the translated pass already
  exists, is more substantive than the parked stub passes, and composes
  directly with the live dead-move cleanup without crossing into epilogue or
  frame-layout ownership
- the translated copy-propagation slice landed as another bounded
  orchestration/build step: `peephole/passes/copy_propagation.cpp` now
  compiles into the real x86 backend and runs inside the live peephole round
  without widening into ABI-sensitive cleanup passes
- the new direct regression deliberately gives the existing dead-move cleanup
  explicit overwrite proof after the propagated use so this slice stays about
  activating translated copy propagation rather than broadening dead-code
  semantics
- the first full-suite rerun picked up one extra unrelated failure in
  `cpp_qualified_template_call_template_arg_perf`; rerunning that test in
  isolation passed, and a second full-suite rerun returned to the same
  `2723` passed / `181` failed count as the baseline
- the aggregate `backend_bir_tests` entry still does not serve as a narrow
  peephole harness: on the rerun it reached the same pre-existing
  `test_bir_lowering_accepts_local_two_field_struct_sub_sub_two_return_module`
  crash path quickly enough to surface as `SEGFAULT` instead of the earlier
  `Timeout`, so the unstable aggregate runner remains a known blocker rather
  than a new peephole regression

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
- widened the live translated loop-trampoline pass to coalesce a bounded
  64-bit single-slot stack spill when the trampoline reloads the loop-carried
  register directly from `(%rbp)` instead of bouncing through `%rax`
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct-reload stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- widened the live translated loop-trampoline pass to coalesce a bounded
  32-bit single-slot stack spill when the trampoline sign-extends directly from
  `(%rbp)` into the loop-carried register instead of reloading `%eax` first
- added a direct regression test that proves the live x86 peephole now rewrites
  that bounded direct `movslq` stack-spill trampoline onto the loop-carried
  register and drops the now-dead spill and direct trampoline reload
- added a direct regression test that proves the live x86 peephole keeps a
  bounded stack-spill trampoline parked when the branch fallthrough still
  consumes `%rax`, locking in the current fallthrough-safety boundary instead
  of widening the matcher
- rebuilt and reran the full ctest suite with monotonic results:
  `183` failures before, `181` failures after, no newly failing tests
- rebuilt and reran the full ctest suite for this test-only safety slice with
  monotonic results: `181` failures before, `181` failures after, no newly
  failing tests
- compiled the translated `peephole/passes/dead_code.cpp` unit into the real
  x86 backend and test builds
- wired the translated dead register-move and overwritten stack-store cleanup
  passes into the live x86 peephole optimization loop
- added direct regression tests that prove the live x86 peephole now drops a
  dead `movq` whose destination is overwritten before any read and removes an
  earlier `(%rbp)` store when a later store overwrites the same slot before any
  read
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct dead-code test filters, and reran the full ctest suite with monotonic
  results: `181` failures before, `181` failures after, no newly failing tests
- compiled the translated `peephole/passes/copy_propagation.cpp` unit into the
  real x86 backend and test builds
- wired the translated register copy-propagation pass into the live x86
  peephole optimization loop ahead of the existing dead-code cleanup
- added a direct regression test that proves the live x86 peephole now
  propagates a transitive register-copy chain into a downstream ALU use and
  lets the existing overwrite-based dead-move cleanup drop the superseded
  copies
- rebuilt, ran the filtered `test_x86_peephole_` backend subset plus the new
  direct copy-propagation test filter, and reran the full ctest suite to the
  same `181`-failure count as the baseline; the stable rerun surfaced the
  existing aggregate `backend_bir_tests` crash path as `SEGFAULT` instead of
  the earlier timeout while keeping the failure set otherwise unchanged
