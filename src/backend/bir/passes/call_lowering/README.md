# D2 Shared ABI-Aware Call Lowering Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: D2
Upstream: exact private D1 candidate and projection
Downstream: exact private D2 candidate submitted to D3

## Purpose

D2 is the sole shared owner that replaces every prospective `D.GenericCall`
with explicit abstract pseudo transport, call, clobber, and preservation
operations using exact C3/C4/C5/C8 requirements.

## Owns

Generic-call elimination, explicit abstract argument/result/byval/sret/variadic
transport, call-site clobbers/preservation obligations, candidate mapping,
fresh C9 projection, and D2 postconditions.

## Does Not Own

D2 does not spell concrete registers or frame offsets, assign general homes,
layout frames, spill, perform target one-to-many expansion, resolve copies,
publish `PseudoBir`, or reclassify ABI/call/helper requirements.

## Inputs

The exact D1 candidate/revision/projection plus unchanged C1 and exact C2-C9
lineage, especially C3 `AbiPlan`, C4 `CallPlan`, C5 `VariadicPlan`, and C8
helper requirements.

## Input NodeKind/Tag Vocabulary

Exactly the prospective D1 output groups. `D.GenericCall` is the only call
lowering input; all other groups receive explicit retain/reject rows. These
names are planning vocabulary, not production enum claims.

## Required Analyses and Products

Exact D1 CFG/value-flow/SSA, current projection, and C2-C9 products. D2 cannot
reconstruct a requirement or choose a compatible ABI.

## Ordered Behavior

1. Validate D1 revision/projection and every preparation fingerprint.
2. Inventory each `D.GenericCall` and assign its exact plan rows.
3. Reserve and build all explicit transport/call/clobber/preserve nodes in one
   private candidate with total result/provenance mapping.
4. Verify graph/SSA/requirements and request fresh C9 projection.
5. Freeze the complete candidate for D3 or discard it.

## NodeKind/Tag Lowering Matrix

| D1 planning input | Outcome and D2 planning output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| ordinary `D.GenericCall` | expand into abstract argument moves/stores, prospective `D.Call`, result moves, clobber and call-site preserve/restore obligations | signature/callee/args/result/effects | explicit pseudo transport/def-use/clobber/ABI-slot roles | generic-call placeholder | source retires; all outputs fresh with ordered call-role provenance and total result map | incomplete C3/C4 plan rejects |
| helper-derived `D.GenericCall` | same shared expansion using exact C8 interface + C3/C4 plan | helper semantic provenance/signature/effects | same explicit call transport roles | helper/generic-call placeholder | fresh IDs; helper source chain retained as provenance | helper mismatch rejects |
| variadic/byval/sret call | expand per exact C3/C4/C5 requirements | semantic transport/type obligations | explicit abstract stack-object/ABI-slot roles | unresolved transport | fresh outputs and total mapping | missing requirement rejects |
| non-call generic value/memory/effect/control | retain exact prospective group | all classifications/roles/effects | none | none | preserve | mutation forbidden |
| pending phi | retain for D5 | phi/type/predecessor roles/SSA | none | none | preserve | edge mutation forbidden |
| opaque inline asm | retain exact bytes/bindings | opaque/ordinary roles/effects | none | none | preserve | parsing or rebinding forbidden |
| remaining `D.GenericCall`, unknown/illegal/omitted/premature kind | reject | none | none | none | no candidate | `D2CallCoverageInvalid` |

## Identity and Provenance

Every call expansion retires the generic source and creates fresh role-ordered
nodes. Results map totally to explicit result transport. Abstract ABI slots are
requirements, not concrete register or frame identity.

## Outputs

One private D2 candidate with no `D.GenericCall`, complete explicit abstract
call transport/clobber/preserve operations, exact revision/lineage, and fresh
`ProjectedConstraintSet`.

## Verification and Publication

Verify total call-site and plan coverage, signatures/types/roles/effects,
def-use/CFG/SSA, exact projection, no generic-call leftovers, and absence of
concrete registers/frame offsets/general assignments/spills. D2 publishes no
stage; D3 owns the gate.

## Analysis Preservation and Invalidation

Call expansion invalidates value-flow, call graph, effects, SSA consumers,
liveness, and prior projection. Required facts are recomputed for the D2 key;
preparation products remain immutable lineage only.

## Failure and Diagnostics

Stale/mixed products, uncovered call, incomplete mapping, plan disagreement,
projection failure, concrete-resource attempt, cancellation, or verifier
failure discards the candidate and publishes nothing.

## Adjacent-Stage Contract

D1 supplies the sole input. D3 receives this exact frozen candidate and
projection and performs no rewrite. D4 alone owns later target expansion.

## Implementation State

Absent. Legacy per-target call lowering does not implement this shared owner.

## Proof Requirements

Prove ordinary/helper/direct/indirect/void/value/variadic/byval/sret neighbors,
total call elimination/mapping, exact plans/projection, and no concrete resource.

## Open Questions

New call transport shapes require explicit prospective schema and plan rows.
