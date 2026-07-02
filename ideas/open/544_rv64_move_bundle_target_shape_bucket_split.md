# RV64 Move-Bundle Target-Shape Bucket Split

Status: Open
Type: Bucket review and follow-up splitter
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: Prepared contract plus RV64/MIR boundary

## Goal

Consume the stable 2026-07-02 `unsupported_move_bundle_target_shape` bucket
and split its 183 current rows into coherent RV64 move-materialization work
versus prepared/BIR authority gaps.

## Why This Exists

The post-cleanup RV64 gcc_torture evidence identifies
`unsupported_move_bundle_target_shape` as the largest explicit current
ordinary-C bucket. A single implementation idea would be too broad because
some rows may have complete prepared facts that RV64 can lower, while others
may need earlier BIR or prepared producer repair.

## In Scope

- Reconstruct the 183-row current bucket from the 2026-07-02 scan evidence.
- Classify rows by first owner: coherent RV64/MIR materialization,
  prepared-module target-shape authority, BIR semantic producer, or evidence
  gap.
- Create separate follow-up implementation ideas only for rows whose facts are
  coherent enough for the owning layer.
- Preserve row evidence for representative cases and any rows deferred to
  producer-owned work.

## Out Of Scope

- Implementing RV64 lowering inside this review idea.
- Guessing missing prepared facts from case names, BIR text shape, or target
  register names.
- Combining producer repair and RV64 lowering in one implementation slice.
- Changing gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison.
- Treating F128 rows as ordinary-C move-bundle progress.

## Acceptance Criteria

- The 183-row bucket is reproducible from current scan artifacts or a fresh
  equivalent scan.
- Each row is classified by first owner with enough evidence for a reviewer to
  audit the route decision.
- Coherent RV64 materialization rows and prepared/BIR authority gaps become
  separate follow-up ideas or explicitly documented subqueues.
- Any F128-primary row is routed to the existing F128 quarantine lane.
- No implementation or test-contract changes are required to close this
  review idea.

## Reviewer Reject Signals

- Reject RV64 lowering that infers missing target-shape authority from a
  testcase name, filename, register spelling, or raw BIR fragment.
- Reject classifying all 183 rows as RV64 work without proving prepared facts
  are complete.
- Reject mixing producer repair and RV64 materialization in the same
  implementation slice.
- Reject unsupported downgrades, expectation rewrites, or allowlist filtering
  as evidence that the bucket was improved.
- Reject helper renames or diagnostic text changes claimed as capability
  progress while the same `unsupported_move_bundle_target_shape` rows remain.

