# BIR Prealloc Pointer Carrier Provenance Contract Runbook

Status: Active
Source Idea: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md

## Purpose

Clarify the authority boundary for pointer carrier provenance so prealloc
regalloc preparation consumes explicit BIR or prepared facts instead of
reconstructing target-neutral pointer relations from instruction shape.

## Goal

Classify and repair the pointer-carrier provenance routes used by prealloc
regalloc preparation for local-slot pointer storage, pointer-symbol seeding,
and pointer plus/minus carrier inference.

## Core Rule

Do not turn prealloc carrier convenience into durable semantic authority.
Retain prealloc ownership of regalloc metadata, storage homes, and target
placement, but require target-neutral pointer identity and provenance facts to
come from structured BIR/prepared authority when they describe source-level or
prepared-addressing relations.

## Read First

- `ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md`
- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- Prepared memory-address and pointer-value publication surfaces
- Existing backend tests covering local pointer storage, global/link-name
  pointer carriers, and pointer arithmetic carrier propagation

## Current Targets

- Every pointer-carrier seed and propagation path in
  `src/backend/prealloc/regalloc/pointer_carriers.cpp`.
- Local-slot pointer propagation currently derived from load/store instruction
  shape without `MemoryAddress`.
- Pointer-symbol carrier seeding through structured symbol/link-name identity.
- Pointer plus/minus carrier derivation around addressed stores.

## Non-Goals

- Do not move register allocation, storage homes, spill decisions, or frame-slot
  placement into BIR.
- Do not rewrite unrelated prepared lookup, stack layout, or call ABI
  contracts.
- Do not attempt broad pointer analysis beyond carrier facts needed by prealloc
  regalloc preparation.
- Do not weaken tests, mark supported paths unsupported, or add named-case
  shortcuts.

## Working Model

Prepared pointer-value memory accesses are legitimate prealloc consumers of
`MemoryAddress::PointerValue`. The audit concern is that the carrier map also
derives target-neutral-looking provenance from raw local load/store shape,
copies symbol identity directly from BIR instruction results, and infers
predecessor/successor carrier relations from recent memory order and byte
deltas. The route must decide which facts are only transient regalloc
convenience and which need explicit BIR/prepared authority.

## Execution Rules

- Keep inventory and classification notes in `todo.md`.
- Prefer consuming `MemoryAddress::PointerValue`, structured symbol identity,
  or a narrow prepared fact over re-walking local load/store shape.
- If a path is retained as transient regalloc metadata, name the boundary and
  prove it cannot become semantic pointer provenance authority.
- If a path describes target-neutral provenance, move consumption to an
  explicit BIR/prepared fact rather than adding another shape special case.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation if shared prepared memory, pointer carrier, or regalloc
  interfaces change.

## Step 1: Inventory Pointer Carrier Seeds And Propagation

Goal: trace each pointer-carrier provenance route and classify its current
producer/consumer surface.

Primary targets:

- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- Prepared memory-address publication and lookup code
- Existing backend pointer-carrier tests

Actions:

- Trace carrier seeds from `MemoryAddress::PointerValue`.
- Trace local-slot pointer propagation from `LoadLocalInst` and
  `StoreLocalInst` without `MemoryAddress`.
- Trace `pointer_symbol_link_name_id` seeding and any direct BIR result-copying.
- Trace predecessor/successor plus/minus carrier inference around addressed
  stores, step bytes, and byte deltas.
- Separate transient regalloc convenience from target-neutral pointer
  provenance facts.
- Record candidate contract gaps and proof gaps in `todo.md`.

Completion check:

- `todo.md` lists every owned seed/propagation route and its current authority.
- Analysis proof confirms no implementation diff under `src/backend/bir` or
  `src/backend/prealloc`.

## Step 2: Decide The Provenance Authority Contract

Goal: decide which pointer-carrier fields remain transient prealloc metadata
and which require explicit BIR/prepared target-neutral facts.

Actions:

- Compare Step 1 findings against the source idea acceptance criteria.
- Decide the contract for local-slot pointer storage propagation.
- Decide the contract for pointer-symbol carrier seeding.
- Decide the contract for pointer plus/minus carrier derivation.
- Identify focused proof cases for all three routes.
- Reject routes that preserve instruction-shape reconstruction as semantic
  authority under new helper names.

Completion check:

- `todo.md` records the authority decision, implementation targets, retained
  transient paths if any, and proof targets.
- Analysis proof confirms no implementation diff unless this packet
  intentionally begins implementation.

## Step 3: Implement The Carrier Provenance Contract

Goal: make pointer-carrier consumers use explicit authority or named transient
metadata according to the Step 2 contract.

Actions:

- Update pointer-carrier seeding to consume structured memory or prepared facts
  where target-neutral provenance is required.
- Constrain or remove local-slot load/store shape reconstruction when a
  structured memory fact should exist.
- Give pointer-symbol carrier seeding a structured authority contract and avoid
  silent duplication of global/link-name provenance.
- Either constrain plus/minus derivation to regalloc-only convenience or back
  it with an explicit target-neutral carrier relation.
- Preserve prealloc ownership of register allocation and target placement.

Completion check:

- The default build passes.
- The diff does not add special cases for one local slot, one symbol, or one
  plus/minus pattern.
- The changed routes are reviewable as explicit provenance authority or named
  transient regalloc metadata.

## Step 4: Add Focused Provenance Proof

Goal: prove local-slot pointer storage, pointer-symbol seeding, and pointer
plus/minus carrier behavior against the chosen contract.

Actions:

- Add or strengthen tests for local-slot pointer storage propagation.
- Add or strengthen tests for pointer-symbol carrier seeding through structured
  identity.
- Add or strengthen tests for pointer plus/minus carrier inference or retained
  regalloc-only behavior.
- Prefer prepared-plan, carrier-map, or contract assertions over fragile
  assembly-only symptoms when possible.

Completion check:

- The default build passes.
- Focused backend pointer-carrier tests pass.
- Tests cover all three source acceptance routes and do not weaken existing
  expectations.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant backend pointer-carrier/prepared-memory
  subset.
- Include broader `^backend_` validation if shared prepared memory, pointer
  carrier, or regalloc interfaces changed.
- Update `todo.md` with final proof, retained transient metadata details, and
  close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows coverage for local-slot pointer storage, pointer-symbol
  carrier seeding, and pointer plus/minus carrier behavior.
- The source idea acceptance criteria are satisfied without unrelated prepared
  lookup, stack layout, call ABI, or broad pointer-analysis work.
