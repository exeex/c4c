Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current Opaque Pointer Provenance Routes

# Current Packet

## Just Finished

Activation created the active runbook from the source idea. No executor packet
has completed yet.

## Suggested Next

Begin `Step 1: Inventory Current Opaque Pointer Provenance Routes` by mapping
the current opaque pointer byte-offset bridge, prepared `memory_accesses`
facts, missing provenance/range facts, and focused proof subset named in
`plan.md`.

## Watchouts

- Do not remove the `allow_opaque_ptr_base && stored_type == I8`
  compatibility rule until every required route has structured range facts or
  an explicit unknown/fail-closed verdict.
- Do not treat `can_use_base_plus_offset` as object-range proof.
- Do not add target-specific prepared/prealloc acceptance that bypasses
  target-independent BIR provenance facts.
- Do not weaken stale-row, duplicate-row, cross-block, dynamic-range, overflow,
  or out-of-range rejection to make a narrow testcase pass.

## Proof

Not run; lifecycle activation only.
