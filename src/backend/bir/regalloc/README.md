# E2 Shared Abstract Pseudo-Physical Allocation Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: E2
Upstream: exact E1 product and finite C2 pools
Downstream: immutable assignment candidate, eviction request, or failure for E3

## Purpose

E2 assigns finite abstract `(category, class/group, slot)` homes to every
allocation identity under exact constraints/interference. It is shared across
targets and performs no graph mutation or machine construction.

## Owns

One legality relation, deterministic home selection, ties/groups/fixed homes,
alias-unit exclusion, coalescing decisions, assignment product, bounded
eviction request, validation, and atomic return.

## Does Not Own

E2 does not insert spills/reloads, spill scratch, resolve copies, change graph
or constraints, select concrete machine registers/opcodes, or build frames/MIR.

## Inputs

Exact graph revision, E1 product, C2 finite pools/aliases/reservations, current
C9 projection, C3/C4 call requirements, and D5/E3 lineage.

## Input NodeKind/Tag Vocabulary

E2 observes every allocation-visible identity represented in E1; node admission
is unchanged. Any admitted graph identity absent from E1 is failure.

## Required Analyses and Products

Exact E1, C2, projection, call/asm constraints, D5 scratch rules, and on retry
E3 spill state. No compatible or reconstructed pool/pressure input.

E1's exact `ExceptionalBoundaryAllocationFacts` are correctness constraints,
not profitability hints. E2 rejects an assignment that keeps a value only in a
clobbered register unit across a non-local boundary, coalesces identities across
incompatible boundary states, omits required memory residency, or evicts a
volatile/escaped semantic object into a substitute spill identity. When an
ordinary spillable value needs a pre-boundary store and post-boundary reload,
E2 emits one normal progress-ranked eviction request for E3. It cannot insert
actions, choose frame placement, weaken the obligation, or special-case the
boundary outside versioned policy and stable tie-break rules.

## Ordered Behavior

1. Validate all keys and total E1-to-graph identity coverage.
2. Build legal domains and interference over finite alias units.
3. Assign nonspillable/fixed/group/scratch identities first, then ordinary
   identities deterministically with registered coalescing.
4. Return a complete valid assignment, one progress-ranked ordinary eviction
   request for E3, or structured unsatisfiable failure; never mutate the graph.

## NodeKind/Tag Lowering Matrix

| Allocation input subset | Graph outcome | E2 outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| fixed/tied/group-constrained identity | retain graph | exact legal abstract home satisfying all relations | none | identity keyed assignment | empty domain rejects |
| `CopyScratch` | retain graph | finite assigned home, nonspillable, nonaliasing when simultaneous, disjoint from transfer homes | none | scratch remains explicit | any spill/alias conflict rejects |
| ordinary spillable identity | retain graph | legal home or deterministic progress-ranked eviction request | none | unchanged | no candidate may be ordinary failure/eviction |
| `ParallelCopy`/`EdgeCopy` endpoints | retain graph | endpoint homes/coalescing facts respecting simultaneous semantics | none | endpoints remain distinct | sequential shortcut rejects |
| call/asm clobber-sensitive identity | retain graph | home outside exact live clobber units or fixed as required | none | unchanged | clobber conflict rejects |
| non-local-boundary survivor | retain graph | legal unaffected home or exact memory-residency/eviction decision satisfying E1 | none | boundary identity unchanged | register-only clobbered home or missing route rejects |
| retry spill/reload identity | retain graph | normal assignment/interference treatment | none | exact E3 identity | hidden preassignment rejects |
| unknown/uncovered identity or premature frame/machine input | reject allocation | none | none | none | `AllocationCoverageInvalid` |

## Identity and Provenance

Assignments are exact-key products keyed by stable identities; home equality
does not merge values. Abstract slot IDs are not concrete register spellings.

## Outputs

Exactly one of: complete immutable `AssignmentPlan`; one deterministic ordinary
eviction request with progress witness for E3; or structured failure. No graph.

## Verification and Publication

Validate total assignments, legal domains, alias/interference, ties/groups,
clobbers, scratch nonspillability/nonalias, and exact keys. Partial assignments
never publish or accompany eviction.
Validation includes every exceptional boundary entry and forbids a complete
assignment that leaves any required survivor without a legal home or explicit
E3 realization route.

## Analysis Preservation and Invalidation

Any E1/graph/pool/projection/constraint/spill change invalidates E2. E3 mutation
destroys the assignment candidate and restarts at E1.

## Failure and Diagnostics

Unsatisfiable nonspillable/fixed/group/scratch demand fails immediately.
Ordinary shortage yields only a bounded progress eviction. Cancellation or
resource failure returns no partial assignment.

## Adjacent-Stage Contract

Success proceeds toward stable post-E3 D5 closure. An eviction request goes
only to E3; after E3 mutation the sole edge is `E3 -> E1`, never directly E2.

## Implementation State

Absent. Legacy register allocation is not this shared abstract allocator.

## Proof Requirements

Prove finite pools, alias units, ties/groups/clobbers, scratch nonspillability,
ordinary eviction progress, determinism, stale keys, and no concrete/MIR work.

## Open Questions

New allocation classes require versioned C2/E2 rule rows and proof.
