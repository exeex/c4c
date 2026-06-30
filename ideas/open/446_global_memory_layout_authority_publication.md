# Global Memory Layout Authority Publication

Status: Open
Type: Producer/prepared authority idea
Parent: `ideas/closed/439_store_source_global_memory_publication_authority.md`
Source Evidence: `build/agent_state/439_step4_residual_disposition/`
Owning Layer: BIR/prepared global memory layout authority before RV64 lowering

## Goal

Publish explicit global memory layout authority for prepared global-symbol
accesses so global load/store publication can become target-consumable without
RV64 inference.

## Why This Exists

Idea 439 added global-memory publication authority predicates and focused
fail-closed coverage. Representative rows still fail those predicates because
prepared global accesses such as `@mem+792` and `@global+0` report
`layout_authority=unknown`, even when range checks are otherwise
`proven_in_bounds`.

## In Scope

- Audit global-symbol accesses from
  `build/agent_state/439_step4_residual_disposition/`.
- Define the producer facts required for global object identity, extent,
  layout authority, offset, width, alignment, and range.
- Add focused producer/prepared coverage for accepted global layout authority
  and fail-closed missing/incoherent authority.
- Preserve the idea 439 publication predicates and consume them only after
  layout authority is explicit.

## Out Of Scope

- Reopening selected global object-data emission completed by idea 433.
- Inferring global object layout, symbol identity, addressability, or offsets
  in RV64 from raw BIR, symbol names, object labels, or testcase shape.
- Immediate store-source encoding.
- Pointer-value memory authority, direct-global return/select-chain authority,
  or terminator/select publication.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Global-symbol memory records are classified by missing versus coherent layout
  authority with representative evidence.
- Accepted records satisfy the idea 439 publication predicates because producer
  facts are explicit.
- Missing or incoherent global layout authority remains fail-closed.
- Any RV64 target work is split or sequenced after prepared authority is
  present.

## Reviewer Reject Signals

- Reject RV64 changes that infer global layout, object extent, addressability,
  offset meaning, or symbol identity from raw BIR, symbol spelling, object
  labels, or testcase names.
- Reject testcase-shaped fixes keyed to `930930-1`, `20041112-1`, `@mem`,
  `@global`, or one dump layout.
- Reject weakening idea 439 publication predicates so
  `layout_authority=unknown` becomes target-consumable.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as layout authority progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.

