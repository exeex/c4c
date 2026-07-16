# E1 Allocation Liveness and Interference Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: E1
Upstream: exact initial-D5 publication or exact E3 retry candidate
Downstream: immutable `LivenessInterference` product for E2 and later gates

## Purpose

E1 computes complete allocation liveness, alias-unit interference, simultaneous
copy constraints, scratch intervals, clobbers, and pressure for one exact
post-SSA pseudo revision. It does not assign homes or mutate BIR.

## Owns

Exact allocation-domain value inventory, live ranges, interference/alias-unit
facts, pressure, copy/scratch constraints, product key, validation, and atomic
publication.

## Does Not Own

E1 does not allocate/coalesce, choose spills, insert nodes, resolve copies,
change constraints, spell concrete registers, build frames, or publish a graph.

## Inputs

Either the exact initial D5 allocation-free publication or one exact fully
verified E3 retry candidate, with current C2 pools, C3/C4 call facts, C9
projection, CFG/value-flow/effects, and complete D5 copy/scratch schema.

## Input NodeKind/Tag Vocabulary

Exactly the admitted post-D5 prospective pseudo groups for the given interval,
including every value/def/use, call/asm clobber, `ParallelCopy`, `EdgeCopy`,
`CopyScratch`, and on retry explicit `Spill`/`Reload`. Unknown/omitted kinds fail.

## Required Analyses and Products

Exact CFG, value-flow, memory/effects, current projection, C2 alias units/pools,
call/clobber requirements, D5 edge-copy plan, and E3 spill state when present.

### Exceptional-boundary allocation facts

E1 consumes exact B4/B5 non-local boundary products plus the current C3/C4
target/call rule versions and derives immutable
`ExceptionalBoundaryAllocationFacts` for the same graph revision. Each entry
names the checkpoint occurrence, continuation program point, complete live-in/
live-out identities, exact clobbered alias units, values forbidden from
register-only survival, required memory-resident object identities, and every
required pre-boundary store and post-boundary reload use. It references B3 CFG
facts but introduces no edge, reachability, or dominance authority.

Every identity observable after non-local return is covered exactly once as
unaffected, legally fixed, memory-resident, or explicitly reloaded. A value in
a clobbered unit cannot remain live across the boundary. Volatile and escaped
objects retain their B5 semantic memory identities; E1 may add allocation
constraints but cannot replace them with spill objects. Missing, unknown,
stale, or overlapping classifications reject E1.

## Ordered Behavior

1. Validate the exact revision and all predecessor keys.
2. Enumerate every allocation identity/role in stable graph order.
3. Derive use/def/clobber points, simultaneous-copy semantics, scratch live
   intervals, interference edges, alias-unit occupancy, and pressure.
4. Validate total coverage and atomically publish one immutable E1 product.

## NodeKind/Tag Lowering Matrix

This analysis matrix creates facts only; it does not lower nodes or tags.

| Input subset | Graph outcome | E1 product facts | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| ordinary pseudo value def/use | retain graph | exact live range, allowed requirement/domain, interference | none | stable value/node IDs are keys | missing role/use rejects |
| call/inline-asm clobber/fixed requirement | retain graph | exact blocked alias units, fixed/tied/group constraints | none | unchanged | unbound clobber/constraint rejects |
| non-local-return boundary | retain graph | exact clobbers, live identities, memory-residency and store/reload obligations | none | exact B4/B5 boundary provenance | uncovered or register-only survivor rejects |
| `ParallelCopy`/`EdgeCopy` | retain graph | simultaneous source-before-destination semantics and copy interference | none | transfer endpoint IDs remain distinct | sequentialized inference rejects |
| `CopyScratch` | retain graph | finite live interval, nonspillable/nonalias/disjoint requirement and pressure | none | explicit scratch ID is allocation identity | hidden/unbounded scratch rejects |
| retry `Spill`/`Reload` and spill objects | retain graph | explicit defs/uses/memory effects and pressure changes | none | exact E3 IDs/objects are keys | implicit spill state rejects |
| control/effect/frame-forbidden input | retain admitted control/effect only | CFG/clobber facts as declared | none | unchanged | premature frame/machine kind rejects |
| unknown/illegal/omitted kind or role | reject product | none | none | none | `LivenessCoverageInvalid` |

## Identity and Provenance

E1 never creates semantic identities. Dense indices are product-local and map
back to stable IDs; they cannot escape as allocation or diagnostic authority.

## Outputs

One immutable `LivenessInterference` product keyed by exact graph revision,
target/layout, projection, D5/E3 lineage, algorithms, and total coverage digest.

## Verification and Publication

Validate every admitted def/use/clobber/copy/scratch exactly once, reciprocal
interference, alias-unit consistency, deterministic pressure, and complete keys.
Publish all-or-nothing; no graph capability is created.
For non-local boundaries validation also proves complete exact-key coverage and
that every clobbered live identity is either unavailable after return or has an
explicit memory/reload route exposed to E2/E3/E4.

## Analysis Preservation and Invalidation

Any graph revision, operand/role/order/CFG/effect/copy/scratch/spill/projection/
pool change invalidates E1. A retry always recomputes; no result is retagged.

## Failure and Diagnostics

Coverage hole, stale/mixed product, invalid role/alias unit, resource bound, or
cancellation publishes no partial ranges/pressure and leaves the graph unchanged.

## Adjacent-Stage Contract

E2 consumes only this exact complete product. E3 mutation invalidates it and
returns exclusively to E1; no direct E3-to-E2 reuse or hidden retry exists.

## Implementation State

Absent. Existing liveness/legacy allocation code is not this keyed product.

## Proof Requirements

Prove all roles, loops/edges/calls/asm, simultaneous copies, scratch pressure,
retry spill nodes, stale keys, deterministic bounds, and exact E3-to-E1 return.
Include complete exceptional-boundary classification, exact target/call-rule
keys, clobbered survivors, semantic-memory identity preservation, and missing
store/reload routes.

## Open Questions

New allocation-visible roles require explicit schema and matrix coverage.
