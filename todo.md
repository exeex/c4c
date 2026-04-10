# x86_64 Peephole Pipeline Completion

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 1 audit the translated peephole surface
- immediate target:
  confirm which translated passes are ready to run under the current C++
  `LineStore` / `LineInfo` implementation and define the minimum viable
  orchestration slice for `passes/mod.cpp`

## Next Slice

- compare the Rust reference pass order with the current C++ pass interfaces
- identify any signature or infrastructure mismatches that block orchestration
- choose the first enablement slice for `passes/mod.cpp`
- after orchestration is real, wire `peephole_optimize(...)` into x86 emission

## Current Iteration Notes

- this plan was activated by explicit user priority override
- idea 44 remains open as the parked shared-BIR cleanup and legacy-matcher
  consolidation lane
- the current question is not "what more should be translated"
- the current question is "what already-translated peephole pieces can be made
  real and reachable first"

## Recently Completed

- created idea 43 to own x86 peephole pipeline completion
- parked idea 44 as a separate shared-BIR cleanup lane
- switched the active runbook and execution state to idea 43
