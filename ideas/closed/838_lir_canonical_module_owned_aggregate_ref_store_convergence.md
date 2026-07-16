# LIR Canonical Module-Owned Aggregate Ref/Store Convergence

Status: Closed
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

## Closure Record

- Disposition: capability complete for the bounded A1 M4--M6 aggregate
  convergence route.
- Accepted implementation: Step 1 established the canonical HIR aggregate
  ref/store seam in `69ffa299f` and `a50d35d4e`; Step 2 declaration/store fact
  capture was accepted in `8eca000c9`; the bounded `lir_owned_type_spec`
  function-signature occurrence migration was accepted through `88ccf591d`;
  Step 3 store-backed declaration, field, call, verifier, printer, and receiver
  consumers were accepted through `9497d5980`.
- Accepted proof: the rejected baseline expansion was repaired, then the
  supervisor accepted a fresh full-suite baseline review at `53a1a8515` with
  3038/3038 tests passing. Step 4 assessment was accepted through
  `1ff60849f`.
- Completion judgment: 838's acceptance criteria are satisfied for aggregate
  references interned once into the owning LIR module aggregate store, including
  focused coverage for nested recursion, repeated interning,
  registration-before-use, named aggregate forms, and fail-closed rejection for
  invalid refs while preserving accepted 754/798/801/803 seams.
- Remaining work is intentionally not absorbed by 838: `StructNameId` mirrors,
  aggregate compatibility helpers, runtime-text factories, raw call/signature
  parsing, extern/global text mirrors, GEP/local-memory compatibility bridges,
  and direct HIR construction text paths remain deletion gates for F1, V1, S1,
  U1, P1, G1, R1, C1, T1, 836/831, 812/813, and 797.
- Non-goals preserved: this closure does not close or resume 836/831 and does
  not claim function, vector, scalar, union, generic verifier/printer,
  collector, global, or universal-model migration beyond the bounded
  aggregate-store consumers named above.

## Historical Resumption Record: canonical HIR aggregate-occurrence ref return

- Last accepted progress: Step 1 established the canonical aggregate-ref/store
  seam in commits `69ffa299f` and `a50d35d4e`. Step 2's declaration/store fact
  capture was accepted in `8eca000c9`: `build_type_decls` snapshots ordered
  structured LIR field types, packed/opaque flags, and direct/byte-storage/union
  layout kind before declaration recording.
- Completed steps: Step 1 is accepted. Step 2 has accepted declaration/store
  fact capture but remains incomplete.
- Interrupted step: Step 2 — **Preserve recursive aggregate facts and all
  aggregate forms**.
- Prior blocker outside this idea's scope: `QualType::aggregate_ref` existed
  but HIR construction did not populate it for aggregate occurrences, including
  function return and parameter occurrences. Populating that definition-backed
  HIR occurrence fact was an upstream HIR producer migration outside this
  idea's LIR M4--M6 scope.
- Blocker disposition: closed 852 satisfied the return condition. Commit
  `a4f822740` closed
  `ideas/closed/852_hir_canonical_semantic_aggregate_ref_binding.md`; its
  accepted production HIR work populated ordinary aggregate free-function
  return and parameter `QualType::aggregate_ref` occurrences and proved the
  handoff with a fresh default build plus focused `frontend_hir_tests`.
- Exact return point: resume Step 2 and migrate only the bounded
  `lir_owned_type_spec` function-signature occurrence producer to consume the
  populated `QualType::aggregate_ref` through the already accepted
  HIR-ref-to-LIR-ref intern relation.
- Remaining action: retain the captured declaration facts and migrate that one
  LIR occurrence producer; do not add owner-key, tag, parser-pointer,
  `record_def`, rendered-text, runtime-string, or `Node*` reconstruction.
- Accepted proof/commit references: `8eca000c9` has the accepted fresh
  `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` proof (6/6).
  `a4f822740` closed the upstream HIR prerequisite after accepted
  `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' ) > test_after.log 2>&1`
  proof. The attempted pre-852 partial canonical-enforcement patch was
  reverted; its failed `test_after.log` is not acceptance evidence.
