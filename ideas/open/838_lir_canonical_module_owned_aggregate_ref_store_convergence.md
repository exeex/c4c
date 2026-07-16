# LIR Canonical Module-Owned Aggregate Ref/Store Convergence

Status: Open
Type: first-owner nominal aggregate identity, store, and lowering convergence
Matrix Rows: M4, M5, M6
Dependencies: accepted 834/835 relation; evidence from 754, 798, 801, 803, and 832--836

## Goal

Make every aggregate-bearing occurrence carry a stable canonical HIR aggregate
reference and intern it once into its owning LIR module's aggregate store.

## In Scope

- Introduce `HirAggregateRef { ModuleId, HirAggregateId }` or a proven-equivalent
  stable carrier, and map it to `LirAggregateRef` with module/lowering-session
  lifetime.
- Make the store own struct/union kind, ordered fields, layout, projections,
  and recursive typed children for named, anonymous, local, template, and
  typedef/alias occurrences. Register definitions before occurrence lowering.
- Fail closed for unknown, incomplete, stale, foreign, and wrong-module refs;
  retain the three 836 groups separately: incomplete key/ref, present but
  unmatched module owner, and legitimate no-owner compatibility.

## Out Of Scope

- Completing or closing parked 836, repairing 831's comparable gate, or
  returning to 831; 836 retains its own Step 1 and later 831 Step 4 return.
- Function, vector, scalar, union, generic verifier/printer, or terminal
  universal-model migration.

## Acceptance Criteria

- Nested recursion, repeated interning, registration-before-use, every named
  aggregate form, and foreign/wrong-module/incomplete rejection have focused
  lowering, verifier, and printer proof plus a fresh build.
- Preserve parity at accepted 754/798/801/803 seams.
- Retire `record_def`, reconstructed owner keys, tag/rendered-text/cross-table
  lookup, and duplicate owner/identity/layout/operation/field metadata only
  after each named declaration, field, call, verifier, printer, and receiver
  uses store facts. Keep legitimate no-owner compatibility until its named
  consumer migrates.

## Reviewer Reject Signals

- Reject tag, parser-pointer, rendered-text, or reconstructed-key recovery.
- Reject merging the three 836 groups or claiming this closes 836/831.
- Reject operation-local aggregate metadata retained as competing authority,
  testcase-shaped fixes, weakened owner rejection, or broad family migration.
