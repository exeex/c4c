# X86 Backend Prepared-Module Frontier Re-Baseline Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Checkpoint the active runbook after the bounded single-defined-function direct
external-call prepared-module lane landed. The next route must be chosen
honestly from the remaining frontier instead of silently widening the completed
`00131` / `00211` slice into `00210` or another adjacent family.

## Goal

Re-baseline the prepared-module frontier around `00210` and nearby neighbors,
then name one truthful next bounded family for the source idea.

## Core Rule

Keep progress explained by shared backend capability growth. Do not weaken
`x86_backend` expectations, do not reintroduce fallback LLVM IR acceptance,
and do not add testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/calls.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/calls.cpp)
- [tests/c/external/c-testsuite/src/00210.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00210.c)
- [tests/c/external/c-testsuite/src/00189.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00189.c)
- [tests/c/external/c-testsuite/src/00057.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00057.c)
- [tests/c/external/c-testsuite/src/00124.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00124.c)

## Current Targets

- Treat the landed `00131` / `00211` single-defined-function direct
  external-call lane as the baseline that must remain supported.
- Re-measure `c_testsuite_x86_backend_src_00210_c` as the adjacent
  multi-defined-function prepared-module neighbor.
- Keep `00189`, `00057`, and `00124` explicit as nearby indirect/runtime,
  emitter/control-flow, and scalar-control-flow neighbors while choosing the
  next lane.
- Use current `x86_backend` checkpoint evidence plus backend notes and handoff
  coverage to decide whether `00210` is the next honest family or whether a
  different nearby family should take priority.

## Non-Goals

- Treating `00210` as automatically in scope just because it is adjacent to
  the completed lane.
- Satisfying the next packet by globally deleting the prepared-module
  defined-function gate.
- Mixing multi-defined-function routing, indirect/runtime plumbing, and
  unrelated control-flow growth into one packet without an explicit checkpoint.
- Claiming progress through expectation downgrades, testcase-named shortcuts,
  or rendered-text matching.

## Working Model

- The linked source idea remains open.
- The prior runbook goal is complete: the bounded single-defined-function
  direct external-call lane now admits `00131` and `00211`.
- `00210` remains the most obvious adjacent boundary because it crosses the
  multi-defined-function prepared-module consumer boundary, but that does not
  make it the next honest packet automatically.
- The next coherent slice must start by re-baselining the remaining frontier
  and naming one bounded family with explicit out-of-scope neighbors.

## Execution Rules

- Keep packet churn in `todo.md`; rewrite `plan.md` again only for a genuine
  route checkpoint.
- Validation ladder per packet: build, narrow backend proof, chosen
  same-family `c_testsuite_x86_backend_*` probes, then `x86_backend`
  checkpoint once a coherent slice exists.
- If Step 1 shows that the next honest family is not the `00210`
  multi-defined-function boundary, record that route note explicitly instead of
  forcing `00210` back into scope.
- Do not delegate implementation until one family, one proving cluster, and
  the adjacent out-of-scope neighbors are named clearly.

## Step 1. Re-Baseline The Remaining Frontier

Goal: Determine the smallest honest next family after the completed direct
external-call lane.

Primary targets:
- current `x86_backend` checkpoint evidence
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/c/external/c-testsuite/src/00210.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00210.c)

Actions:
- inspect the current failure surface after the completed `00131` / `00211`
  lane
- measure whether `00210` is still blocked specifically at the
  multi-defined-function prepared-module boundary
- compare that route against nearby neighbors such as `00189`, `00057`, and
  `00124` so the next packet is chosen by capability family rather than
  testcase convenience
- stop and record a route note if another family is the more honest next lane

Completion check:
- one next-family candidate and its nearby out-of-scope neighbors are named
  clearly without testcase-by-testcase repair framing

## Step 2. Name The Next Bounded Family

Goal: Turn the Step 1 evidence into one bounded implementation lane.

Actions:
- choose exactly one prepared-module or lowering family whose nearby probes
  should move together
- record the intended proving cluster for that family
- keep the already-landed `00131` / `00211` lane explicit as baseline support
- make the non-goals for the next packet explicit before implementation starts

Completion check:
- one bounded family and one proving cluster are named clearly enough for a
  later implementation packet

## Step 3. Extend The Chosen Family Honestly

Goal: Implement the smallest shared capability widening required by the chosen
next family.

Actions:
- touch only the canonical prepared-module consumer or directly supporting
  lowering/codegen surfaces needed by the chosen family
- reuse shared lowering and x86 codegen paths instead of adding
  testcase-shaped recognizers
- keep unrelated neighbors out of scope until the chosen lane is proven
- do not use expectation downgrades, testcase-named shortcuts, or rendered-
  text matching as proof of progress

Completion check:
- the chosen family advances through shared backend capability work rather than
  one named testcase

## Step 4. Keep The Boundary Truthful

Goal: Describe the newly supported family and adjacent unsupported boundary
honestly.

Primary targets:
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- revise backend notes and handoff coverage to describe the supported family
  by capability
- keep the next unsupported boundary explicit when it remains outside the lane
- ensure the tests still prove honest prepared-module boundaries rather than
  adapter growth

Completion check:
- backend notes and handoff tests describe the supported family and nearby
  unsupported boundary truthfully

## Step 5. Prove Nearby Same-Family Cases

Goal: Show the slice improved a capability family instead of one probe.

Actions:
- run the narrow backend tests for the touched boundary
- run the chosen same-family `c_testsuite_x86_backend_*` probes
- keep adjacent out-of-scope boundaries explicit when they remain unsupported
- once a coherent slice exists, run the `x86_backend` checkpoint to measure
  the truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  family blocker instead of pretending success
