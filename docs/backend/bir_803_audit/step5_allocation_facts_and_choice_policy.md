# Idea 803 Step 5: Allocation Facts And Deterministic Choice Policy

Status: normative planned E1/E2/E3 contract; implementation absent

Root placement: [Step 1 matrix](step1_placement_and_conflict_baseline.md)  
E1 authority: [liveness and allocation facts](../../../src/backend/bir/analysis/liveness/README.md)  
E2 authority: [allocation decisions](../../../src/backend/bir/regalloc/README.md)  
E3 authority: [spill/reload realization](../../../src/backend/bir/regalloc/spill_reload/README.md)

## Closed ownership decision

E1 publishes complete immutable `AllocationFactsV1` for one exact revision and
makes no assignment, coalescing, victim, eviction, or profitability choice. E2
owns all such choices under `DeterministicAllocationPolicyV1`. Correctness
predicates are non-negotiable filters; versioned profitability inputs rank only
legal alternatives. E3 realizes exactly one selected `EvictionRequestV1`,
invalidates E1/E2, and returns only to fresh E1.

The versioned keys distinguish factual and decision invalidation:

```text
# Documentation schema sketch; no corresponding landed type is claimed.
E1KeyV1 = (
  graph_revision, target_layout, pool_alias_rules, call_rules,
  projection_key, D5_E3_lineage, profile_revision_or_NoProfileV1,
  AllocationFactSchemaV1
)

E2KeyV1 = (
  E1KeyV1, DeterministicAllocationPolicyV1, finite_tuning_table
)
```

Graph, profile, constraint, pool, call-rule, projection, copy, or spill changes
invalidate E1 and E2. A policy/tuning change invalidates E2 only. E3 mutation
invalidates both products regardless of whether the new graph looks equivalent;
the sole retry edge is `E3 -> E1`.

## Fact-to-decision-to-rewrite trace

For stable value `v17`, E1 records two live segments, use/def frequencies,
loop/profile weight, pressure points, a call crossing, allowed class `GPR`, one
copy affinity to `v09`, a legal rematerialization recipe, ordinary spillability,
and no fixed home. It records interference with `v04`; it records no preference
or victim choice.

E2 first rejects every home aliasing `v04` and every call-clobbered live-across
home. Under the exact V1 integer weights it compares the remaining candidates
in abstract slot order. Coalescing `v17` with `v09` is rejected if their merged
legal domain is empty; copy benefit cannot override that fact. If no legal home
remains, the normalized victim table selects `v17` by the V1 eviction-cost
tuple and stable-ID tie break, then emits one closed request naming its exact
stores, reloads, rewrites, and improving progress witness. The decision trace
records every rejection, score, and tie field.

E3 either realizes that complete request in a private candidate or fails. On
success it creates fresh spill/reload identities, proves the requested progress
improvement, discards E1/E2, and returns to E1 on the new revision. If placement
is impossible, the witness does not improve, the lineage tuple repeats, or a
finite retry bound is reached, the candidate fails; E3 cannot select `v09`,
reweight the cost, retain an old assignment, or jump directly to E2.

## Determinism, bounds, and verification

All enumerations use stable graph/value/copy/home identities. Pointer values,
hash iteration, allocation addresses, incidental traversal, floating point,
randomness, and timing are forbidden decision inputs. Checked arithmetic and
policy-declared maxima cover identities, pools, copy pairs, score operations,
coalescing/assignment attempts, and retries. Overflow and exhaustion fail
closed.

E1 verification requires complete use/def, range, loop/profile, pressure,
fixed/tied/group, call/clobber, rematerialization, copy, spill, scratch, and
exceptional-boundary coverage. E2 verification replays correctness filtering,
stable normalization, scoring, coalescing legality, assignment, victim choice,
ties, and bounds from the exact key. E3 verification proves exact request
realization, no extra decision, strict progress, cycle absence, and full
invalidation. Final E4 validation may check these products but cannot repair or
re-decide them.

## Compatibility note

This is idea 803's register-first baseline. A later initiative may supersede
the policy only through a new explicit version and reconciled owner contract;
it cannot silently reinterpret V1 facts or choices.
