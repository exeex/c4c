Status: Active
Source Idea Path: ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Collector Population Contract

# Current Packet

## Just Finished

Step 2 defined the selected proof-edge path record collector/population
contract.

| Area | Step 2 contract |
| --- | --- |
| Accepted representative | Cross-block guard-to-body dynamic local-array GEP with a prepared fused compare on the dynamic index and exactly one selected successor reaching the producer block. |
| Exact matching | Collector must match by prepared function spelling plus exact `lir_producer_lookup_key`; duplicate/ambiguous keys or proof candidates must fail closed. |
| Proof source | Accept only prepared `FusedCompare` branch conditions with predicate/type/lhs/rhs and true/false successors present for the first packet. |
| Path facts | Reachability/path coverage and dominance/guard helper results may be computed from prepared CFG helpers only if published as `local_array_selected_proof_edge_paths` records/statuses. |
| Same-block policy | Same-block proof/producer candidates stay `missing_same_block_ordering`; `lir_producer_instruction_index` remains a LIR producer-site coordinate. |
| Step 3 seam | Implement via a narrow path/dominance helper API or local collector placement plus a mutable prealloc hook that fills `Function::local_array_selected_proof_edge_paths`. |

Artifact: `build/agent_state/493_step2_collector_population_contract/contract.md`.

## Suggested Next

Execute Step 3: implement the bounded collector/population packet, or route
only if a concrete helper/API or mutable-population blocker appears.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Step 3 must publish helper-derived reachability/dominance facts as
  `local_array_selected_proof_edge_paths` records/statuses before downstream
  proof population can consume them.
- Do not make duplicate keys, ambiguous proof candidates, or both-successor
  coverage available; fail closed or route if the status vocabulary is
  insufficient.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  ordering bridge exists.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
