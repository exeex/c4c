# E3 Explicit Spill/Reload Insertion and Retry Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: E3
Upstream: exact E2 ordinary eviction request and current candidate
Downstream: one fully verified retry candidate returned exclusively to E1

## Purpose

E3 is the sole owner that responds to an ordinary E2 eviction by inserting
explicit prospective `Spill`/`Reload` nodes and abstract spill-object identity.
Every mutation invalidates E1/E2 and returns only to E1.

## Owns

Deterministic spill-object IDs, explicit spill/reload placement and roles,
total result/use mappings, fresh C9 projection, candidate gate, progress metric,
retry bound, and structured termination/failure.

## Does Not Own

E3 does not spill `CopyScratch` or other nonspillable identities, resolve
`ParallelCopy`, preserve E1/E2 across mutation, choose frame offsets/concrete
registers, allocate homes, build frames/MIR, or publish a stage capability.

## Inputs

One exact current candidate, E1 product, E2 eviction request/progress witness,
C2 pools, current projection, D5 copy/scratch plan, and predecessor spill state.

## Input NodeKind/Tag Vocabulary

Exactly the assigned-candidate gate's prospective post-D5 vocabulary, including
unresolved `ParallelCopy`/`CopyScratch` and prior explicit spill/reload on retry.
The requested victim must be an ordinary spillable identity.

## Required Analyses and Products

Exact E1/E2/current projection, CFG/value-flow/effects, D5 edge-copy plan, and
bounded retry state. Missing/stale/compatible-looking inputs reject.

## Ordered Behavior

1. Validate exact keys, victim spillability, progress witness, and retry bound.
2. Allocate one deterministic abstract spill-object ID and plan all required
   stores/reloads/RAUW without touching scratch or parallel-copy semantics.
3. Build one private candidate with explicit nodes and total mappings.
4. Verify graph/copy/scratch rules, request fresh C9 projection, and atomically
   stage the retry candidate.
5. Discard all E1/E2 facts and return exclusively to fresh E1 computation.

## NodeKind/Tag Lowering Matrix

| E3 input subset | Outcome | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| selected ordinary spillable definition/value | retain producer as applicable; insert explicit spill and reloads, rewrite dominated uses | type/value/effect obligations | prospective spill/reload memory/effect/def-use roles and spill-object reference | direct unspilled use at rewritten sites | spill/reload/object IDs fresh; total typed use map | incomplete placement rejects |
| prior spill/reload/object on retry | retain exactly unless current victim rule explicitly adds separate actions | roles/effects/object identity | none | none | preserve | hidden coalescing/deletion forbidden |
| `ParallelCopy`/`EdgeCopy` | retain unresolved semantics | transfer/endpoints/edge provenance | none | none | preserve | early resolution forbidden |
| `CopyScratch` | retain unchanged | nonspillable/nonalias requirement | none | none | preserve | selection as victim is hard failure |
| call/asm clobber/fixed/group identity | retain; spill only if E2 request proves ordinary spillability under constraints | exact roles/constraints | explicit spill actions if legal | none | fresh actions, identity unchanged | illegal victim rejects |
| control/effect nodes | retain except explicit insertion points | control/effects | spill/reload effects only | none | existing IDs preserved | CFG semantic change rejects |
| unknown/illegal/omitted/premature frame/machine kind | reject candidate | none | none | none | no staging | `SpillRewriteCoverageInvalid` |

## Identity and Provenance

Every spill object/action gets a fresh stable ID. Reload results are distinct
value identities; rewriting uses does not merge identities. Frame placement is
not encoded. All source/victim/use relationships are explicit provenance.

## Outputs

One private fully verified retry revision with explicit spill state, fresh
projection, mutation summary, increased progress metric, and unchanged explicit
parallel-copy/scratch semantics. It carries no current E1/E2 product.

## Verification and Publication

The private assigned-candidate gate checks exact victim/progress, spill object/
action coverage, types/effects/def-use/CFG, copy/scratch preservation, no scratch
spill, current projection, and no concrete frame/machine facts. No public stage
is minted.

## Analysis Preservation and Invalidation

Every E3 mutation invalidates E1, E2, value-flow, effects, liveness, projection,
and dependent products. Fresh projection is candidate lineage only; next work is
fresh E1, followed by new E2.

## Failure and Diagnostics

Nonspillable victim, no legal placement, missing progress, repeated victim/
state cycle, bound exhaustion, stale products, projection/verifier failure,
cancellation, or resource exhaustion stages nothing and terminates explicitly.

## Adjacent-Stage Contract

E2 supplies only a progress-ranked ordinary eviction request. Successful E3 has
exactly one successor: E1 on the new revision. It cannot jump to E2, D5 copy
resolution, or E4. When E2 later succeeds without eviction, stable post-E3 D5
closure consumes the fresh exact products.

## Implementation State

Absent. Legacy spill code does not implement this explicit bounded retry.

## Proof Requirements

Prove ordinary victim insertion/RAUW, scratch/nonspillable rejection, copy
preservation, progress monotonicity/bound/cycle failure, fresh projection, and
the sole `E3 -> E1` edge.

## Open Questions

Retry bound/progress metric must be versioned before implementation; it may not
be an unbounded heuristic loop.
