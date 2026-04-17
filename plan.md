# X86 Backend Next Capability-Family Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Retarget the active runbook after the completed pointer-backed same-module
global lane so the next packet starts by naming the next bounded
`x86_backend` capability family instead of drifting under exhausted plan text.

## Goal

Name one honest next capability family from the remaining `x86_backend`
failures, then land one bounded slice that moves a real cluster without
expectation weakening or testcase overfit.

## Core Rule

Keep progress explained by shared backend capability growth. Do not weaken
`x86_backend` expectations, do not reintroduce fallback LLVM IR acceptance,
and do not add testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/globals.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/globals.cpp)
- [tests/c/external/c-testsuite/src/00045.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00045.c)
- [tests/c/external/c-testsuite/src/00051.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00051.c)
- [tests/c/external/c-testsuite/src/00189.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00189.c)

## Current Targets

- Start from the post-pointer-backed checkpoint recorded in `todo.md`, not the
  older creation-day `18/220` baseline in the source idea.
- Spend the first packet naming the next bounded family from the remaining
  failures before widening codegen or semantic lowering again.
- Prefer one family whose probes should move together if a shared capability
  is repaired honestly.
- Keep adjacent families explicit when they are not required by the chosen
  lane.

## Non-Goals

- Reopening the completed pointer-backed same-module global lane as routine
  follow-up.
- Hiding a route change inside an implementation packet.
- Mixing bootstrap scalar globals, multi-function prepared-module work, and
  multi-block control flow into one packet without Step 1 explicitly justifying
  that boundary.
- Accepting fallback LLVM IR or weakening supported-path expectations.
- Adding testcase-shaped helpers or recognizers keyed to rendered output.

## Working Model

- The current source idea remains open, but the prior runbook is exhausted.
- The next packet must first re-baseline the remaining failure surface into one
  dominant family instead of assuming the previous prepared-module frontier is
  still the best target.
- Candidate remaining families currently visible from the last packet include
  bootstrap scalar globals, multi-function prepared-module routes, and
  multi-block control flow, but Step 1 must verify the next route from current
  evidence before code changes begin.
- If the next honest family sits outside the prepared x86 handoff/emitter
  boundary, record that explicitly rather than silently broadening scope.

## Execution Rules

- Step 1 must name one family, one proving cluster, and the nearby out-of-scope
  shapes before implementation starts.
- Keep packet churn in `todo.md`; use `plan.md` only for genuine route
  checkpoint changes.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family `c_testsuite_x86_backend_*` probes, then `x86_backend`
  checkpoint once a coherent slice exists.
- If Step 1 shows a distinct initiative that should live outside this source
  idea, stop and record that lifecycle blocker instead of mutating the plan ad
  hoc.

## Step 1. Re-Baseline The Remaining Frontier

Goal: Group the post-pointer-backed failures into credible next-family
candidates before choosing new code work.

Primary targets:
- current `x86_backend` checkpoint evidence
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- inspect the current failure surface after the completed pointer-backed lane
- cluster remaining failures by one dominant capability blocker rather than by
  testcase name
- separate neighboring families such as bootstrap scalar globals,
  multi-function prepared-module routes, and multi-block control flow instead
  of merging them prematurely
- stop and record a route note if the best next family no longer fits the
  active source-idea path

Completion check:
- one or two credible next-family candidates are named with clear evidence and
  without testcase-by-testcase repair framing

## Step 2. Name The Next Bounded Family

Goal: Choose exactly one next capability family and its proving cluster.

Actions:
- select the smallest family whose nearby probes should move together if one
  shared capability is widened honestly
- record the intended proving cluster and the nearby out-of-scope neighbors
- make explicit whether the chosen route stays at the prepared x86
  handoff/emitter boundary or needs broader semantic work

Completion check:
- one bounded family and one proving cluster are named clearly enough for a
  later implementation packet

## Step 3. Extend The Chosen Family Honestly

Goal: Implement the smallest shared capability widening required by the chosen
family.

Primary targets:
- implementation files selected by Step 2's family choice

Actions:
- change shared lowering or backend logic only where the chosen family
  requires it
- keep adjacent families unsupported when they are not part of the named lane
- do not use expectation downgrades, testcase-named shortcuts, or rendered-text
  matching as proof of progress

Completion check:
- the chosen family is admitted through shared logic rather than testcase
  recognition

## Step 4. Keep The Boundary Truthful

Goal: Describe the newly supported family and the nearby unsupported boundary
explicitly.

Primary targets:
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- revise backend notes and handoff coverage to describe the chosen family by
  capability
- keep nearby unsupported neighbors explicit when they remain outside the lane
- ensure the tests still prove honest prepared-module or lowering boundaries
  rather than adapter growth

Completion check:
- backend notes and handoff tests describe the supported family and the nearby
  unsupported boundary truthfully

## Step 5. Prove Nearby Same-Family Cases

Goal: Show the slice improved a capability family instead of one probe.

Actions:
- run the narrow backend tests for the touched boundary
- run the chosen same-family `c_testsuite_x86_backend_*` probes
- once a coherent slice exists, run the `x86_backend` checkpoint to measure the
  truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  family blocker instead of pretending success
