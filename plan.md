# X86 Backend Same-Module Global Emission Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Continue the x86 backend capability-family route after the completed non-global
prepared guard lane by targeting one bounded same-module global-emission lane.

## Goal

Land one honest straight-line same-module defined-global slice that broadens
truthful `x86_backend` coverage without reopening scalar-cast, pointer-indirect
global, or multi-block control-flow work.

## Core Rule

Grow shared x86 prepared-module capability by family. Do not weaken
expectations, do not reintroduce fallback LLVM IR acceptance, and do not add
testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/globals.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/globals.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/c/external/c-testsuite/src/00047.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00047.c)
- [tests/c/external/c-testsuite/src/00048.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00048.c)
- [tests/c/external/c-testsuite/src/00049.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00049.c)
- [tests/c/external/c-testsuite/src/00045.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00045.c)

## Current Targets

- Stay within the prepared x86 handoff/emitter boundary, not another semantic
  `lir_to_bir` expansion.
- Choose a small cluster of `x86_backend` failures that already lower honestly
  except for same-module defined-global emission or guard handling.
- Prefer probes whose common blocker is fixed-offset reads from defined globals
  followed by straight-line equality guards and early returns.

## Non-Goals

- Reopening the completed non-global prepared guard lane as routine follow-up.
- Using this packet to repair scalar-cast, bootstrap scalar-global, or
  multi-block control-flow families.
- Accepting fallback LLVM IR or weakening `x86_backend` expectations.
- Adding testcase-shaped emit helpers or recognizers keyed to rendered output.
- Expanding to pointer-indirect or relocation-backed global routes just to pull
  `00049` into the packet.

## Working Model

- The next bounded family is same-module defined-global emission, not pointer
  indirection or broader global-address materialization.
- Candidate probes should already reach the prepared-module path and differ
  mainly in fixed-offset global loads feeding straight-line compare/test plus
  early-return behavior.
- Progress must be explained by shared emitter or handoff capability, not by
  one testcase win.

## Execution Rules

- Name one same-module global family before widening codegen support.
- Keep out-of-scope neighboring failures explicit in `todo.md` instead of
  silently broadening the route.
- Use backend notes and handoff tests to describe the supported boundary by
  family.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family c-testsuite probes, then checkpoint `x86_backend` only once a
  coherent cluster moves together.

## Step 1. Name The First Same-Module Global Lane

Goal: Choose one dominant same-module defined-global family and its proving
cluster before x86 codegen changes widen.

Primary targets:
- failing `x86_backend` c-testsuite cases that already lower honestly
- current x86 handoff tests that define the unsupported global-backed boundary

Actions:
- inspect the current prepared-module global rejection and group probes by one
  fixed-offset defined-global read plus equality-guard shape
- select a small proving cluster that should move together if the global lane
  is widened honestly
- record nearby out-of-scope shapes that still require bootstrap scalar
  globals, pointer-indirect global addressing, or multi-block control flow

Completion check:
- one same-module defined-global lane and one same-family proving cluster are
  named without drifting into testcase-by-testcase repair

## Step 2. Extend The Same-Module Global Lane Honestly

Goal: Teach the x86 prepared-module path to accept the chosen straight-line
same-module defined-global family.

Primary targets:
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/globals.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/globals.cpp)

Actions:
- implement the smallest shared prepared-handoff/codegen widening needed for
  fixed-offset reads from defined globals feeding equality-immediate guards and
  early returns
- keep pointer-indirect global addressing, bootstrap scalar-global setup, and
  broader relocation families unsupported when they are not required by the
  chosen lane
- do not add testcase-named shortcuts or rendered-text recognizers

Completion check:
- the chosen same-module global lane is accepted through shared x86
  prepared-module logic without testcase-shaped recognition

## Step 3. Keep The Boundary Truthful

Goal: Describe the supported same-module global family and the remaining nearby
unsupported boundary explicitly.

Primary targets:
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- revise handoff coverage so the new global lane is described by family rather
  than testcase name
- keep nearby unsupported notes or handoff expectations explicit for bootstrap
  scalar globals, pointer-indirect global addressing, and multi-block
  control-flow families
- ensure the tests still prove prepared-module honesty rather than legacy
  adapter growth

Completion check:
- backend notes and handoff tests prove the new same-module global lane is
  supported and the nearby unsupported boundary remains explicit

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
