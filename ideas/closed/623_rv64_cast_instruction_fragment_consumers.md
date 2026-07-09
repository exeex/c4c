# RV64 Cast Instruction Fragment Consumers

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Split From: `ideas/closed/612_rv64_instruction_fragment_consumers.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: RV64/MIR cast consumer, unless refreshed probes prove producer ownership
Queue Order: 23
Prerequisites: refresh current cast residual diagnostics before implementation
Estimated Evidence Breadth: cast-shaped `unsupported_instruction_fragment` rows that remain after idea 612 pointer-route closure
Proof Surface: RV64/MIR object or backend diagnostics for cast instruction-fragment rows and nearby non-cast guard rows

## Goal

Repair RV64/MIR consumer lowering for cast-shaped instruction fragments only
when refreshed diagnostics prove the first stop is a consumer-side cast
fragment with complete upstream producer facts.

## Why This Exists

Idea 612 closed the mixed binary/pointer instruction-fragment route after the
remaining non-cast residuals were reclassified to existing owners or
out-of-scope policy gaps. Cast rows were the only visible source-scope residual
worth preserving as a durable follow-up, but they need their own refresh and
ownership split before any implementation.

## In Scope

- Refreshing cast-shaped `unsupported_instruction_fragment` diagnostics.
- Splitting cast rows by RV64 consumer gap versus BIR producer, semantic cast,
  ABI, global, local-memory, select, branch, move-bundle, runtime, or policy
  ownership.
- RV64 consumer lowering for a cast sub-family with multiple refreshed rows
  and complete upstream facts.
- Diagnostic-preserving rejection for cast rows still missing producer,
  prepared, width, signedness, source-kind, or authority facts.

## Out Of Scope

- BIR semantic cast production.
- Pointer `BinaryInst`, scalar narrow-integer, move-bundle, terminator, ABI,
  runtime, local-memory, select, branch, global, expectation, unsupported-marker,
  allowlist, timeout, or accounting changes.
- Floating, vector, or library policy cast lanes unless refreshed evidence
  proves ordinary-C backend leverage and the active runbook explicitly narrows
  to that family.

## Acceptance Criteria

- The active runbook first refreshes and sub-buckets current cast residuals.
- Any implemented cast sub-family progresses multiple rows or records a clear
  no-breadth blocker.
- Non-implemented cast rows keep accurate unsupported or owner diagnostics.
- Nearby non-cast guard rows from idea 612 remain outside this route.

## Closure Notes

Closed after the Step 7 zext consumer slice at `132ae8871`. The runbook
refreshed and classified the residual cast rows, selected the 18-row
`rv64-consumer:width-preserving-zext-i32-to-i32` family as the coherent
multi-row cast-consumer packet, and implemented that packet without expectation,
unsupported-marker, allowlist, timeout, or accounting changes.

The corrected Step 7 proof split no longer has `instruction_kind=CastInst`
failures for the 18 zext rows. Remaining proof-visible rows from that selected
set are runtime mismatches, C4C link failures, and non-cast object compile
failures, including `CallInst` object compile rows; those are out of scope for
idea 623 and should be handled by separate owner routes if pursued.

Step 5 also preserved the lower-breadth trunc classification: two compile-time
trunc `CastInst` rows and one trunc-family runtime mismatch. They were not the
selected Step 7 route and should not be silently folded into this closed
runbook; a future trunc-specific continuation should be opened explicitly if
the supervisor wants that work.

## Reviewer Reject Signals

- Reject named-case-only cast lowering for one source file or one operation id.
- Reject treating BIR semantic cast production as RV64 consumer progress.
- Reject expectation, unsupported-marker, allowlist, timeout, or accounting
  rewrites claimed as cast lowering progress.
- Reject broad pointer `BinaryInst`, ABI, local-memory, select, branch,
  move-bundle, global, runtime, or terminator changes under this cast idea.
- Reject any route that keeps the same cast `unsupported_instruction_fragment`
  failure behind a renamed helper or diagnostic.
- Reject implementation from stale idea-612 counts without refreshed cast
  owner evidence and negative guard rows.
