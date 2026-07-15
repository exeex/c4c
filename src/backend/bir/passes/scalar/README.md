# B2 / P02 Scalar Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B2 / P02
Upstream: exact B1 `TypesLegal` checkpoint
Downstream: exact B2 `ScalarsCanonical` checkpoint for B3

## Purpose

B2 normalizes portable arithmetic, comparison, conversion, and select forms
without changing CFG topology or interpreting target facts.

## Owns

Closed scalar folding/identity rules, predicate orientation, proven cast-chain
composition, select normalization, and the `ScalarsCanonical` postcondition.

## Does Not Own

B2 does not legalize Raw/import-only forms, mutate successors/phi edges,
establish SSA, normalize memory/aggregates/intrinsics, select helpers/targets,
or publish a stage capability.

## Inputs

One exact B1 checkpoint carrying `RawVerified + TypesLegal`, its revision/stamp,
complete mutation lineage, and no forbidden later-stage facts.

## Input NodeKind/Tag Vocabulary

Exactly `B1.LegalValue`, `B1.LegalEffect`, `B1.RawControl`,
`B1.RawPhiMerge`, and `B1.OpaqueToken`. `B.RawImportOnly` is forbidden.

## Required Analyses and Products

One exact-revision `ComparisonSelect` result. `Known` facts may justify a
registered rule; `Unknown`/`Absent` preserve the already-valid input and never
authorize speculation. Stale, foreign, or mixed-revision handles reject.

## Ordered Behavior

1. Validate B1 stamp/properties and exact analysis key.
2. Inventory all scalar candidates and assign one closed matrix row.
3. Prove rule preconditions from typed facts and `Known` analysis results.
4. Build private per-function candidates, apply typed RAUW, and derive the
   aggregate mutation summary.
5. Verify all functions and atomically publish the complete B2 wave, or roll
   every candidate back.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B2 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| already canonical arithmetic/value member | retain as `B2.ScalarValue` | all six-axis facts | `ScalarsCanonical` checkpoint fact only | none | preserve | schema/payload mismatch rejects |
| registered fold/identity arithmetic member | replace or merge into `B2.ScalarValue` | exact value/type/semantic intent | output kind facts | source kind facts that changed | preserve only for exact same semantics/result/roles/effects; otherwise fresh ID + derivation | unproved equivalence rejects rewrite |
| comparison member | retain canonical form or replace with oriented/folded `B2.CompareValue` | compare domain/type/effects | canonical predicate/refinement | noncanonical predicate/refinement | normative identity gate | unknown predicate/domain rejects |
| conversion/cast member | retain, delete redundant cast via total RAUW, or replace a proven chain with `B2.ConvertValue` | exact conversion semantics/type | canonical conversion classification | redundant/noncanonical cast classification | deletion retires ID; replacement normally fresh | non-equivalent chain remains retained only if already legal; malformed rejects |
| select member | retain or replace/merge proven condition/arms into `B2.SelectValue` | result type and lazy poison/effect semantics | canonical select form | noncanonical select form | fresh ID unless all identity conditions hold | speculative arm choice rejects |
| `B1.LegalEffect` | retain as `B2.LegalEffect` | all effect/control/shape facts | none | none | preserve | any scalar rewrite of effect rejects |
| `B1.RawControl` | retain as `B2.RawControl` | control/successor/effect facts | none | none | preserve | topology mutation is forbidden |
| `B1.RawPhiMerge` | retain as `B2.RawPhiMerge` | predecessor roles/value/type | none | none | preserve | phi rewrite is forbidden |
| `B1.OpaqueToken` | retain as `B2.OpaqueToken` | opaque payload/roles/effects | none | none | preserve | text/target interpretation forbidden |
| unknown, illegal, omitted, or import-only kind | reject | none | none | none | no publication | `UnknownScalarForm` |

## Identity and Provenance

All replacements follow the normative gate. Constant/value substitution uses
typed total RAUW before retirement. Expansion receives fresh IDs and ordered
derivation roles; equal rendered expressions do not establish identity.

## Outputs

One exact immutable B2 checkpoint with the eight matrix-named B2 groups,
`RawVerified + TypesLegal + ScalarsCanonical`, exact revision lineage, and a
mutation-derived summary.

## Verification and Publication

Verify cumulative B1 rules, scalar normal forms, exact poison/undef/trap/
floating semantics, def-use, stable IDs, and unchanged CFG/later-owner forms.
The framework publishes only the complete green wave.

## Analysis Preservation and Invalidation

Changes to opcode, predicate, operands, results, types, constants, uses, or
conditions invalidate `ComparisonSelect` and all dependent value-flow/effect
facts unless exact registered preservation is proven under the new key.

## Failure and Diagnostics

Unknown forms, stale analysis, failed semantic proof, nonconvergence,
forbidden target authority, cancellation, verifier failure, or partial wave
publishes nothing and leaves B1 unchanged.

## Adjacent-Stage Contract

B1 supplies the sole input. B3 consumes only the exact B2 checkpoint and owns
all CFG/terminator topology normalization.

## Implementation State

Absent. Analysis and legacy scalar code do not implement this pass contract.

## Proof Requirements

Prove every matrix row, exact integer/floating/poison semantics, analysis-key
rejection, atomic wave rollback, idempotence, and no catch-all/target behavior.

## Open Questions

New scalar forms require a reviewed closed rule and matrix row; otherwise they
fail closed.
