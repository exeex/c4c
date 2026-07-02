# RV64 Runtime And No-Diagnostic Failure Triage

Status: Open
Type: Evidence reconstruction and runtime triage
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: Runtime mismatch, crash triage, or first-owner reconstruction

## Goal

Reproduce and classify current failures that lack explicit ownership evidence
before they are used to justify implementation work.

## Why This Exists

The current bucket map records 503 compile failures without an explicit
`unsupported_*` diagnostic, 57 other non-unsupported failures, and 7
segmentation-fault exits. These 567 rows are evidence gaps, not an
implementation-ready ordinary-C bucket.

## In Scope

- Reproduce representative no-diagnostic compile failures and crash rows.
- Identify whether rows belong to BIR producer, prepared contract, RV64
  lowering, runtime mismatch, test infrastructure, timeout, or unsupported
  feature owners.
- Split high-frequency reconstructed owners into durable follow-up ideas.
- Preserve minimal reproduction commands and logs for reviewer audit.

## Out Of Scope

- Claiming broad RV64 capability progress from unclassified failures.
- Implementing crash fixes without a reproduced first bad fact.
- Weakening runtime comparison, expected output, unsupported markers, or
  pass/fail accounting.
- Treating primary-F128 crashes as ordinary-C blockers.

## Acceptance Criteria

- Representative rows from each no-diagnostic family have reproducible logs.
- High-frequency first owners are named with current evidence or explicitly
  left as evidence gaps.
- Runtime mismatch work is separated from compile-time producer or lowering
  work.
- New implementation ideas are created only after first ownership is proven.

## Reviewer Reject Signals

- Reject implementation claims based only on a crash, timeout, or compile-fail
  count without first-owner reproduction.
- Reject changing runtime comparison, expected output, or allowlists to turn
  failures into passes.
- Reject named-case fixes that make one reproduced row pass while nearby rows
  remain unclassified.
- Reject merging runtime mismatch, producer repair, and RV64 lowering into one
  broad slice.
- Reject leaving the same no-diagnostic failure hidden behind a new wrapper or
  diagnostic name.

