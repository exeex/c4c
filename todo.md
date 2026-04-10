# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 1 audit translated x86 codegen coverage
- immediate target:
  map `src/backend/x86/codegen/*.cpp` into:
  compiled-and-reachable;
  translated-but-not-built;
  built-but-still-not-owning;
  and choose the first bounded integration slice

## Next Slice

- identify the first translated unit or cluster that can be added to the build
  without widening scope too far
- update `CMakeLists.txt` and the active x86 path for that slice
- then complete `peephole/passes/mod.cpp` orchestration and route emitted asm
  through `peephole_optimize(...)`

## Current Iteration Notes

- this plan was activated by explicit user priority override
- idea 44 remains open as the parked shared-BIR cleanup and legacy-matcher
  consolidation lane
- the current question is not "what more should be translated"
- the current question is "which already-translated x86 codegen pieces can be
  made real and reachable first"
- current evidence shows most of `src/backend/x86/codegen/*.cpp` is still not
  in the build, while `emit.cpp` remains the practical owner of x86 emission

## Recently Completed

- created idea 43 to own x86 peephole pipeline completion
- parked idea 44 as a separate shared-BIR cleanup lane
- switched the active runbook and execution state to idea 43
- reprioritized idea 43 so integrating already-translated x86 codegen units is
  now ahead of peephole-only work
