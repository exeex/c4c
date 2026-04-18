# LIR To BIR Memory Semantic Ownership Split Runbook

Status: Active
Source Idea: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md
Activated from: ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md

## Purpose

`src/backend/bir/lir_to_bir_memory.cpp` has become the upstream ownership
hotspot for continued backend local-memory work. Activate a narrow refactor
runbook that splits the monolith by semantic ownership before more capability
packets widen the same translation unit.

## Goal

Extract `addressing/layout` and `pointer provenance` helper families into their
own translation units while keeping `lower_scalar_or_local_memory_inst` as the
main coordinator in `src/backend/bir/lir_to_bir_memory.cpp`.

## Core Rule

This runbook is a behavior-preserving refactor. Do not claim semantic repair,
new backend capability, or x86 c-testsuite improvement from the split itself.
Split by semantic ownership, not by testcase shape, `local` versus `global`,
or arbitrary line counts.

## Read First

- [ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md](/workspaces/c4c/ideas/open/55_lir_to_bir_memory_semantic_ownership_split.md)
- [review/lir_to_bir_memory_split_review.md](/workspaces/c4c/review/lir_to_bir_memory_split_review.md)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- adjacent `src/backend/bir/lir_to_bir_*.cpp` files for existing ownership shape

## Current Targets

- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- new `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- new `src/backend/bir/lir_to_bir_memory_provenance.cpp`

## Non-Goals

- landing new x86 backend capability-family support
- de-headerizing `src/backend/mir/x86/codegen/x86_codegen.hpp`
- forcing a `local` versus `global` split
- hiding semantic changes inside the extract packet
- AArch64 cleanup or parity work

## Working Model

- `src/backend/bir/lir_to_bir_memory.cpp` currently mixes three ownership
  domains: shared address/layout walking, pointer provenance, and opcode
  lowering coordination.
- The correct first split is:
  - coordinator remains in `lir_to_bir_memory.cpp`
  - address/layout helpers move to `lir_to_bir_memory_addressing.cpp`
  - provenance and addressed-object helpers move to
    `lir_to_bir_memory_provenance.cpp`
- When ownership is ambiguous, keep the helper in
  `lir_to_bir_memory.cpp` for the first packet rather than forcing a premature
  micro-split.

## Execution Rules

- Keep public lowering contract updates reviewable in
  [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp).
- Prefer pure move/extract edits in the first packet; preserve existing error
  behavior and lowering admission.
- If a helper's ownership is unclear, bias toward leaving it in the
  coordinator TU for this runbook.
- Validation ladder per coherent slice: build, current narrow backend proof
  surface, then broaden if the extraction leaks beyond the intended seam.
- Do not mix x86 renderer refactors or new c-testsuite capability work into
  this runbook.

## Step 1. Confirm Ownership Boundaries

Goal: Turn the source idea and reviewer guidance into an exact extraction map.

Primary targets:
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)

Actions:
- inspect the existing helper declarations and implementation clusters
- mark which helpers are clearly `addressing/layout`
- mark which helpers are clearly `pointer provenance`
- leave ambiguous helpers in the coordinator bucket for the first packet

Completion check:
- one concrete first-pass extraction map exists for the two new translation
  units, and the retained coordinator surface is explicit

## Step 2. Extract Addressing/Layout Ownership

Goal: Move shared address/layout helpers into
`lir_to_bir_memory_addressing.cpp`.

Primary targets:
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)

Actions:
- add `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- move shared aggregate walking, byte-offset resolution, and related layout
  helpers that are clearly ownership-aligned there
- keep declarations and includes coherent without changing behavior

Completion check:
- address/layout helpers live in the new TU and the build still treats the
  split as behavior-preserving

## Step 3. Extract Pointer Provenance Ownership

Goal: Move provenance and addressed-object helpers into
`lir_to_bir_memory_provenance.cpp`.

Primary targets:
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)

Actions:
- add `src/backend/bir/lir_to_bir_memory_provenance.cpp`
- move pointer/global alias, addressed-object, and pointer-base recovery
  helpers that clearly belong to provenance ownership
- keep cross-TU access explicit through the existing lowerer contract

Completion check:
- provenance helpers live in the new TU and no semantic change is claimed

## Step 4. Re-Center The Coordinator

Goal: Leave `lir_to_bir_memory.cpp` as the readable opcode-lowering
coordinator.

Primary targets:
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)

Actions:
- trim leftover helper clutter that no longer belongs in the coordinator
- keep `lower_scalar_or_local_memory_inst` as the main admission surface
- ensure the remaining glue helpers are only the ones best read next to the
  coordinator

Completion check:
- `lir_to_bir_memory.cpp` reads as the opcode-lowering coordinator rather than
  a mixed-ownership monolith

## Step 5. Prove The Split As Refactor-Only

Goal: Validate that the extraction preserved behavior on the active backend
proof surface.

Actions:
- run `cmake --build --preset default`
- run the narrow backend proof surface active when implementation begins
- broaden proof if the diff touches shared lowering paths beyond the intended
  seams

Completion check:
- build plus the chosen backend proof surface pass, and the packet can be
  described honestly as a refactor slice rather than a capability claim
