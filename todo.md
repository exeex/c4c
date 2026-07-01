Status: Active
Source Idea Path: ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Collector Inputs And Matching Keys

# Current Packet

## Just Finished

Step 1 audited collector inputs and exact matching keys for real
`local_array_selected_proof_edge_paths` population.

| Area | Step 1 audit result |
| --- | --- |
| Local-array path keys | `LocalArrayElementPathRecord` already carries exact `lir_producer_*` fields, `lir_producer_lookup_key`, coordinate status, role, dynamic indices, and layout facts. |
| Proof source facts | `PreparedBranchCondition` carries function id, block id, fused compare predicate/type/operands, condition value, and true/false successor labels. |
| Path/guard facts | Prepared reachability and dominance helpers exist in `prepared_lookups.cpp`, but are currently helper-local; Step 2 should require publishing their results as records/statuses before downstream consumption. |
| Exact matching | A bounded collector can match by prepared function spelling plus exact `lir_producer_lookup_key`; no fuzzy branch-shape matching is needed. |
| Smallest representative | Cross-block guard-to-body dynamic local-array GEP: branch compare on `%idx`, selected successor reaches producer block, proof dominates/guards producer, and same-block ordering is avoided. |
| Step 2 readiness | Step 2 can define a bounded collector population contract now; no lower semantic owner is needed before contract definition. |

Artifact: `build/agent_state/493_step1_collector_inputs_matching_audit/audit.md`.

## Suggested Next

Execute Step 2: define the collector population contract for cross-block
selected proof-edge path records, including accepted facts, fail-closed
statuses, helper/publication seams, and the exact Step 3 target files/tests.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Step 3 must publish helper-derived reachability/dominance facts as
  `local_array_selected_proof_edge_paths` records/statuses before downstream
  proof population can consume them.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  ordering bridge exists.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
