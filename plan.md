# X86 Backend C-Testsuite Capability Families Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Continue the x86 backend capability-family route after the completed first
local-memory slice by targeting one bounded prepared-handoff guard lane.

## Goal

Land one honest straight-line prepared-module guard slice that broadens
truthful `x86_backend` coverage without reopening global-data, scalar-cast, or
multi-block control-flow work.

## Core Rule

Grow shared x86 prepared-module capability by family. Do not weaken
expectations, do not reintroduce fallback LLVM IR acceptance, and do not add
testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/c/external/c-testsuite/src/00047.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00047.c)
- [tests/c/external/c-testsuite/src/00048.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00048.c)
- [tests/c/external/c-testsuite/src/00049.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00049.c)
- [tests/c/external/c-testsuite/src/00054.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00054.c)
- [tests/c/external/c-testsuite/src/00055.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00055.c)

## Current Targets

- Stay within the prepared x86 handoff/emitter boundary, not another semantic
  `lir_to_bir` expansion.
- Choose a small cluster of `x86_backend` failures that already prepare
  honestly but still stop at the minimal straight-line return or guard lane.
- Prefer probes whose common blocker is a straight-line scalar compare/guard
  chain over already-prepared operands.

## Non-Goals

- Reopening the completed first local-memory lane as routine follow-up.
- Using this packet to repair scalar-cast, bootstrap global-data, or multi-
  block control-flow families.
- Accepting fallback LLVM IR or weakening `x86_backend` expectations.
- Adding testcase-shaped emit helpers or recognizers keyed to rendered output.

## Working Model

- The next bounded family is an x86 prepared-handoff guard lane, not another
  semantic memory-lowering lane.
- Candidate probes should already reach the prepared-module path and differ
  mainly in straight-line compare/test plus early-return behavior.
- Progress must be explained by shared emitter or handoff capability, not by
  one testcase win.

## Execution Rules

- Name one guard family before widening codegen support.
- Keep out-of-scope neighboring failures explicit in `todo.md` instead of
  silently broadening the route.
- Use backend notes and handoff tests to describe the supported boundary by
  family.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family c-testsuite probes, then checkpoint `x86_backend` only once a
  coherent cluster moves together.

## Step 1. Name The First Prepared Guard Lane

Goal: Choose one dominant straight-line guard family and its proving cluster
before x86 codegen changes widen.

Primary targets:
- failing `x86_backend` c-testsuite cases that already prepare honestly
- current x86 handoff tests that define the supported straight-line boundary

Actions:
- inspect the current minimal return-path failures and group them by one
  straight-line compare/test plus early-return shape
- select a small proving cluster that should move together if the guard lane is
  widened honestly
- record nearby out-of-scope shapes that still require global-data,
  scalar-cast, or multi-block control-flow work

Completion check:
- one prepared guard lane and one same-family proving cluster are named without
  drifting into testcase-by-testcase repair

## Step 2. Extend The Prepared Guard Lane Honestly

Goal: Teach the x86 prepared-module path to accept the chosen straight-line
guard family.

Primary targets:
- `src/backend/mir/x86/codegen/x86_codegen.hpp`

Actions:
- implement the smallest shared prepared-handoff/codegen widening needed for
  the chosen straight-line scalar compare/test plus early-return lane
- keep unsupported notes truthful for neighboring out-of-scope families
- do not add testcase-named shortcuts or rendered-text recognizers

Completion check:
- the chosen guard lane is accepted through shared x86 prepared-module logic
  without testcase-shaped recognition

## Step 3. Keep The Boundary Truthful

Goal: Describe the supported prepared guard family and the remaining nearby
unsupported boundary explicitly.

Primary targets:
- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`

Actions:
- revise handoff coverage so the new guard lane is described by family rather
  than testcase name
- keep nearby unsupported notes or handoff expectations explicit for
  scalar-cast, bootstrap global-data, and multi-block control-flow families
- ensure the tests still prove prepared-module honesty rather than legacy
  adapter growth

Completion check:
- backend notes and handoff tests prove the new guard lane is supported and
  the nearby unsupported boundary remains explicit

## Step 4. Prove Nearby Same-Family Cases

Goal: Show the slice improves a capability family instead of one probe.

Actions:
- run the narrow backend tests for notes and handoff coverage
- run the chosen same-family `c_testsuite_x86_backend_*` probes
- once a coherent slice exists, run the `x86_backend` checkpoint to measure the
  truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  family blocker instead of pretending success
