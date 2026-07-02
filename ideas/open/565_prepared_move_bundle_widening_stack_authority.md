# Prepared Move-Bundle Widening Stack Authority

Status: Open
Type: Prepared classifier implementation
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Derived From: `ideas/closed/548_prepared_global_stack_frame_infrastructure_review.md`
Owning Layer: Prepared move-bundle classifier authority

## Goal

Publish prepared authority for conversion-adjacent stack-slot to stack-slot
integer moves where the source and destination widths differ, or split the
shape into an explicit supported conversion plus stack-destination form before
RV64 consumption.

## Why This Exists

Step 4 of the prepared infrastructure review classified the representative
move-bundle rows as prepared classifier authority gaps. Both stop in
`prepared_move_bundle_classifier` with `authority=none` for stack-slot source
to stack-slot destination widening moves.

## Representative Rows

- `src/20010224-1.c`
- `src/pr87623.c`

Evidence:

- `build/agent_state/548_step4_move_bundle_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20010224-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr87623.c/case.log`

Observed fragments:

```text
producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move
authority=none
```

Representative shapes:

- `src/20010224-1.c`: stack-slot source to stack-slot destination,
  `i16` to `i32`, `source_size_bytes=2`, `destination_size_bytes=4`
- `src/pr87623.c`: stack-slot source to stack-slot destination,
  `i8` to `i32`, `source_size_bytes=1`, `destination_size_bytes=4`

## In Scope

- Repair prepared move-bundle classification for conversion-adjacent widening
  integer moves between stack slots.
- Define the supported authority shape or split into explicit prepared
  conversion plus stack-destination move facts.
- Add focused prepared/backend tests for stack-slot source to stack-slot
  destination widening authority.
- Re-run both representative rows and show they move off
  `unsupported_prepared_move_bundle_classification`.

## Out Of Scope

- RV64 object-route consumption before prepared authority is published.
- Arbitrary memory-to-memory copies unrelated to conversion-adjacent stack-slot
  widening.
- F128 or long-double move-bundle policy.
- Helper renames, diagnostic wording changes, expectation rewrites, or
  pass/fail accounting.

## Acceptance Criteria

- The prepared classifier no longer reports `authority=none` for the
  representative widening stack-slot move shapes.
- Tests cover both narrower-source cases or a semantic width-general rule that
  subsumes them.
- RV64 handoff is attempted only after prepared authority is explicit.
- Any remaining representative failure is a later owner with a concrete
  diagnostic, not the same prepared classifier rejection.

## Reviewer Reject Signals

- Reject routing these rows to RV64 object-route work while
  `prepared_move_bundle_classifier` still reports `authority=none`.
- Reject filename-shaped handling for `src/20010224-1.c` or `src/pr87623.c`,
  source-width-only shortcuts, or event-name matching as capability repair.
- Reject changing diagnostics, unsupported markers, allowlists, or expected
  pass/fail accounting as progress.
- Reject broad move-bundle rewrites that do not prove the stack-slot
  source/destination widening authority shape.
- Reject helper renames or classification-only edits claimed as prepared
  authority repair.
- Reject retaining the exact old conversion-adjacent rejection behind a renamed
  classifier status.
