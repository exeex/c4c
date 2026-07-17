# Idea 803 Step 2: Non-local Control-transfer Safety

Status: normative prospective contract; implementation remains absent

Placement baseline: [Step 1](step1_placement_and_conflict_baseline.md)

## Closed owner chain

| Phase | Exact authority or product | Must reject |
| --- | --- | --- |
| B3 | ordinary terminator-derived topology only | any invented non-local edge, predecessor, phi input, or reachability |
| [B4](../../../src/backend/bir/passes/ssa/README.md) | exact-revision `NonLocalSsaBoundary`: checkpoint, continuation point, visible/modified/indeterminate definitions, promotion legality | stale register-only visibility, post-checkpoint-only definition, incomplete checkpoint coverage |
| [B5](../../../src/backend/bir/passes/memory/README.md) and [MemoryEffects](../../../src/backend/bir/analysis/memory_effects/README.md) | exact-revision `NonLocalMemoryBoundary`: retained semantic objects, volatility/escape/modification, ordered effects, legal post-return access | implicit memory transitions, lost volatile/escaped identity, unsafe `Unknown` narrowing |
| [E1](../../../src/backend/bir/analysis/liveness/README.md) | exact target/call-rule-keyed `ExceptionalBoundaryAllocationFacts`: liveness, clobbers, residency, store/reload points | uncovered identity, clobbered register-only survivor, stale predecessor key |
| [E2](../../../src/backend/bir/regalloc/README.md) | assignment obeying E1 or one ordinary progress-ranked eviction request | special-case repair, illegal coalescing, substitute spill for semantic object |
| [E3](../../../src/backend/bir/regalloc/spill_reload/README.md) | explicit pre-checkpoint store and continuation reload, then sole retry to fresh E1 | fabricated CFG, late/partial rewrite, stale continuation use |
| [E4](../../../src/backend/bir/allocated/README.md) | stable addressable home, lifetime, frame placement/actions, exact-current final validation | missing/unstable home, semantic/spill-object conflation, repair or F1 deferral |
| [F1](../../../src/backend/bir/machine_construction/README.md) | apply exactly one registered mapping per verified explicit BIR node/action | expansion, allocation, spill, frame repair, or hidden record insertion |

All prospective product names are documentation vocabulary. They do not claim
landed C++ types, `NodeKind` values, verifier rules, or runtime behavior.

## Exact keys and invalidation

B4 facts bind the exact final B4 graph revision and SSA proof. B5 facts bind the
exact final B5 revision, matching B4 lineage, memory-effects/provenance keys,
and retained object/effect digest. E1 facts additionally bind the current
target, layout, C3/C4 call-rule versions, C9 projection, D5/E3 lineage, and
complete boundary coverage digest. E2 assignment/eviction, E3 spill state, and
E4 frame/realizability products name that lineage and their exact current graph
revision. Equal-looking or predecessor-only products are never accepted.

Any change to call effects, checkpoint/continuation order, definitions, uses,
object volatility/escape/modification, memory effects, target clobbers, graph,
projection, assignment, spill state, frame action, or rule version invalidates
the observing product and every downstream product. E3 mutation has exactly one
successor, fresh E1. E4 has no repair or retry edge.

## Complete scenario trace

Consider a registered `ReturnsTwice` checkpoint with an address-escaped volatile
object `v`, an ordinary spillable value `x` that remains semantically observable,
and a non-volatile automatic `y` modified after the checkpoint whose post-return
value is indeterminate.

1. B3 supplies only the ordinary call continuation and surrounding CFG.
2. B4 publishes one boundary record. It forbids promotion of `v`, classifies
   `x` as requiring a legal continuation value, marks `y` indeterminate, and
   rejects any post-return use of a post-checkpoint-only SSA definition.
3. B5 preserves `v` as explicit volatile memory, orders its required effects,
   retains its semantic object identity, and refuses to turn `y` into a known
   pre-checkpoint value.
4. E1 combines these exact facts with the target/call clobber set. If `x` is in
   a clobbered unit, it exposes one pre-checkpoint store and continuation reload
   obligation; `v` remains a semantic object rather than a spill candidate.
5. E2 either gives `x` an unaffected legal home or emits one ordinary bounded
   eviction request. It cannot declare a clobbered register-only assignment
   complete.
6. E3 realizes the request with explicit store/reload nodes and total use
   rewrites, then discards E1/E2 and returns to fresh E1. It adds no CFG edge.
7. E4 assigns the spill object a stable addressable home whose lifetime spans
   both points, materializes any required bounded frame actions, recomputes E1,
   validates E2/E3 without mutation, and publishes only if no stale register
   continuation remains.
8. F1 applies the verified explicit nodes/actions one-to-one. If a store,
   reload, home, or frame action is missing, F1 rejects rather than repairing.

## Stage-local failure and proof obligations

- B4 fails on missing/duplicate boundary records, ambiguous source
  observability, illegal promotion, fabricated topology, or stale SSA keys.
- B5 fails on lost volatile/address-escaped identity, incomplete effect order,
  implicit memory transitions, illegal indeterminate access, or stale facts.
- E1 fails on incomplete classification or unknown/stale clobber/residency data.
- E2 fails on unsatisfiable nonspillable constraints; an ordinary shortage may
  produce only its existing bounded eviction outcome.
- E3 fails atomically on illegal placement, incomplete RAUW, no progress, retry
  bound/cycle, or any topology change.
- E4 fails atomically on missing lifetime/home/action, incompatible non-local or
  unwind realization, stale lineage, pressure deficit, or any repair request.
- F1 remains strict apply-only and has no recovery edge.

Required future implementation proof covers registered `ReturnsTwice` and
equivalent boundaries without name inference; volatile, escaped, unaffected,
modified, and indeterminate objects; clobbered and unaffected homes; explicit
store/reload placement; retry recomputation; dynamic-stack/frame lifetime;
stale keys; rollback at every stage; and strict F1 rejection. This documentation
step has no runtime proof and does not claim those tests currently exist.
