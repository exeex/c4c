# RV64 Bankless Or Conversion-Adjacent Stack Slot Moves

Source Parent: ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md
Owning Layer: Prepared storage classification and RV64 object emission

## Goal

Resolve prepared stack-slot moves whose homes look stack-to-stack but whose
storage facts lack a GPR bank or whose source and destination scalar types
imply a conversion rather than a same-scalar copy.

## Why This Exists

Idea 513 deliberately rejected stack-slot to stack-slot moves unless prepared
home and storage-plan facts were coherent for a same-scalar GPR frame-slot
copy. Step 5 evidence for `src/pr69447.c` showed the first remaining failure
has stack homes for values 15 and 16, but the source storage-plan entry is
`frame_slot bank=none` and the move is adjacent to an `i16` to `i64`
conversion shape. Accepting that row in idea 513 would have inferred bank or
conversion semantics that were not explicitly published.

## In Scope

- Reproduce and inspect the `src/pr69447.c` prepared move, storage-plan, and
  nearby type/conversion facts.
- Decide whether `bank=none` frame slots should be repaired by producer
  storage classification or rejected with a more precise diagnostic.
- Decide whether conversion-adjacent stack-slot movement needs an explicit
  conversion materialization contract separate from same-scalar copies.
- Add focused coverage for bankless frame-slot rejection or repaired
  publication, and for any accepted explicit conversion materialization.

## Out Of Scope

- Inferring GPR bank from scalar type alone when the prepared storage plan does
  not publish that authority.
- Treating conversion-adjacent moves as raw same-width copies without an
  explicit conversion contract.
- Broad integer conversion lowering unrelated to prepared stack-slot movement.
- Reopening idea 513's coherent same-scalar stack-slot to stack-slot
  materializer.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes claimed as capability progress.

## Acceptance Criteria

- `src/pr69447.c` no longer fails because a bankless or conversion-adjacent
  stack-slot move is misreported as the same coherent stack-to-stack copy shape
  accepted by idea 513.
- Bankless frame-slot facts are either repaired at the producer or rejected
  with an owner-specific diagnostic.
- Conversion-adjacent stack-slot moves are either explicitly materialized under
  a documented contract or rejected before the generic stack-copy materializer.
- Validation includes a fresh build, focused representative proof, focused
  backend coverage, and the relevant backend subset.

## Reviewer Reject Signals

Reject any route or slice that:

- treats `pr69447.c` as a named special case
- assumes a GPR bank when storage-plan facts explicitly say `bank=none`
- emits a raw stack copy for a conversion-adjacent move without proving the
  conversion semantics
- weakens diagnostics, expectations, unsupported markers, allowlists, or scan
  accounting instead of repairing the storage or conversion owner
- broadens into unrelated integer conversion or ABI work without preserving the
  prepared move authority boundary
