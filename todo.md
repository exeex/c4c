Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Provenance Authority Contract

# Current Packet

## Just Finished

Step 2 authority contract decision completed without implementation or test
edits.

Chosen contract:

- Semantic pointer-carrier provenance authority is explicit BIR/prepared data
  only. A value home may publish `PointerBasePlusOffset`,
  `pointer_base_value_name`, `pointer_base_symbol_name`, or
  `pointer_byte_delta` only when the carrier came from a named provenance
  authority, not from raw instruction shape.
- `PreparedAddressBaseKind::PointerValue` in prepared memory accesses remains
  the authority for pointer-value memory base identity and access step size.
  Prealloc may cache its `pointer_value_name` and minimum `size_bytes` as a
  self carrier, but that cache is a prepared-memory-address projection.
- BIR `Value::pointer_symbol_link_name_id` remains the authority for symbol
  identity. Pointer-symbol carrier seeding should be wrapped in a named helper
  that validates/imports structured BIR link-name provenance before attaching
  `base_symbol_name`; it must not be presented as a generic carrier inference.
- Local-slot pointer storage propagation without `MemoryAddress` is not
  semantic authority. It may remain only as transient prealloc-local
  bookkeeping to carry an already-authorized carrier through a raw local slot.
  It must not mint a new `PointerBasePlusOffset` relation from slot spelling or
  load/store adjacency.
- Pointer plus/minus carrier derivation from predecessor/successor instruction
  order is not semantic authority. New plus/minus base+delta publication must
  come from an explicit BIR/prepared carrier relation. Until such a fact exists,
  the old order-derived `+step_bytes` and `-step_bytes` routes should fail
  closed for semantic publication rather than being renamed.
- Retained transient prealloc-only paths are limited to: self carriers projected
  from prepared pointer-value memory accesses, symbol self carriers projected
  from BIR link-name metadata, and slot bookkeeping that preserves an existing
  authorized carrier without changing base, symbol, delta, or step. Physical
  register/stack homes and target placement remain prealloc/MIR facts.

Rejected routes:

- Do not reconstruct target-neutral pointer provenance from
  `LoadLocalInst`/`StoreLocalInst` shape, raw slot names, or "last loaded"
  instruction order under new helper names.
- Do not treat `MemoryAddress::PointerValue` merely being present on a later
  store as proof that a stored pointer value is the previous or next pointer.
- Do not move register allocation, frame-slot placement, spill homes, or target
  addressing-mode decisions into BIR.

Implementation targets for Step 3:

- Split `pointer_carriers.cpp` helpers into named authority/projector helpers
  for prepared pointer-value access facts and BIR pointer-symbol facts, plus a
  visibly transient slot-preservation helper.
- Constrain `slot_pointer_carriers` so it only propagates already-authorized
  carriers unchanged through no-address local slots; remove or fail closed the
  same-slot `+step_bytes` minting path for unresolved stored values.
- Remove or fail closed the addressed local/global store `-step_bytes`
  predecessor inference unless an explicit BIR/prepared carrier relation is
  added by the implementation packet.
- Keep `classify_prepared_value_home` publishing
  `PointerBasePlusOffset` only from carriers tagged or otherwise proven to come
  from an allowed authority path.
- Preserve existing physical consumers: decoded home storage, publication
  plans, call plans, prepared printers, liveness, and MIR target memory operand
  planning should consume prepared facts, not become provenance authorities.

Focused proof targets:

- Local-slot pointer storage: a no-address local slot may preserve an existing
  authorized pointer-symbol or prepared-pointer-value carrier unchanged, but a
  same-slot load/store adjacency must not create a new base+offset home.
- Pointer-symbol seeding: a named pointer result with valid
  `pointer_symbol_link_name_id` produces the expected symbol-backed carrier
  publication, and an invalid/missing link name fails closed.
- Pointer plus/minus derivation: old predecessor/successor `+step_bytes` and
  `-step_bytes` inference does not publish `PointerBasePlusOffset` without an
  explicit carrier relation; if Step 3 adds such a relation, tests must assert
  the relation field rather than the instruction order pattern.

## Suggested Next

Proceed to Step 3 and implement the contract in the narrow carrier map surface:
named authority helpers, unchanged transient slot preservation, and fail-closed
behavior for instruction-shape plus/minus provenance.

## Watchouts

- Do not treat load/store instruction shape as target-neutral provenance
  authority without a structured fact.
- Keep register allocation, storage homes, spill decisions, and target
  placement in prealloc.
- Separate transient regalloc carrier metadata from durable BIR/prepared
  pointer provenance.
- Avoid adding slot-name parsing, testcase-shaped pointer pairing, or a contract
  that makes same-slot load/store order semantic authority.
- `base_symbol_name` currently only rides on carrier state; make sure Step 2
  distinguishes BIR symbol provenance from physical indirect-call or
  publication-plan movement.
- Step 3 should prefer fail-closed behavior over preserving the current
  plus/minus relation by renaming helpers.
- If preserving any carrier through local slots, prove the carrier is unchanged
  and already authorized before it reaches `classify_prepared_value_home`.

## Proof

Passed:
`git diff --quiet -- src/backend/bir src/backend/prealloc src/backend/mir tests && printf 'analysis-only proof: no implementation or test diff for pointer carrier provenance contract decision\n' > test_after.log`

Proof log: `test_after.log`

Result:
`analysis-only proof: no implementation or test diff for pointer carrier provenance contract decision`
