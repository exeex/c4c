# X86 Backend C-Testsuite Capability Families Runbook

Status: Active
Source Idea: ideas/open/57_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/57_x86_backend_c_testsuite_capability_families.md

## Purpose

Turn the current x86 backend c-testsuite fail surface into honest capability
family work packets instead of testcase-shaped repairs.

## Goal

Choose one dominant x86 backend capability family, land one bounded lane in
that family through semantic `lir_to_bir` into the prepared-module handoff,
and prove the result across nearby cases without weakening `x86_backend`
contracts.

## Core Rule

This route is capability-family work only. Do not claim progress through
fallback-to-IR acceptance, testcase-named shortcuts, or expectation downgrades.

## Read First

- [ideas/open/57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

## Current Targets

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- targeted `tests/c/external/c-testsuite/src/*.c` probes from one family

## Non-Goals

- solving the whole x86 backend c-testsuite surface in one packet
- reviving legacy adapter-growth as the main mechanism
- mixing AArch64 cleanup or unrelated emitter restructuring into this route
- weakening expected-output contracts or converting supported paths to
  unsupported

## Working Model

- Treat the fail surface as a small number of capability families, not a pile
  of isolated testcase names.
- Prefer semantic lowering growth in `lir_to_bir_*`; use x86 handoff tests to
  prove prepared-module ingestion boundaries honestly.
- Nearby cases in the same lane must be checked so the packet does not overfit
  one named failing testcase.
- Route-reset constraints remain active: if the next honest packet still needs
  an ownership split in `lir_to_bir_memory.cpp`, stop and hand that back
  through lifecycle instead of stretching this runbook.

## Execution Rules

- Start by re-baselining the current dominant family from current tests, not
  from stale packet assumptions.
- Name packets by capability family and lane, not by testcase ID.
- Keep proving shaped as `build -> backend notes/handoff tests -> targeted
  c-testsuite cluster -> x86_backend checkpoint when justified`.
- Escalate to broader backend validation when shared lowering or prepared-module
  legality changes beyond the bounded lane.
- Reject any slice whose main effect is expectation churn or matcher growth
  instead of backend capability repair.

## Step 1. Re-Baseline The Active Family

Goal: Choose the first honest capability-family packet from the current x86
backend failure surface.

Primary targets:
- `tests/c/external/c-testsuite/src/*.c`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Actions:
- inspect the current x86 backend failure notes and handoff boundaries
- group the immediate failing probes by one dominant capability family
- select one bounded lane and a nearby-case proving cluster for that family

Completion check:
- the next packet names one capability family and one bounded lane with a
  concrete proving cluster

## Step 2. Repair One Bounded Capability Lane

Goal: Land one honest backend improvement for the selected family.

Primary targets:
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir.hpp`
- any prepared-module boundary surface required by the same lane

Actions:
- implement the smallest semantic lowering or legality change that repairs the
  chosen lane
- keep the packet aligned to the selected capability family instead of one test
  name
- update backend-owned notes or boundary checks to describe the new supported
  edge honestly

Completion check:
- one bounded lane from the chosen family lowers honestly through the prepared
  x86 handoff without expectation weakening

## Step 3. Prove The Lane And Nearby Cases

Goal: Show the packet improves a family lane rather than one named case.

Actions:
- run `cmake --build --preset default`
- run `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary(_test)?)$'`
- run the targeted `c_testsuite_x86_backend_*` cluster chosen in Step 1
- capture the exact proving commands and results in `todo.md`

Completion check:
- backend notes, x86 handoff proof, and the targeted nearby-case cluster all
  pass for the same lane

## Step 4. Re-Checkpoint The X86 Backend Surface

Goal: Decide whether the route should continue on the same family, switch
family, or return through lifecycle because a prerequisite refactor surfaced.

Actions:
- compare the landed lane against nearby remaining failures
- run `ctest --test-dir build -L x86_backend --output-on-failure` when the
  slice is coherent enough for a checkpoint
- record the next honest family packet or blocker in `todo.md`

Completion check:
- the next packet is described by a capability family and blocker/proof state,
  not by isolated testcase names
