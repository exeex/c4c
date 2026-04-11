# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Source Plan: plan.md

## Current Active Item

- Step 1 owner-blocker audit reset:
  rebuild the active queue around the shared `x86_codegen.hpp` surface and the
  next translated owner cluster, instead of continuing direct-call testcase
  expansion

## Immediate Target

- confirm which translated top-level x86 owner files are blocked by missing
  public declarations on `X86Codegen`
- choose the smallest cluster that can become real runtime ownership next
- define one proving backend test slice for that cluster

## Completed Context Carried Forward

- translated peephole optimization is already wired into the active x86 path
- `globals.cpp` is already compiled and carries bounded translated-owner
  behavior
- `returns.cpp` already has symbol/link coverage in the build, but its real
  translated bodies still need more public `X86Codegen` surface
- several direct-call spacing/suffix regressions were added in prior
  iterations; treat them as existing guardrails, not as the current work queue

## Guardrails For This Activation

- do not add more direct-call testcase families unless the active owner slice
  cannot be proven without one specific new test
- do not widen into branch/select/shared-BIR cleanup during this activation
- if a gap is useful but not required for the owner transfer, record it back in
  the source idea instead of extending this todo

## Next Slice After Current Item

- Step 2 shared-header surfacing for the chosen owner cluster
- then Step 3 runtime cutover of that cluster out of legacy `emit.cpp`

## Open Questions

- whether `calls.cpp` is still the best first translated owner cluster after a
  fresh audit of public type/state blockers
- whether `returns.cpp` plus `prologue.cpp` now forms a smaller practical
  cutover than `calls.cpp`
