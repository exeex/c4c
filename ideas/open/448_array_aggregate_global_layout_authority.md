# Array/Aggregate Global Layout Authority

Status: Open
Type: Producer/prepared authority idea
Parent: `ideas/closed/446_global_memory_layout_authority_publication.md`
Source Evidence: `build/agent_state/446_step4_residual_disposition/`
Owning Layer: BIR/prepared global memory layout authority before RV64 lowering

## Goal

Publish explicit array/aggregate global layout authority for prepared
global-symbol memory accesses such as `930930-1 @mem+792`, without RV64
target-side inference.

## Why This Exists

Idea 446 completed scalar global layout authority. Its Step 4 residual review
found `930930-1 @mem+792` still reports `layout_authority=unknown` with
`range_verdict=proven_in_bounds`. That row must remain fail-closed until the
producer/prepared pipeline can prove non-scalar global object identity, extent,
element or aggregate layout, offset, width, alignment, and range.

## In Scope

- Audit non-scalar global-symbol memory rows from
  `build/agent_state/446_step4_residual_disposition/`.
- Define the producer facts required for array and aggregate global object
  identity, extent, layout authority, element or field offset, width,
  alignment, and range.
- Add focused producer/prepared coverage for accepted non-scalar global layout
  authority and fail-closed missing or incoherent authority.
- Preserve idea 439 global-memory publication predicates and consume
  non-scalar global layout only after authority is explicit.

## Out Of Scope

- Scalar global layout authority already completed by idea 446.
- Immediate global-store source encoding, tracked separately by
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Pointer-value memory authority, direct-global return/select-chain authority,
  terminator/select publication, or local/frame-slot residuals.
- Inferring array or aggregate layout, object extent, addressability, offset
  meaning, or symbol identity in RV64 from raw BIR, symbol spelling, object
  labels, representative filenames, or dump shape.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Non-scalar global-symbol memory records are classified by missing versus
  coherent array/aggregate layout authority with representative evidence.
- Accepted records satisfy idea 439 global-memory publication predicates
  because producer facts are explicit.
- Missing or incoherent array/aggregate layout authority remains fail-closed.
- Any RV64 target work is split or sequenced after prepared authority is
  present.

## Reviewer Reject Signals

- Reject RV64 changes that infer array or aggregate global layout, object
  extent, addressability, offset meaning, or symbol identity from raw BIR,
  symbol spelling, object labels, testcase names, or one prepared dump layout.
- Reject testcase-shaped fixes keyed to `930930-1`, `@mem`, offset `792`, or
  one representative row.
- Reject claiming scalar global layout authority covers non-scalar rows that
  still report `layout_authority=unknown`.
- Reject weakening idea 439 publication predicates so unknown non-scalar
  layout authority becomes target-consumable.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as array/aggregate layout progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
