# Immediate Global-Store Source Encoding

Status: Open
Type: Producer/prepared publication idea
Parent: `ideas/closed/439_store_source_global_memory_publication_authority.md`
Source Evidence: `build/agent_state/439_step4_residual_disposition/`
Owning Layer: BIR/prepared store-source publication before RV64 global stores

## Goal

Publish explicit immediate-source encoding for global stores so store-global
publication authority can distinguish immediate values from unknown sources.

## Why This Exists

Idea 439 added store-global publication authority predicates and coverage. In
`20041112-1`, `main.entry.0` stores immediate `1` to `@global+0`, but the
prepared store-source row has `source=<none>` and
`source_producer=unknown`. That must remain fail-closed until the producer
publishes an explicit immediate-source fact.

## In Scope

- Audit store-global rows with immediate values from
  `build/agent_state/439_step4_residual_disposition/`.
- Define the prepared source encoding for immediates, including value type,
  width, signedness or bit pattern, destination global, and publication intent.
- Add focused producer/prepared tests for immediate global-store source
  publication and fail-closed unknown-source cases.
- Preserve idea 439 store-publication predicates and consume immediate sources
  only when encoded explicitly.

## Out Of Scope

- Global layout authority publication; that is a separate prerequisite when
  `layout_authority=unknown`.
- RV64 target-side guessing of immediate values from raw BIR store shape.
- Pointer-value memory authority, direct-global return/select-chain authority,
  or terminator/select publication.
- Rewriting expectations, allowlists, unsupported markers, or runtime
  comparisons.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Immediate global-store rows are classified by explicit versus missing source
  encoding with representative evidence.
- Accepted immediate-source records are covered by focused tests and satisfy
  store-source publication authority.
- Missing immediate-source encoding remains fail-closed as
  `source_producer=unknown` or a precise replacement diagnostic.
- RV64 global-store lowering consumes only explicit immediate-source facts.

## Reviewer Reject Signals

- Reject RV64 code that infers immediate store values from raw BIR, testcase
  names, block labels, symbol names, or one dump layout.
- Reject testcase-shaped fixes keyed to `20041112-1`, `main.entry.0`,
  immediate `1`, `@global`, or one store row.
- Reject weakening store-source authority so `source_producer=unknown` becomes
  target-consumable.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as source-encoding progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.

