Status: Active
Source Idea Path: ideas/open/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select The Next Producer Or Consumer Packet

# Current Packet

## Just Finished

Step 2 selected the next owner from the Step 1 refreshed semantic-admission
classification. The selected next packet is a BIR semantic `gep local-memory`
admission producer packet for the `62`-row family represented by
`src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, and
`src/20000717-4.c`. Evidence is recorded in
`build/agent_state/496_step2_next_owner_selection/`.

## Suggested Next

Ask the plan owner to create or activate a focused semantic GEP local-memory
producer idea, or to revise the active 496 runbook if that lifecycle route is
preferred. No existing open idea is a clean, unblocked more-specific match for
this packet.

## Watchouts

- Consumers should use `local_array_scalar_local_loads`; do not re-derive local
  object, element offset, layout, range, provenance, exact-address checks, or
  scalar load identity from lower surfaces, final homes, raw testcase shape,
  names, or target behavior.
- The selected `gep local-memory` route is a producer packet, not a consumer of
  `local_array_scalar_local_loads`.
- `ideas/open/442_pointer_value_memory_provenance_publication.md` is related but
  not selected: it is parked on missing closed-world formal pointer authority
  and is scoped to runtime pointer dereference provenance, not semantic local
  GEP admission.
- The full `79`-row load local-memory family is not solved as a group:
  representative local-array element-load rows can now consume scalar-load facts,
  while aggregate/member, variadic, global, pointer-derived, and integer-pointer
  round-trip rows remain producer-owned boundaries.
- If a remaining failure lacks producer authority, route to a producer-owned idea
  before RV64/MIR lowering.
- Do not fold move-bundle materialization, F128, runtime mismatch, or global
  stack-frame work into this packet unless refreshed bucket evidence selects
  that owner.

## Proof

Step 2 routing proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
