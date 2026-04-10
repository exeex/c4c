# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 3 continue expanding the translated x86 peephole subtree beyond the
  first live optimization round
- immediate target:
  identify the next translated pass that can be made live without depending on
  a broader x86 emitter syntax rewrite, likely by either broadening the shared
  classifier/helpers or by choosing another pass with a syntax-agnostic seam

## Next Slice

- make additional translated local cleanup rules real only when they can be
  expressed against the current emitted syntax or after the x86 emitter syntax
  boundary is normalized
- evaluate whether more of `peephole/passes/*.cpp` can be compiled in without
  pulling in unrelated x86 top-level codegen ownership
- investigate whether `compare_branch.cpp` can be safely compiled in next after
  consolidating the duplicated `mark_nop` and `replace_line` helpers it needs,
  while keeping Intel-syntax countdown-loop output unchanged

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
- rebuilt and reran the full ctest suite with monotonic results:
  `181` failures before, `181` failures after, no newly failing tests
