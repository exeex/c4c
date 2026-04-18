# LIR To BIR Memory Semantic Ownership Split Runbook

Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Supersedes: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md

## Purpose

Switch the active route from x86 capability-family execution to the reopened
memory-ownership prerequisite so `lir_to_bir_memory.cpp` can become a real
coordinator again before more backend lane work widens the monolith.

## Goal

Land behavior-preserving second-layer ownership splits for
`lir_to_bir_memory.cpp` while keeping semantic lowering contracts and proof
surfaces honest.

## Core Rule

This runbook is refactor-only. Do not claim backend capability growth, change
admission behavior, or smuggle x86 family progress into these packets.

## Read First

- [ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md](/workspaces/c4c/ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md)
- [review/lir_to_bir_memory_split_review.md](/workspaces/c4c/review/lir_to_bir_memory_split_review.md)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir_memory_addressing.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_addressing.cpp)
- [src/backend/bir/lir_to_bir_memory_provenance.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_provenance.cpp)

## Current Targets

- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory_addressing.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_addressing.cpp)
- [src/backend/bir/lir_to_bir_memory_provenance.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_provenance.cpp)
- likely follow-on files under `src/backend/bir/` such as
  `lir_to_bir_memory_value_materialization.cpp` and
  `lir_to_bir_memory_local_slots.cpp`
- backend-owned proof surfaces already active around this route

## Non-Goals

- claiming progress on the x86 c-testsuite capability-family route
- de-headerizing `src/backend/mir/x86/codegen/x86_codegen.hpp`
- splitting by testcase, packet nickname, or equal line counts
- forcing a fake `local` versus `global` file divide
- moving `lower_scalar_or_local_memory_inst` out of the main coordinator TU

## Working Model

- `lir_to_bir_memory.cpp` should remain the opcode-lowering coordinator.
- `lir_to_bir_memory_addressing.cpp` owns shared layout and address-walking
  helpers.
- `lir_to_bir_memory_provenance.cpp` owns pointer provenance and addressed
  object reasoning.
- The reopened route adds explicit homes for value materialization and
  local-slot or local-aggregate mechanics when read-through confirms those
  seams.
- Each packet must be understandable as move/extract work first, with behavior
  preserved and proof run afterward.

## Execution Rules

- Prefer semantic ownership seams over convenience slices.
- Keep new declarations and definitions coherent across `lir_to_bir.hpp` and
  the extracted `.cpp` owners.
- If a helper is ambiguous, choose the strongest semantic owner rather than
  leaving it in `lir_to_bir_memory.cpp` by default.
- Validate every packet as `build -> narrow backend proof`, and broaden proof
  if the extraction reaches beyond the immediate seam.
- If execution uncovers a separate initiative, stop and route it through
  lifecycle instead of stretching this refactor plan.

## Step 1. Re-Read And Confirm Ownership Buckets

Goal: Reconfirm the next extraction seam before moving code.

Primary targets:
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir_memory_addressing.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_addressing.cpp)
- [src/backend/bir/lir_to_bir_memory_provenance.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory_provenance.cpp)

Actions:
- inspect the remaining helpers still owned by `lir_to_bir_memory.cpp`
- classify them as coordinator glue, value materialization, local-slot or
  local-aggregate mechanics, or remaining provenance/addressing work
- choose one extraction packet whose ownership is coherent and behavior
  preserving
- name the narrow backend proof surface that stays active for the packet

Completion check:
- the next packet has one explicit ownership seam, owned files are clear, and
  the proof command is chosen before edits begin

## Step 2. Extract One Second-Layer Ownership Bucket

Goal: Move one coherent helper family out of the coordinator TU.

Primary targets:
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- one extracted owner file under `src/backend/bir/`

Actions:
- move one coherent helper cluster into its semantic owner
- keep `lower_scalar_or_local_memory_inst` as the main coordinator
- preserve signatures and call flow clarity across the split
- keep the packet free of claimed lowering or backend-capability behavior
  changes

Completion check:
- one helper family now lives under a clearer owner and the main coordinator TU
  shrinks without changing the route's behavior contract

## Step 3. Prove The Refactor Packet Narrowly

Goal: Show the extraction is behavior preserving on the active backend proof
surface.

Actions:
- run `cmake --build --preset default`
- run the chosen narrow backend proof command for this packet
- capture the exact proving command and result in `todo.md`

Completion check:
- the build passes, the narrow proof passes, and the packet is recorded as
  refactor-only progress

## Step 4. Check Coordinator Health

Goal: Decide whether the coordinator still carries the next obvious ownership
bucket or whether activation should return to the x86 umbrella later.

Actions:
- compare the remaining helper mix in `lir_to_bir_memory.cpp` against the idea
  55 target seams
- record the next honest extraction candidate in `todo.md`
- if the reopened split is no longer the active blocker, leave a clear handoff
  for the next lifecycle checkpoint

Completion check:
- the next packet is described by ownership, not by a testcase or temporary
  lane name
