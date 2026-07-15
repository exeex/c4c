# E4 Allocated and MIR-Readiness Publication Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: E4
Upstream: exact private post-stable-E3 D5-resolved candidate and product lineage
Downstream: owning Allocated capability plus distinct borrowing MIR-readiness view

## Purpose

E4 materializes every remaining frame action as an explicit bounded one-record
node, recomputes/validates all exact-current products, and atomically publishes
the Allocated graph plus a separate MIR-readiness capability over that same
revision. It performs no allocation repair or retry.

## Owns

Private frame-action draft, deterministic action materialization,
`FrameActionFingerprint`, final projection/E1 recomputation, non-mutating E2/E3
validation, final frame plan/target realizability, and atomic E4 publication.

## Does Not Own

E4 does not change assignments, choose new spills, retry allocation, resolve
parallel copies, perform hidden frame work in F1, create machine records,
encode instructions, or reinterpret source-semantic stack operations.

## Inputs

One exact private D5-resolved candidate after stable E3 with current projection,
`CopyResolutionFingerprint`, E1/E2 assignments, E3 spill state, C1-C9/D1-D5
lineage, and all target/layout/call/frame requirement identities.

## Input NodeKind/Tag Vocabulary

Exactly prospective assigned directly realizable ordinary pseudo nodes,
`EdgeCopy`, explicit `Spill`/`Reload`, opaque inline asm, and admitted control/
effect forms. Pending phi, expansion placeholder, `GenericCall`,
`ParallelCopy`, `CopyScratch`, premature frame actions, and machine kinds reject.

## Required Analyses and Products

Exact current projection, copy-resolution fingerprint, C2 layout, C3-C8 plans,
C9 binding, E1/E2/E3, D5 lineage, and deterministic frame-action requirement
draft inputs. No predecessor-only or equal-looking product is accepted.

## Ordered Behavior

1. Freeze/validate the exact D5-resolved candidate and all predecessor keys.
2. Derive one deterministic private frame-action draft from actual assignments,
   spill objects, calls, dynamic stack semantics, and target requirements.
3. Materialize only bounded prospective `FrameAdjust`, `FrameBaseSetup`,
   `FrameBaseRestore`, `FrameCalleeSave`, `FrameCalleeRestore`, and `FrameProbe`
   nodes at exact planned points; derive mutation summary/fingerprint.
4. Request final C9 projection, recompute E1 on the materialized revision, run
   E2 assignment and E3 spill validation without mutation, then derive final
   frame plan and target realizability.
5. Run the full Allocated gate and atomically publish both capabilities, or
   discard the complete transaction. No retry edge exists.

## NodeKind/Tag Lowering Matrix

| E4 input/output subset | Outcome | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| ordinary assigned realizable node | retain as `E.AllocatedOperation` | semantic/type/effect/pseudo intent, exact assignments | Allocated admission and exact home/product references | unresolved allocation state | preserve only under identity gate | missing/illegal assignment rejects |
| resolved `EdgeCopy` | retain as `E.AllocatedCopy` | transfer type/endpoints/edge provenance | allocated endpoints and one-record realizability | unresolved copy state | preserve | illegal move/mapping rejects |
| explicit `Spill`/`Reload` | retain as `E.AllocatedSpillAction` | spill object/type/memory effects/def-use | Allocated admission and exact homes/frame-plan reference | unresolved spill residency | preserve | hidden/unassigned transition rejects |
| opaque inline asm | retain as `E.AllocatedOpaque` | exact bytes/bindings/effects | exact assigned operands/clobber satisfaction | unresolved homes | preserve | parsing or incomplete assignment rejects |
| required frame action | insert one bounded `E.FrameAction` at planned point | target/frame obligation provenance | allocation-action family, fixed roles, Allocated admission, one-record disposition | hidden/unmaterialized frame requirement | always fresh ID with requirement/point provenance | missing/duplicate action rejects |
| source-semantic stack save/restore/dynamic allocation | retain as ordinary semantic/pseudo operation | original semantics/effects | exact allocated realization facts | none | preserve; never replaced by frame action | reinterpretation rejects |
| residual phi/`ParallelCopy`/`CopyScratch`/placeholder/`GenericCall` | reject | none | none | none | no publication | `AllocatedResidualVocabulary` |
| unknown/illegal/omitted/stale/premature/machine kind | reject | none | none | none | no publication | `AllocatedVocabularyInvalid` |

## Identity and Provenance

Every frame action gets a fresh ID tied to exact requirement and insertion
point. Retained nodes preserve identity only when all eight conditions hold.
Assignments/frame locations/products are not node identity. Machine identity is
later distinct; BIR IDs may be provenance only.

## Outputs

One owning immutable Allocated graph/capability and one distinct borrowing
MIR-readiness capability/view. Both name the identical materialized revision,
projection, E1/E2/E3, copy/frame fingerprints, frame plan, and target
realizability key. Neither borrowing capability owns or copies graph storage.

## Verification and Publication

The Allocated gate proves complete legal assignments/spill residency, no
pressure deficit, resolved copies, exact frame-action coverage, no hidden frame
work, one-record realizability, final projection/E1 keys, non-mutating E2/E3
validation, and all products exact-current. It rejects any repair request,
unresolved/unknown vocabulary, stale product, active editor, or partial module.
Success atomically mints both outputs; failure mints neither.

## Analysis Preservation and Invalidation

Frame-action insertion invalidates projection, E1 and every observing product.
E4 recomputes them after materialization. E2/E3 are validators only and cannot
mutate/retry. Any later graph/key change invalidates MIR readiness entirely.

## Failure and Diagnostics

Missing/duplicate action, assignment/spill/copy drift, stale/mixed product,
pressure deficit, hidden frame requirement, failed realizability, cancellation,
or resource exhaustion rolls back copy/frame/product staging and publishes
nothing. There is no allocation-repair fallback.

## Adjacent-Stage Contract

Only subordinate post-E3 D5 supplies the private resolved input. F1 consumes
only the borrowing MIR-readiness view and applies one mapping per explicit node;
it cannot insert frame work, expand, allocate, spill, or return repair.

## Implementation State

Absent. Prospective frame-action names are planning vocabulary, not production
`NodeKind` claims. Existing legacy prepared/frame emission is not E4.

## Proof Requirements

Prove every assigned/copy/spill/opaque/frame row, exact action multiplicity,
materialize-before-project/E1 order, non-mutating E2/E3, stale products,
failure atomicity, no repair, and same-revision dual capability binding.

## Open Questions

New frame actions require reviewed schema/admission/realizability rows before
implementation; no hidden F1 action is allowed.
