# X86 Backend Prepared-Module Call Route Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Repair the active runbook after fresh rebuilt-source evidence showed that the
current `00210`-anchored indirect-callee framing is stale. The next honest
lane is smaller: bounded single-defined-function prepared modules that already
lower direct external calls correctly in semantic BIR but still fail at the
canonical x86 prepared-module consumer boundary.

## Goal

Land one bounded slice that admits the single-defined-function direct
external-call prepared-module lane for cases like `00131` and `00211`, while
keeping the adjacent multi-defined-function `00210` route explicit and out of
scope until it is named honestly.

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
- [tests/c/external/c-testsuite/src/00131.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00131.c)
- [tests/c/external/c-testsuite/src/00211.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00211.c)
- [tests/c/external/c-testsuite/src/00210.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00210.c)

## Current Targets

- Start from the rebuilt-source blocker notes recorded in `todo.md`, not the
  stale `00210` indirect-callee route from the prior runbook.
- Treat `c_testsuite_x86_backend_src_00131_c` and
  `c_testsuite_x86_backend_src_00211_c` as the next proving cluster for the
  bounded single-defined-function direct external-call prepared-module lane.
- Treat `c_testsuite_x86_backend_src_00210_c` as the adjacent
  multi-defined-function neighbor that currently still trips the
  single-function prepared-module gate and should remain out of scope for the
  next executor packet.
- Keep broader neighbors explicit when they are not required by this lane:
  `00189` remains the indirect/global-function-pointer/variadic-runtime
  neighbor, while `00057` and `00124` remain emitter/control-flow and
  scalar-control-flow neighbors.

## Non-Goals

- Hiding another route change inside an implementation packet.
- Treating `00210` as the proving anchor before the smaller single-defined-
  function external-call lane is admitted.
- Satisfying the next slice by deleting the `functions.size() != 1` rejection
  globally or by adding testcase-shaped recognizers.
- Mixing multi-defined-function routing, indirect/runtime plumbing, bootstrap
  scalar globals, and control-flow growth into one packet without an explicit
  runbook checkpoint.
- Accepting fallback LLVM IR or weakening supported-path expectations.

## Working Model

- The current source idea remains open, but the prior runbook is exhausted.
- Fresh rebuilt-source checks showed that `00210` already lowers
  `actual_function` as a direct same-module call in semantic BIR, so the old
  indirect-callee-identity framing no longer matches reality.
- The surviving honest blocker is smaller and earlier at the x86 prepared-
  module consumer boundary: even simpler single-defined-function cases with
  direct external calls such as `00131` and `00211` are still not admitted.
- The next coherent packet therefore targets one bounded prepared-module call
  family: single-defined-function modules with otherwise already-supported
  control flow and direct external calls.
- `00210` remains a truthful adjacent blocker because it also crosses the
  multi-defined-function prepared-module boundary; do not silently fold that
  broader route back into the next packet.

## Execution Rules

- Step 1 must name one family, one proving cluster, and the nearby out-of-
  scope shapes before implementation starts.
- Keep packet churn in `todo.md`; use `plan.md` only for genuine route
  checkpoint changes.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family `c_testsuite_x86_backend_*` probes, then `x86_backend`
  checkpoint once a coherent slice exists.
- If Step 1 shows that the smaller single-defined-function external-call lane
  is still not the honest next family, stop and record that lifecycle blocker
  instead of silently broadening the plan again.
- Do not delegate a packet that claims `00210` progress unless the plan has
  been updated again to justify widening from the single-defined-function lane
  into the multi-defined-function boundary.

## Step 1. Re-Baseline The Remaining Frontier

Goal: Confirm the next honest family after the rebuilt-source correction.

Primary targets:
- current `x86_backend` checkpoint evidence
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- inspect the current failure surface after the completed pointer-backed lane
- confirm that `00131` and `00211` move together as single-defined-function
  external-call cases through the canonical prepared-module handoff
- keep `00210` explicit as the adjacent multi-defined-function neighbor instead
  of using it as the primary route anchor
- stop and record a route note if the honest next family no longer fits this
  source-idea path

Completion check:
- one bounded next-family candidate and its adjacent out-of-scope neighbor are
  named clearly without testcase-by-testcase repair framing

## Step 2. Name The Next Bounded Family

Goal: Choose exactly one next capability family and its proving cluster.

Actions:
- select the smallest family whose nearby probes should move together if one
  shared prepared-module capability is widened honestly
- record the intended proving cluster around `00131` and `00211`
- make explicit that `00210` remains outside the next packet because it also
  needs multi-defined-function prepared-module support
- keep `00189`, `00057`, and `00124` explicit as out-of-scope neighbors

Completion check:
- one bounded family and one proving cluster are named clearly enough for a
  later implementation packet

## Step 3. Extend The Chosen Family Honestly

Goal: Implement the smallest shared capability widening required by the chosen
single-defined-function prepared-module direct external-call family.

Primary targets:
- the canonical x86 prepared-module consumer and any directly supporting call
  handling it requires

Actions:
- admit bounded single-defined-function modules whose bodies stay within the
  already-supported minimal return/control-flow envelope while issuing direct
  external calls
- preserve the single-defined-function boundary for this packet instead of
  broadening immediately into multi-defined-function modules
- reuse shared call/data emission paths instead of adding testcase-shaped
  prepared-module matchers
- keep multi-defined-function `00210`, broader `00189`-style indirect/runtime
  plumbing, and control-flow growth out of scope
- do not use expectation downgrades, testcase-named shortcuts, or rendered-
  text matching as proof of progress

Completion check:
- the chosen family is admitted through shared prepared-module call support and
  shared x86 codegen logic rather than testcase recognition

## Step 4. Keep The Boundary Truthful

Goal: Describe the newly supported family and the nearby unsupported boundary
explicitly.

Primary targets:
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- revise backend notes and handoff coverage to describe the bounded
  single-defined-function external-call family by capability
- keep the adjacent multi-defined-function `00210` boundary explicit when it
  remains outside the lane
- ensure the tests still prove honest prepared-module boundaries rather than
  adapter growth

Completion check:
- backend notes and handoff tests describe the supported family and the nearby
  unsupported boundary truthfully

## Step 5. Prove Nearby Same-Family Cases

Goal: Show the slice improved a capability family instead of one probe.

Actions:
- run the narrow backend tests for the touched boundary
- run the chosen same-family `c_testsuite_x86_backend_*` probes for `00131`
  and `00211`
- keep `00210` as a neighboring boundary check only if the packet explicitly
  measures that out-of-scope edge
- once a coherent slice exists, run the `x86_backend` checkpoint to measure the
  truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  family blocker instead of pretending success
