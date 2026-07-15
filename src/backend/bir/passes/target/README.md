# D4 Target Pseudo Realizability and Expansion Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: D4
Upstream: exact D3 allocation-free `PseudoBir` plus current projection
Downstream: fully reverified directly realizable D4 Pseudo revision for D5

## Purpose

D4 is an always-on target realizability gate and legalization chain. It makes
every required target-specific one-to-many expansion explicit before
allocation and eliminates every prospective `D.ExpansionPlaceholder`.

## Owns

Versioned target pseudo-rule selection, mandatory realizability check, explicit
one-to-many expansion, total mappings, fresh C9 projection, and full Pseudo
reverification/publication.

## Does Not Own

D4 does not redo ABI classification or D2 call lowering, assign homes, spill,
lay out frames, construct machine records, remove SSA, or hide temporaries in a
product. Optional target optimizations require separately reviewed registry rows.

## Inputs

One exact D3 publication with full `PseudoStageKey`, D2 lineage, current
`ProjectedConstraintSet`, unchanged C1/C2-C9 products, and dynamic SSA proof.

## Input NodeKind/Tag Vocabulary

Exactly the prospective D3 groups accepted by the allocation-free gate:
generic value/memory/effect/control, explicit D2 call transport, pending phi,
opaque inline asm, and bounded expansion obligations. Planning names are not
production enum claims.

## Required Analyses and Products

Exact target realizability registry, C2 layout, preparation products, current
CFG/value-flow/effects/SSA, D2 facts, and projection. Equal-looking targets or
stale products reject.

## Ordered Behavior

1. Validate D3 capability, exact products, and complete target rule registry.
2. Assign every node an explicit retain/expand/reject row.
3. Build each required expansion entirely in one private candidate with all
   introduced defs/uses/clobbers visible and total old-result mapping.
4. Recompute affected facts, request fresh C9 projection, and prove every
   non-opaque node directly one-record realizable.
5. Run the full allocation-free Pseudo gate and publish only the complete D4
   revision; the mandatory no-change case still runs every check.

## NodeKind/Tag Lowering Matrix

| D3 planning input | Outcome and closed D4 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| directly realizable generic value/memory/effect/control | retain in corresponding `D4.Realizable*` group | semantic/type/effect/roles/pseudo admission | direct-realizability proof, not free tag | unresolved realizability state | preserve | missing mapping rule rejects |
| `D.ExpansionPlaceholder` or expandable pseudo | expand into complete `D4.Realizable*` sequence | source semantic/effect/type obligations | explicit pseudo defs/uses/clobbers and realizable disposition | placeholder/expansion-required classification | source retires; every output fresh with role/ordinal provenance and total result map | incomplete expansion rejects |
| explicit D2 call transport/call/clobber/preserve | retain only if directly realizable; target expansion by exact D4 rule otherwise | exact D2 requirements/roles/effects | realizable output facts | unresolved target expansion | identity gate; expanded outputs fresh | ABI/call reclassification forbidden |
| pending phi | retain unchanged for D5 | phi/type/predecessor roles/dynamic SSA | none | none | preserve | any SSA removal forbidden in D4 |
| opaque inline asm | retain one opaque node | exact bytes/ordinary bindings/effects | one-opaque-record realizability exception | none | preserve | parsing/expansion forbidden |
| residual placeholder/unknown/illegal/omitted/premature kind | reject publication | none | none | none | no publication | `D4RealizabilityInvalid` |

## Identity and Provenance

Every one-to-many expansion retires the source and gives each output a fresh
ID. The total result map selects exact replacement/projections before RAUW.
Target rule/ordinal is provenance only, never identity.

## Outputs

One allocation-free D4 `PseudoBir` revision containing no expansion placeholder
and one exact-current projection; every non-inline-asm node has a registered
direct one-record realization and every introduced value is ordinary graph state.

## Verification and Publication

Full Pseudo revalidation checks schema/stage, products, CFG/def-use/SSA,
effects, mappings, projection, and realizability. It rejects assignments,
spill/frame/machine facts and any placeholder. Publication is atomic.

## Analysis Preservation and Invalidation

Expansion invalidates all observing graph/SSA/effect/realizability/liveness
facts and prior projection. Recompute under the D4 key; never retag old handles.

## Failure and Diagnostics

Stale products, absent/ambiguous target rule, incomplete mapping, residual
placeholder, projection/SSA/verifier failure, cancellation, or resource
exhaustion publishes no partial expansion or capability.

## Adjacent-Stage Contract

D3 supplies the only input. D5 accepts only this fully reverified revision and
owns all phi/SSA removal. D4-created defs/uses/clobbers must remain visible to D5
and later E1/E2.

## Implementation State

Absent. Prospective target pseudo rules and legacy legalization are not this pass.

## Proof Requirements

Prove mandatory no-op and expansion cases, placeholder elimination, total maps,
fresh projection/full gate, neighboring targets, and no ABI/allocation/frame/MIR.

## Open Questions

Every new target expansion requires a reviewed closed rule and proof family.
