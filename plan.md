# X86 Backend C-Testsuite Capability Families Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Turn the current x86 backend c-testsuite fail surface into capability-family
packets, starting with one honest local-memory lane instead of testcase-shaped
repair.

## Goal

Land one bounded local-memory capability slice that improves truthful
`x86_backend` coverage through semantic `lir_to_bir` and the prepared x86
handoff.

## Core Rule

Grow shared backend capability by family. Do not weaken expectations, do not
reintroduce fallback LLVM IR acceptance, and do not add testcase-named or
rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

## Current Targets

- Start with the local-memory semantic family, not control-flow or cast work.
- Choose a small cluster of failing `x86_backend` c-testsuite cases that share
  one straight-line stack/object addressing blocker.
- Keep the slice honest through shared `lir_to_bir` semantics and prepared
  module ingestion.

## Non-Goals

- Solving the full x86 backend c-testsuite fail surface in one pass.
- Reviving adapter-growth or testcase-specific backend shortcuts.
- Weakening `x86_backend` expectations or accepting fallback IR.
- Expanding primarily into multi-block control flow, broad global-data work, or
  unrelated backend families.

## Working Model

- Family naming belongs to capability boundaries, not individual tests.
- The first slice should cover stack-object creation, addressed local
  loads/stores, and simple constant-offset addressing in already-supported
  control flow.
- Shared lowering should explain the progress. Target codegen should consume an
  honest prepared module rather than a bespoke special case.

## Execution Rules

- Prefer one coherent lane across several nearby cases over one testcase win.
- Use backend notes and handoff tests to define the supported boundary by
  family.
- Record routine packet state in `todo.md`; do not rewrite this runbook for
  normal execution churn.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family c-testsuite probes, then checkpoint `x86_backend` only when the
  slice is coherent enough to claim pass-count movement.

## Step 1. Name The First Local-Memory Lane

Goal: Choose one dominant local-memory capability lane and its proving cluster
before code edits widen.

Primary targets:
- failing `x86_backend` c-testsuite cases from the same local-memory family
- existing backend notes and handoff tests that describe the current boundary

Actions:
- inspect the current `x86_backend` local-memory failures and group them by one
  straight-line stack/object addressing shape
- select a small proving cluster of nearby cases that should move together if
  the lane is repaired honestly
- record any adjacent unsupported shapes that must remain out of scope for the
  first slice

Completion check:
- one capability lane and one same-family proving cluster are named without
  drifting into testcase-by-testcase repair

## Step 2. Lower The Local-Memory Lane Honestly

Goal: Make semantic `lir_to_bir` produce the prepared-module form needed by the
selected local-memory lane.

Primary targets:
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir.hpp`

Actions:
- implement the smallest shared lowering needed for stack-object creation,
  addressed local loads/stores, and simple constant-offset addressing on the
  chosen lane
- keep unsupported notes truthful for nearby out-of-scope shapes
- do not add named-case shortcuts or expectation rewrites

Completion check:
- the chosen local-memory lane lowers through shared semantic BIR logic without
  testcase-shaped recognition

## Step 3. Keep The X86 Prepared Handoff Honest

Goal: Ensure the prepared x86 path accepts the new lane and still rejects
unsupported families truthfully.

Primary targets:
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Actions:
- update the prepared-module handoff only where the new lane requires honest
  ingestion
- add or revise backend notes and handoff tests so the capability boundary is
  described by family, not by testcase name
- keep unsupported notes for control-flow, cast, or broader memory shapes that
  remain out of scope

Completion check:
- backend notes and handoff tests prove the selected lane is supported and the
  nearby unsupported boundary remains explicit

## Step 4. Prove Nearby Same-Family Cases

Goal: Show the slice improves a capability family instead of one probe.

Actions:
- run the narrow backend tests for notes and handoff coverage
- run the chosen same-family `c_testsuite_x86_backend_*` probes
- once a coherent slice exists, run the `x86_backend` checkpoint to measure the
  truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  capability blocker instead of pretending success
