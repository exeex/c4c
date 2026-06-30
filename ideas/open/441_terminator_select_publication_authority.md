# Terminator Select Publication Authority

Status: Open
Type: Terminator/select publication idea
Parent: `ideas/closed/433_rv64_global_select_pointer_memory_residuals.md`
Source Evidence: `build/agent_state/433_step4_residual_disposition/`
Owning Layer: Terminator and select publication facts before RV64 lowering

## Goal

Establish explicit publication authority for terminator/select residuals so
RV64 control-flow lowering can consume prepared facts without reconstructing
them from raw BIR.

## Why This Exists

After idea 433 completed the selected global object-data packet, remaining
representatives still fail through `unsupported_terminator_fragment` and
select-publication residuals. Rows include `20010329-1`, `20041112-1`, plus
select residuals in `20000622-1` and `930930-1`. These are not object-data
failures and need a separate terminator/select owner.

## In Scope

- Audit terminator/select publication evidence from
  `build/agent_state/433_step4_residual_disposition/`.
- Identify the prepared facts needed to publish select results into
  terminators, returns, branches, or value homes.
- Split coherent RV64 consumer packets from producer gaps.
- Add focused tests for accepted publication facts and fail-closed boundaries.

## Out Of Scope

- Global object-data support completed by idea 433.
- Pointer-value memory authority.
- Store-source/global-memory publication not tied to terminator/select
  publication.
- Direct-global return/select-chain authority beyond its interaction with
  terminator publication.
- Inferring select publication or terminator operands from raw BIR shape.

## Acceptance Criteria

- Terminator/select residuals are bucketed by first owner with representative
  evidence.
- Prepared publication facts and fail-closed boundaries are covered by focused
  tests.
- Any RV64 implementation consumes explicit publication facts rather than raw
  select or terminator shape.
- Remaining unrelated residuals are routed to separate ideas instead of folded
  into this route.

## Reviewer Reject Signals

- Reject RV64 changes that infer terminator operands, select results, or
  publication homes from raw BIR shape, block order, filenames, or function
  names.
- Reject testcase-shaped fixes keyed to `20010329-1`, `20041112-1`,
  `20000622-1`, `930930-1`, or one dump layout.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as publication progress.
- Reject broad control-flow rewrites that retain the same
  `unsupported_terminator_fragment` owner behind a renamed helper.

