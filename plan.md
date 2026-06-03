# BIR Prealloc Memory Pointer Storage Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/98_bir_prealloc_memory_pointer_storage_boundary_audit.md

## Purpose

Audit the boundary between BIR memory/provenance facts and prealloc stack
layout, addressing, storage, and pointer-carrier preparation before x86 and
RISC-V backend rebuild work starts.

## Goal

Classify memory, pointer, stack-storage, and prepared-addressing authority
overlaps between BIR and prealloc, then create follow-up ideas only for
concrete duplicated authority or unclear contracts.

## Core Rule

This plan is audit-only. Do not edit implementation files, change stack layout
or addressing behavior, rewrite expectations, or claim backend capability
progress from classification alone.

## Read First

- `ideas/open/98_bir_prealloc_memory_pointer_storage_boundary_audit.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/memory/*.cpp`
- `src/backend/bir/lir_to_bir/memory/*.hpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/prealloc/stack_layout/*.cpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/storage_plans.cpp`
- `src/backend/prealloc/decoded_home_storage.*`
- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- `src/backend/prealloc/prepared_lookups.*`

## Scope

- BIR memory/address/provenance facts for local slots, globals, pointer
  values, GEP-derived addresses, dynamic stack access, and memory operations.
- Prealloc stack-layout, storage-home, decoded-home, pointer-carrier,
  prepared-address, and prepared-lookup facts.
- Follow-up idea creation only when an overlap is concrete, owned, and
  proofable.

## Non-Goals

- Do not implement memory, stack-layout, storage, or pointer-carrier cleanup in
  this plan.
- Do not move target stack placement, register/storage homes, or frame offset
  assignment into BIR.
- Do not reopen AArch64 memory owner cleanup routes.
- Do not treat every prealloc scan over BIR instructions as duplicated
  authority without tracing the fact being reconstructed.
- Do not create arch-specific x86/RISC-V implementation work before shared
  contract gaps are named.

## Working Model

Classify each traced overlap with one or more of:

- `bir-semantic-fact`
- `prealloc-placement-authority`
- `prealloc-rederives-bir-provenance`
- `bir-missing-target-neutral-fact`
- `contract-ambiguous`
- `needs-follow-up-idea`

Use `todo.md` for packet notes, traced functions, classifications, and proof
results. Only create or change source ideas when the audit produces durable
follow-up intent.

## Execution Rules

- Keep every classification tied to concrete BIR producers, prealloc
  consumers, and the specific fact being reconstructed or consumed.
- Separate target-neutral memory/provenance identity from target-facing
  placement, stack-layout, and storage-home authority.
- Treat stack slot placement, frame offsets, register/storage homes, and
  target-facing prepared address plans as prealloc authority unless the trace
  proves re-derivation of an existing BIR semantic fact.
- Create follow-up ideas only for narrow boundaries with named files and proof
  routes.
- If implementation files are accidentally touched, stop and require fresh
  build or compile proof before accepting the slice.

## Steps

### Step 1: Inventory BIR Memory And Address Facts

Goal: identify the memory, address, provenance-like, local-slot, global, GEP,
and dynamic stack facts BIR already produces and name their intended authority.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/memory/*.cpp`
- `src/backend/bir/lir_to_bir/memory/*.hpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/globals.cpp`

Actions:

- Inspect BIR memory instructions, address carriers, local/global identity,
  GEP-derived address facts, and dynamic stack access structures.
- Trace where those facts are created from LIR or global initializer lowering.
- Record each fact in `todo.md` with its producer and target-neutral authority
  boundary.

Completion check:

- `todo.md` lists the BIR memory/address fact inventory and identifies which
  facts need prealloc-consumer tracing.

### Step 2: Inventory Prealloc Stack Layout And Storage Facts

Goal: identify where prealloc owns stack placement, storage-home planning, and
decoded-home authority while preparing memory access.

Primary targets:

- `src/backend/prealloc/stack_layout/*.cpp`
- `src/backend/prealloc/storage_plans.cpp`
- `src/backend/prealloc/decoded_home_storage.*`

Actions:

- Trace stack object layout, frame-slot assignment, storage-plan construction,
  and decoded-home preparation.
- Separate placement and storage authority from target-neutral memory
  semantics.
- Record each prealloc fact source or classifier in `todo.md`.

Completion check:

- `todo.md` names the stack-layout and storage-home authority sites and their
  initial classification.

### Step 3: Trace Prepared Addressing And Pointer Carriers

Goal: classify prepared addressing, prepared lookup, and pointer-carrier
construction against the BIR/prealloc memory authority boundary.

Primary targets:

- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- `src/backend/prealloc/prepared_lookups.*`

Actions:

- Trace prepared address and pointer-carrier inputs back to BIR facts, stack
  placement facts, or target-facing storage plans.
- Identify any duplicated local slot, global symbol, pointer plus offset,
  dynamic stack, or typed stack-source derivation.
- Record intentional prealloc preparation separately from contract ambiguity.

Completion check:

- `todo.md` classifies prepared addressing, pointer-carrier, and lookup routes
  as retained authority, duplicated provenance, missing BIR fact, ambiguous
  contract, or follow-up material.

### Step 4: Cross-Map Memory And Storage Overlaps

Goal: decide which BIR/prealloc overlaps are correct consumption, duplicated
authority, missing BIR facts, or ambiguous contracts.

Primary targets:

- the Step 1 BIR fact inventory in `todo.md`
- the Step 2 and Step 3 prealloc inventories in `todo.md`
- the source files already traced by earlier steps

Actions:

- Cross-map every scoped BIR memory/address fact to its prealloc consumers.
- For each overlap, classify it with the working-model labels.
- Name the exact producer, consumer, and fact for any
  `prealloc-rederives-bir-provenance`, `bir-missing-target-neutral-fact`, or
  `contract-ambiguous` classification.

Completion check:

- `todo.md` contains a complete overlap table and clearly separates
  target-neutral semantic facts from target-facing placement and storage
  authority.

### Step 5: Synthesize Follow-Up Ideas Or Intentional Retention

Goal: finish the audit with durable conclusions and create follow-up ideas
only where the trace proves a narrow, proofable boundary.

Primary targets:

- `todo.md`
- `ideas/open/` for any generated follow-up ideas
- `ideas/open/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` only
  if closure or durable source-intent notes are required

Actions:

- Summarize each memory, pointer, and storage overlap and final
  classification.
- For every `needs-follow-up-idea` classification, create a focused source
  idea with concrete owned files, proof expectations, and reviewer reject
  signals.
- Explicitly list overlaps judged intentional BIR semantic authority or
  intentional prealloc placement/storage authority.
- Prepare the close decision for the source idea, including generated follow-up
  idea names or an explicit statement that none were generated.

Completion check:

- The audit has classified all scoped memory/pointer/storage overlaps,
  generated only supported follow-up ideas, and left the active source idea
  ready for lifecycle close review.
