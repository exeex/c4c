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

## Resumption Record: canonical HIR aggregate-occurrence ref blocker

- Last accepted progress: Step 1 established the canonical aggregate-ref/store
  seam in commits `69ffa299f` and `a50d35d4e`. Step 2's declaration/store fact
  capture was accepted in `8eca000c9`: `build_type_decls` snapshots ordered
  structured LIR field types, packed/opaque flags, and direct/byte-storage/union
  layout kind before declaration recording.
- Completed steps: Step 1 is accepted. Step 2 has accepted declaration/store
  fact capture but remains incomplete.
- Interrupted step: Step 2 — **Preserve recursive aggregate facts and all
  aggregate forms**.
- Blocker outside this idea's scope: `QualType::aggregate_ref` exists but HIR
  construction does not populate it for aggregate occurrences, including
  function return and parameter occurrences. `Lowerer::qtype_from` therefore
  retains only legacy `aggregate_owner_identity`; canonical-reference
  enforcement breaks supported aggregate function signatures. Populating the
  definition-backed HIR occurrence fact is an upstream HIR producer migration,
  outside this idea's LIR M4--M6 scope.
- Active blocker successor: `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`.
  It must provide definition-backed canonical `HirAggregateRef` values for
  aggregate `QualType` occurrences with explicit invalid, foreign, and missing
  behavior. It must not absorb this idea's LIR producer migration.
- Exact return point: after the blocker is accepted, resume Step 2 and retry
  only the bounded `lir_owned_type_spec` function-signature occurrence-producer
  migration using populated canonical HIR refs and module store facts.
- Remaining next action on return: retain the captured declaration facts and
  migrate that one LIR occurrence producer; do not add owner-key, tag, or
  rendered-text fallback.
- Accepted proof/commit references: `8eca000c9` has the accepted fresh
  `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` proof (6/6).
  The attempted partial canonical-enforcement patch was reverted; its failed
  `test_after.log` is not acceptance evidence.
