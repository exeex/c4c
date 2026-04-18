# X86 Backend C-Testsuite Capability Families Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Resume the umbrella x86 backend capability-family route now that the
`lir_to_bir_memory.cpp` ownership split is complete and closed as its own
refactor idea.

## Goal

Pick one honest x86 backend capability family, prove it across a bounded case
cluster, and improve the backend-owned x86 surface without testcase overfit or
fallback-to-IR relaxation.

## Core Rule

This runbook is capability work, not another refactor packet. Do not hide
prepared-module renderer cleanup or unrelated structural churn inside the first
family packet. If idea `56` turns out to be a real blocker, stop and route that
through lifecycle instead of silently expanding this plan.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [ideas/closed/55_lir_to_bir_memory_semantic_ownership_split.md](/workspaces/c4c/ideas/closed/55_lir_to_bir_memory_semantic_ownership_split.md)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- `tests/c/external/c-testsuite/src/*.c` only as probe inputs for family selection

## Current Targets

- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- bounded `tests/c/external/c-testsuite/src/*.c` cases from one shared family

## Non-Goals

- re-opening the closed memory ownership split from idea `55`
- silently absorbing idea `56` into this runbook
- weakening `x86_backend` expectations to tolerate fallback LLVM IR
- single-testcase shortcuts that leave the same family unsupported nearby
- claiming broad backend readiness from one bounded packet

## Working Model

- Idea `54` is the umbrella source idea for real x86 backend capability-family
  convergence.
- Idea `55` is complete, so the next packet must re-baseline the active fail
  surface instead of assuming the earlier local-memory proving cluster is still
  the right first move.
- Idea `56` stays a parallel open idea unless the currently selected capability
  packet proves the prepared-module renderer boundary is the immediate blocker.
- The first coherent packet should name one dominant capability family and
  prove it across a small related case cluster plus the backend notes and x86
  handoff tests.

## Execution Rules

- Prefer semantic lowering or prepared-module boundary repair over testcase
  recognition.
- Keep proof honest: build first, then narrow backend or x86 proofs, then the
  broader `x86_backend` checkpoint when a coherent slice lands.
- If the chosen family touches shared lowering outside the bounded lane,
  escalate validation before acceptance.
- If renderer de-headerization becomes required, stop and request lifecycle
  repair rather than mixing idea `56` into a capability packet.

## Step 1. Re-Baseline And Choose The First Family

Goal: Refresh the current x86 backend fail surface after the completed memory
split and choose one dominant family for the first capability packet.

Primary targets:
- `tests/c/external/c-testsuite/src/*.c`
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

Actions:
- inspect the current `x86_backend` or equivalent bounded fail surface
- cluster the visible failures by shared backend mechanism
- pick one first family and name a narrow proving set from that cluster
- decide whether idea `56` remains parallel for this packet or is a real blocker

Completion check:
- the next packet has one named capability family, one bounded proving cluster,
  and an explicit judgment on whether idea `56` stays out of scope

## Step 2. Lock The Family Boundary In Backend Tests

Goal: Make the supported versus unsupported line visible in backend-owned proof
surfaces before widening implementation.

Primary targets:
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

Actions:
- update or add the narrowest backend notes and x86 handoff assertions needed
  for the chosen family
- describe the lane by capability family rather than by testcase name
- keep existing unsupported boundaries truthful where support has not landed yet

Completion check:
- backend notes and handoff proofs describe the chosen family clearly enough to
  keep later packets honest

## Step 3. Implement One Honest Capability Lane

Goal: Land one bounded semantic-lowering or prepared-module boundary repair for
the chosen family.

Primary targets:
- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- adjacent prepared-module ingestion files only if the family truly requires them

Actions:
- implement the smallest truthful capability repair that explains the chosen
  family
- keep the route in semantic lowering and honest prepared-module ingestion
- check nearby same-family cases so the change is not just one testcase-shaped fix

Completion check:
- one bounded family lane is supported honestly across more than one nearby case

## Step 4. Prove The Packet Narrowly

Goal: Validate the chosen lane on the narrow backend and x86 proofs that
directly cover the packet.

Actions:
- run `cmake --build --preset default`
- run the backend notes and x86 handoff proofs relevant to the chosen family
- run the targeted c-testsuite cases chosen in Step 1

Completion check:
- the narrow proving set passes and still describes the lane truthfully

## Step 5. Checkpoint The X86 Backend Surface

Goal: Confirm the packet moves the backend-owned x86 surface honestly and
record whether more work remains in the same family.

Actions:
- run `ctest --test-dir build -L x86_backend --output-on-failure`
- compare the pass/fail surface against the pre-packet baseline
- record the family-level result in `todo.md`
- if the packet reveals a separate blocker initiative, route it into
  `ideas/open/` instead of stretching this runbook

Completion check:
- the packet can be described as real family progress, or it stops with an
  explicit blocker and truthful boundary notes instead of pretending success
