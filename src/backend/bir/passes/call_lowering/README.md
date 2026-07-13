# Shared ABI-Aware Call Lowering

Status: converged design contract (unimplemented).

## D2 boundary and sole ownership

`D2` is the one shared ABI-aware BIR call-lowering transaction for every
supported target. Target variation is selected data, not a target-specific
pass implementation: the validated `TargetProfile.backend_abi` selects one
entry from a closed, versioned ABI-rule registry whose descriptor is keyed by
the exact `TargetFingerprint`, verified target-layout schema fingerprint, ABI
rule-set fingerprint, and call-lowering schema fingerprint. An architecture
name, environment switch, backend callback, instruction spelling, or compatible
looking plan cannot select or override a rule.

D2 is the sole owner of turning every `GenericCall`, including an ordinary
call and a D1-created runtime-helper call, into explicit ABI transport. It owns
ordered argument transport, outgoing call-stack stores, hidden sret/byval and
variadic transport, the call operation, result recovery, fixed abstract ABI
slot requirements, declared caller-saved clobbers, and required callee
preservation/restoration. C3 and C4 classify and plan these requirements but do
not mutate BIR; D1 creates only `GenericCall`; D4 may legalize an already
chosen sequence but cannot repeat ABI classification or general call lowering.

## Exact admitted input

D2 consumes one private, frozen, complete D1 candidate and no reconstructed
view. Its input key must bind all of the following:

- the D1 candidate's exact current `PipelineStageStamp`, parent Canonical
  `PipelineStageStamp`, pseudo-schema fingerprint, and D1 lowering-schema and
  occurrence fingerprints;
- the exact `TargetFingerprint` and verified target-layout schema fingerprint;
- the atomic `VerifiedPreparationBundle` fingerprint and its ordered product
  fingerprints, including the exact `RuntimeHelperPlan` used by D1;
- the C3 `AbiPlan` whose key contains that Canonical stamp, target fingerprint,
  target-layout schema fingerprint, ABI-plan schema fingerprint, and no
  predecessor-plan fingerprint;
- the C4 `CallPlan` whose key contains that same Canonical stamp and target,
  the layout and call-plan schema fingerprints, and the exact C3 `AbiPlan`
  fingerprint; and
- the immutable C9 `BoundConstraintSet` fingerprint carried as lineage, the
  exact D1 `ProjectedConstraintSet` keyed to the D1 revision, and the selected
  ABI-rule-set and D2 call-lowering schema fingerprints.

Every `GenericCall` names one live Canonical call identity or one reviewed
runtime-helper interface identity and its exact `CallPlan`/`AbiPlan` entries.
Its ordered ordinary input and result identities, calling convention, tail-call
decision, helper identity when applicable, and hidden-carrier requirements must
agree with those entries. Module-revision equality, stable-ID equality,
semantic hashes, structurally equal products, or products from separate
preparation transactions do not establish freshness. A missing, duplicated,
stale, mixed-target, or mismatched entry rejects D2 before mutation.

## Closed rewrite and output schema

For each `GenericCall`, D2 emits only the pseudo-schema variants below, in the
canonical order required by its selected rule and exact `CallPlan`:

1. `AbiPreserve` operations required at the boundary;
2. ordered `AbiArgMove` operations for register-eligible arguments and hidden
   carriers, plus ordered `AbiArgStore` operations naming abstract outgoing
   stack-object identities for stack-required pieces;
3. exactly one `AbiCall`, carrying the direct/indirect/helper callee identity,
   semantic effects, tail-call decision, abstract caller-clobber units, and
   the plan/rule fingerprints that authorized it;
4. ordered `AbiResultMove` operations for ordinary and hidden-result recovery;
   and
5. matching `AbiRestore` operations when control returns.

`AbiPreserve` and `AbiRestore` are call-site value-transport nodes paired
around that individual `AbiCall`; they preserve live values across its declared
caller-clobber boundary. They are not function entry/exit frame operations.
D2 records abstract function-level callee-save obligations required by the ABI
but never emits saves for them. E4 alone intersects those obligations with
post-allocation used callee-saved units and exact frame placement, then emits
`FrameCalleeSave`/`FrameCalleeRestore`. Final verification requires exactly one
applicable call-site or function-frame coverage record and rejects duplicates.

An empty category is omitted. A proved tail call admits only the closed
tail-call shape selected by the `CallPlan`; it cannot silently degrade to or
from an ordinary returning call. Every introduced operand, result, definition,
use, effect, clobber, requirement, and stack-object reference is ordinary BIR
state. Parallel transport semantics come from the plan's typed ordered pieces;
D2 cannot serialize overlapping transfers in a way that changes those
semantics or hide a temporary in a descriptor or side table.

The output contains no `GenericCall`. It may contain only the closed D1 schema
plus the six D2 families above. Abstract ABI locations are typed layout-owned
`(category, class/group, slot)` requirements or abstract outgoing stack-object
identities. They are fixed ABI requirements, not general assignments. D2 never
spells a concrete register, stack displacement, frame offset, machine opcode,
or encoding; chooses a pressure-driven home; inserts capacity `Spill`/`Reload`;
or creates a target-private call graph. Unsupported target instruction shapes
remain an explicit D4 legalization obligation only after D2 has completed the
one shared semantic ABI rewrite.

Every outgoing-call stack object/store, hidden carrier, call-frame access, and
implicit stack adjustment receives stable abstract identity and complete size,
alignment, lifetime, and access requirements. After allocation and D5
resolution, E4's private frame-action draft is the sole placement authority for
those identities. `FrameActionMaterializationTransaction` emits every required
call/frame action as explicit one-record nodes, and the final frame plan covers
each D2 operation and action. Unrepresentable needs fail atomically before
`MirReadyBirView`; D2 and F1 cannot choose or repair placement.

## Stable identity, revision, and output key

D2 forks one private candidate from the exact D1 input. An unchanged entity
keeps its stable ID. A `GenericCall` may keep its `InstId` only when
`AbiCall` is its unique compatible semantic continuation and its result
identity contract remains valid. Other introduced instructions and results
receive fresh IDs, removed identities become tombstones, and deterministic
old-to-new/tombstone mappings are diagnostic aids rather than freshness or
semantic authority.

Every mutation advances each affected function revision and the module
revision and produces a new complete `PipelineStageStamp`. The frozen output
`PseudoStageKey` contains that exact stamp, the unchanged parent Canonical
stamp, target and layout fingerprints, preparation-bundle and
Canonical `BoundConstraintSet` fingerprint, the exact D2
`ProjectedConstraintSet` fingerprint, pseudo-schema fingerprint, the ordered D1
fingerprints, and one D2 occurrence fingerprint derived from the selected
ABI-rule-set, call-lowering schema, exact `AbiPlan` and `CallPlan` fingerprints,
and deterministic rewrite result. Stable IDs, an equal graph hash, or the D1
stamp cannot substitute for this new key. D2 publishes a private candidate,
not `PseudoBir`.

## Preservation, invalidation, and D3 handoff

Because D2 changes the instruction/value graph, it rebuilds def-use and all
local graph invariants for the new revision. CFG, dominance, SSA, provenance,
publication/value-flow, call-graph, memory-effect, liveness, interference,
assignment, spill, realizability, and other revision-bound products are invalid
unless their owning contract supplies an explicit preservation proof over the
exact D2 mutation summary. Call-boundary products that describe Canonical
requirements remain immutable planning inputs; they are not relabeled as
facts keyed to the D2 revision.

The D1 `ProjectedConstraintSet` is stale after D2 mutation. Before candidate
verification or freeze, the D2 transaction invokes the sole shared
`ConstraintProjectionTransaction` with the immutable C9 root binding, exact
D1 projection, new stamp, D2 occurrence fingerprint, call rewrite map,
tombstones, and complete mutation summary. It must produce one
`ProjectedConstraintSet` keyed to the D2 revision, preserving unchanged
requirements and deriving introduced ABI requirements only from the exact
`AbiPlan`, `CallPlan`, and selected rule. Any projection failure rolls back D2.
Reusing D1's product because an `InstId` survived, records compare equal, or a
mapping was copied is forbidden.

`D3` accepts only the one frozen, complete D2 candidate and exact product set.
Its full cumulative `Pseudo` profile proves that every planned call has exactly
one complete transport sequence, every transport node is plan/rule-derived,
no `GenericCall` remains, every fixed requirement and clobber is explicit, and
all stage/product keys match. Only D3 may atomically mint the first immutable
`PseudoBir`.

## D4 adjacency and forbidden reassignment

`D4` may expand or legalize an explicit D2 call-sequence pseudo when the target
cannot map it one-to-one. Every introduced value remains visible to ordinary
analysis and allocation, and D4 must preserve the selected call plan, ABI-rule
identity, fixed requirements, effects, clobbers, and transport semantics. D4
cannot reconstruct `GenericCall`, select a different `AbiPlan` or `CallPlan`,
reclassify an argument/result, add hidden transport, or become a second shared
call-lowering owner.

## Failure atomicity and legacy disposition

Plan/key mismatch, incomplete call or piece coverage, illegal overlap,
unsupported convention or tail-call shape, unavailable abstract layout slot,
unrepresentable outgoing object, invalid helper interface, stale identity,
schema/rule mismatch, nondeterminism, cancellation, resource exhaustion, or
failed graph/schema checking aborts the whole module D2 transaction. The
transaction rolls back the candidate, replacement map, mutation summary, and
candidate-only derived facts. No function subset, instruction sequence,
revision, property, stage key, cache entry, or fallback D1 revision is
published; the immutable D1
candidate and Canonical/preparation inputs remain unchanged, and D3 receives
nothing.

Legacy `prealloc/calls.hpp`, `call_plans.*`, `formal_publications.*`,
`publication_plans.*`, `variadic_entry_plans.*`, `storage_plans.*`, decoded
homes, special carriers, and call-boundary route records are coverage evidence
only. Their admitted ABI transport behavior is represented by C3/C4 typed
plans and this D2 rewrite. Publication routes, hidden side-record operands,
decoded physical homes, target-private call lowering, and spelling-based
fallback are rejected rather than migrated.
