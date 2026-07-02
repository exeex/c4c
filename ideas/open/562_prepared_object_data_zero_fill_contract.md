# Prepared Object-Data Zero-Fill Contract

Status: Open
Type: Prepared contract implementation
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Derived From: `ideas/closed/548_prepared_global_stack_frame_infrastructure_review.md`
Owning Layer: Prepared object-data producer contract

## Goal

Publish coherent prepared object-data authority for ordinary zero-initialized
global objects so RV64 object emission does not receive
`unsupported_but_coherent` selected data for objects that should be representable
as zero fill.

## Why This Exists

Step 2 of the prepared infrastructure review classified `src/20000412-1.c` as
a prepared object-data contract gap. The prepared route reports an object label
and a 1656-byte extent, but selected object data is still
`unsupported_but_coherent` with no zero-fill authority.

## Representative Rows

- `src/20000412-1.c`

Evidence:

- `build/agent_state/548_step2_global_data_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20000412-1.c/case.log`

Observed diagnostic:

```text
unsupported_global_data: prepared selected object-data contract
status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656
emitted_byte_count=0 zero_fill_byte_count=0
```

## In Scope

- Repair prepared object/global-data production for ordinary zero-initialized
  global arrays and objects.
- Ensure selected object data carries coherent byte count or zero-fill authority
  before RV64 object emission consumes it.
- Add focused prepared or backend tests that prove zero-fill authority for the
  representative shape without relying on the filename.
- Re-run the representative row and show it moves off the prepared selected
  object-data contract diagnostic.

## Out Of Scope

- Teaching RV64 to consume `unsupported_but_coherent` selected object data.
- RV64 global-memory load/store lowering for scalar floating-point accesses.
- String-backed nested char-array BIR shape handoff already covered by closed
  bootstrap global data-shape work.
- F128 global data or long-double object routes.
- Changing gcc_torture expectations, unsupported markers, allowlists, or
  pass/fail accounting as progress.

## Acceptance Criteria

- Prepared object-data facts for the representative zero-initialized global
  object include coherent zero-fill or emitted-byte authority for the full
  object extent.
- `src/20000412-1.c` no longer fails with the prepared selected object-data
  contract diagnostic.
- Focused tests assert semantic prepared/global-data facts rather than matching
  the representative filename.
- Any remaining failure is classified as a downstream consumer gap with concrete
  evidence, not inferred around missing prepared facts.

## Reviewer Reject Signals

- Reject changes that make RV64 object emission accept or ignore
  `unsupported_but_coherent` selected object data instead of repairing prepared
  zero-fill authority.
- Reject filename-shaped handling for `src/20000412-1.c`, object-size-only
  shortcuts for 1656 bytes, or special cases for one global label.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  pass/fail accounting changes as capability progress.
- Reject broad RV64 global-object rewrites or floating-load work inside this
  prepared producer idea.
- Reject helper renames, diagnostic wording changes, or classification-only
  edits claimed as zero-fill contract repair.
- Reject retaining the same missing zero-fill authority behind a renamed
  prepared object-data status.
