# B5 / P05 Memory Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B5 / P05
Upstream: exact B4 `SsaCanonical` checkpoint
Downstream: exact B5 `MemoryCanonical` checkpoint for B6

## Purpose

B5 normalizes target-independent memory, address, stack, atomic, fence, and
effect-bearing forms while preserving or re-proving B4 dynamic SSA.

## Owns

Closed access/address/effect normalization, explicit atomic/order forms,
memory-kind replacement, and the `MemoryCanonical` postcondition.

## Does Not Own

B5 does not select target layout/address modes, change CFG, weaken SSA,
canonicalize aggregates/intrinsics, classify ABI, form pseudos, allocate, or
publish `CanonicalBir`.

## Inputs

One exact B4 checkpoint with cumulative properties through `SsaCanonical`,
whole-graph SSA proof, typed memory operands/results, and no later-stage facts.
The B4 checkpoint must carry a verified exact-revision
`CompletePromotionPolicyV1` plan and exact planned/actual accounting. B5 does
not admit a partial-promotion marker, an eligible object retained merely because
B4 exhausted resources, or a half-promoted object with mixed SSA and implicit
memory state. Explicit memory forms are admitted only when the closed B4 matrix
classified them as non-promotable independently of the resource budget.

## Input NodeKind/Tag Vocabulary

Exactly `B4.SsaValue`, `B4.PhiMerge`, `B4.Effect`,
`B4.CanonicalControl`, and `B4.OpaqueToken`, partitioned by the normative
memory/effect/semantic-family axes. Every kind matches one matrix row.

## Required Analyses and Products

Fresh exact-B4 `MemoryEffects` and `Provenance` products plus their declared
CFG/dominance/value-flow dependencies. `Unknown` may prevent an optimization,
but cannot invent alias/provenance or permit an unsafe effect rewrite.

### Non-local memory visibility

B5 consumes every exact-B4 `NonLocalSsaBoundary` and the matching
`MemoryEffects` result. For each boundary it publishes an immutable
`NonLocalMemoryBoundary` keyed by the exact B5 revision. The record names the
checkpoint and continuation, every retained semantic object identity, its
volatile/address-escape/modification status, the memory effects that must be
ordered before the checkpoint, and the post-return accesses that require an
observable memory version. It contains no successor, predecessor, reachability,
dominance, liveness, assignment, or frame fact.

B5 preserves volatile accesses as explicit volatile memory operations and
keeps every address-escaped or otherwise post-return-observable object in
explicit memory form across the boundary. A required store must occur before
the checkpoint; a post-return observation must be an explicit access to the
retained identity, not reuse of stale register-only SSA state. An object marked
`PostReturnIndeterminate` may be accessed only in a source-semantically legal
way; B5 cannot manufacture a stable pre-checkpoint value. Unknown escape,
unknown modification, or incomplete effect coverage fails closed when safety
cannot be proved.

## Ordered Behavior

1. Validate B4 proof and exact analysis keys.
2. Inventory every memory/address/effect form and select one matrix row.
3. Reject malformed ordering, provenance, payload, or unowned kinds.
4. Build a private candidate, perform typed replacements/RAUW, and derive the
   mutation summary.
5. Recompute affected analyses and whole-graph SSA, then atomically advance B5
   or discard the candidate.
6. Recompute and validate all non-local memory-boundary records on that final
   candidate and publish them only under its exact B5 key.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B5 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| non-memory `B4.SsaValue` | retain as `B5.SsaValue` | all six-axis facts and B4 proof applicability | none | none | preserve | mutation forbidden |
| load/address-producing value | retain canonical form or replace with `B5.MemoryValue` | value/type/SSA intent, read/address semantics | canonical memory/address refinement | noncanonical access refinement | identity gate; replaced kind/roles normally fresh | invalid provenance/type rejects |
| store/zero-result memory effect | retain canonical form or replace with `B5.MemoryEffect` | write/read effect, volatility/order, operand roles | canonical memory/effect refinement | noncanonical refinement | preserve only if semantics/roles/effects exact | lost effect rejects |
| atomic/fence form | retain or replace with explicit `B5.AtomicEffect` | exact ordering/scope/read-write/trap facts | canonical atomic/ordering classification | implicit/noncanonical ordering | fresh ID on semantic/role change | unsupported order/scope rejects |
| stack/allocation semantic form still target-independent | retain as `B5.MemoryValue`/`B5.MemoryEffect` by result form | semantic object/type/lifetime/effects | canonical memory form | raw access refinement | normative identity gate | target placement attempt forbidden |
| `B4.PhiMerge` | retain as `B5.PhiMerge` and re-prove SSA if uses change | phi/value/type/predecessor roles | refreshed dynamic proof only | none | preserve unless operand roles change | stale phi mapping rejects |
| non-memory `B4.Effect` | retain as `B5.Effect` | effect/control/shape facts | none | none | preserve | mutation forbidden |
| `B4.CanonicalControl` | retain as `B5.CanonicalControl` | control/successor/effect facts | none | none | preserve | CFG mutation forbidden |
| `B4.OpaqueToken` | retain as `B5.OpaqueToken` with conservative registered effects | opaque payload/ordinary roles/effects | none | none | preserve | parsing text or narrowing unknown effects forbidden |
| unknown, illegal, omitted, import-only, or target-owned kind | reject | none | none | none | no publication | `UnknownMemoryForm` |

## Identity and Provenance

Address/access normalization follows the eight-condition gate. Replacements
use total typed RAUW; expansions get fresh ordered IDs. Provenance analysis is
evidence about values, not permission to reuse identity or infer addresses.

## Outputs

One B5 checkpoint with `B5.SsaValue`, `B5.MemoryValue`, `B5.MemoryEffect`,
`B5.AtomicEffect`, `B5.PhiMerge`, `B5.Effect`, `B5.CanonicalControl`, and
`B5.OpaqueToken`, exact revisions, mutation summary, and refreshed SSA proof.

## Verification and Publication

Verify complete matrix coverage, memory payload/type/role/effect consistency,
atomic ordering/scope, exact def-use, unchanged CFG, and whole-graph SSA after
every replacement. No target/preparation/pseudo/allocation/frame/machine fact
is admitted. The framework advances only the complete green checkpoint.
Each non-local boundary must also have complete retained-object/effect coverage,
explicit pre-checkpoint ordering, legal post-return accesses, and no hidden
register-only observable state.
Admission also proves that the B4 promotion-plan key matches the exact input
revision, that complete selected promotion was verified, and that no resource
fallback marker or incomplete eligible object is present. Failure is rejection,
never B5 repair or implicit demotion.

## Analysis Preservation and Invalidation

Changed accesses, addresses, operands, effects, types, or uses invalidate
MemoryEffects, Provenance, PublicationValueFlow, and dependents. B4 SSA facts
must be recomputed/re-proved for the B5 key; old handles never retag.

## Failure and Diagnostics

Stale analyses, unknown provenance, invalid order/scope, semantic drift,
unknown vocabulary, cancellation, or verifier failure publishes no partial
rewrite, proof, product, or checkpoint.

## Adjacent-Stage Contract

B4 supplies the sole SSA-valid input. B6 receives only this exact B5 checkpoint
and its dynamic SSA proof; target address preparation remains C6.

## Implementation State

Absent. Core memory kinds and MemoryEffects/Provenance design are not this pass.

## Proof Requirements

Prove each access/effect/atomic row, conservative unknown behavior, exact
analysis keys, SSA re-proof, rollback/invalidation, and target-fact exclusion.
Include volatile, escaped, modified and indeterminate objects at a returns-twice
boundary, stale/missing boundary products, and rejection of implicit stores or
loads.

## Open Questions

New memory/effect forms require a reviewed schema entry and explicit row.
