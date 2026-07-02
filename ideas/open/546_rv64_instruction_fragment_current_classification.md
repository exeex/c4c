# RV64 Instruction-Fragment Current Classification

Status: Open
Type: Current bucket classification
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: RV64/MIR object lowering, with producer-gap boundaries

## Goal

Classify the 137 current `unsupported_instruction_fragment` rows from the
stable 2026-07-02 evidence into implementation-ready RV64/MIR sub-buckets,
producer gaps, and F128 quarantine rows.

## Why This Exists

Older instruction-fragment work contains a useful taxonomy, but its row counts
are stale. The current 137-row bucket must be reclassified before it can drive
implementation ideas.

## In Scope

- Reconstruct the current 137-row instruction-fragment set.
- Split rows by semantic operation family, prepared fact completeness, and
  likely first owner.
- Exclude primary-F128 rows from ordinary-C progress accounting and route them
  to the existing F128 quarantine idea.
- Create narrower RV64 implementation ideas only for rows with coherent
  prepared/BIR facts.

## Out Of Scope

- Implementing instruction lowering in this classification idea.
- Reusing stale historical row counts as current counts.
- Folding aggregate ABI, BIR admission, or prepared authority defects into
  RV64 lowering.
- Changing pass/fail accounting, unsupported markers, or expected outputs.

## Acceptance Criteria

- The current instruction-fragment row set is traceable to the active scan
  evidence.
- Each high-frequency sub-bucket names its first owner and whether it is ready
  for RV64/MIR implementation.
- F128-primary rows are routed to the F128 quarantine lane.
- Any produced implementation idea has concrete row evidence and rejects
  testcase-shaped lowering.

## Reviewer Reject Signals

- Reject implementation work based on the old 190-row or 82-row
  instruction-fragment counts without current 2026-07-02 reconstruction.
- Reject named-case, opcode-text-only, or raw-fragment matching as a lowering
  strategy.
- Reject treating missing BIR/prepared facts as RV64 inference work.
- Reject expectation rewrites, unsupported downgrades, or allowlist filtering
  as progress.
- Reject mixing F128 helper or long-double work into ordinary scalar
  instruction-fragment slices.

