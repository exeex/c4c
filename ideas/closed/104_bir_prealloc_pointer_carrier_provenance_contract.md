# BIR Prealloc Pointer Carrier Provenance Contract

## Goal

Clarify the authority boundary for pointer carrier provenance so prealloc
regalloc preparation consumes explicit BIR/prepared facts instead of
reconstructing target-neutral pointer relations from instruction shape.

## Why This Exists

The BIR/prealloc memory boundary audit found the strongest concrete overlap in
`src/backend/prealloc/regalloc/pointer_carriers.cpp`. Prepared pointer-value
memory accesses are correct prealloc consumers of `MemoryAddress::PointerValue`,
but the carrier map also derives local-slot pointer state from
`LoadLocalInst`/`StoreLocalInst` without `MemoryAddress`, copies
`pointer_symbol_link_name_id` directly from BIR instruction results, and
infers predecessor/successor pointer carrier states around addressed stores
from recent load/store order, step bytes, and byte deltas.

Those relations look target-neutral when they identify "this pointer value is
the same base plus this offset" or "this pointer value denotes this symbol."
Prealloc may need transient carrier metadata for regalloc, but it should not
become the durable semantic authority for pointer provenance.

## In Scope

- Trace every pointer-carrier seed and propagation path in
  `src/backend/prealloc/regalloc/pointer_carriers.cpp`.
- Decide which carrier fields are transient regalloc convenience and which
  require an explicit target-neutral BIR or prepared-addressing fact.
- Prefer consuming `MemoryAddress::PointerValue`, structured symbol identity,
  or a new narrow prepared fact over re-walking local load/store shape.
- Add focused proof for local-slot pointer storage, pointer-symbol carrier
  seeding, and pointer plus/minus carrier inference.

## Out Of Scope

- Moving register allocation, storage homes, spill decisions, or frame-slot
  placement into BIR.
- Rewriting unrelated prepared lookup, stack layout, or call ABI contracts.
- Broad pointer analysis beyond the carrier facts needed by prealloc regalloc
  preparation.
- Weakening tests or accepting named-case shortcuts as progress.

## Acceptance Criteria

- Each pointer-carrier provenance route is classified as either retained
  transient regalloc metadata or converted to consume an explicit BIR/prepared
  target-neutral fact.
- Local-slot pointer propagation no longer relies on raw slot spelling and
  nearby load/store shape as semantic authority when a structured memory fact
  should exist.
- Pointer-symbol carrier seeding has a structured authority contract and does
  not silently duplicate global/link-name provenance.
- Pointer plus/minus carrier derivation is either proven to be regalloc-only
  convenience or backed by a target-neutral carrier relation.

## Reviewer Reject Signals

- The patch only renames pointer-carrier helpers while preserving the same
  instruction-shape reconstruction as the semantic authority.
- It adds another special case for one local slot, one symbol, or one
  plus/minus pattern instead of defining the carrier contract.
- It moves stack placement, register homes, or target addressing-mode choices
  into BIR.
- Proof covers only a single named testcase while local-slot storage,
  pointer-symbol, and plus/minus carrier routes remain unexamined.
- Existing failures are hidden by expectation downgrades, unsupported markings,
  or weaker contracts.

## Close Note

Closed on 2026-06-03.

The source idea is complete. Pointer-carrier provenance routes are classified
and implemented as either retained transient prealloc metadata or explicit
semantic authority: prepared pointer-value access facts, BIR pointer-symbol
link-name facts, and explicit BIR pointer add/sub immediate relations from an
authorized base.

Local-slot propagation no longer relies on raw slot spelling or nearby
load/store order to mint semantic pointer provenance. It may only preserve an
already-authorized carrier unchanged through no-address local slots. Pointer
symbol carrier seeding is authorized by valid BIR `pointer_symbol_link_name_id`
metadata, and missing or invalid link-name metadata fails closed.

Generic pointer plus/minus derivation from recent load/store order now fails
closed. Computed pointer call-argument source shape is retained only through an
explicit BIR pointer-typed add/sub immediate relation from an already
authorized base carrier, not through local-slot/order inference. Register
homes, stack homes, call-argument moves, and target placement remain
prealloc/MIR physical facts rather than provenance authority.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Coverage includes local-slot
preservation and fail-closed behavior, pointer-symbol seeding and fail-closed
behavior, plus/minus fail-closed behavior, and explicit computed-pointer
authority through call-plan and prepared-printer contracts.
