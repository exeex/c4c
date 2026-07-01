Status: Active
Source Idea Path: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Record/Status API Contract

# Current Packet

## Just Finished

Step 2 defined the selected proof-edge path record/status API contract for
dynamic local-array `lir_producer_lookup_key` records.

| Area | Step 2 contract |
| --- | --- |
| Preferred home | Add a sibling record family under `bir::Function` local-array metadata, keyed to `LocalArrayElementPathRecord` and exact `lir_producer_lookup_key`. |
| Ownership boundary | Prealloc/control-flow helpers may compute selected-edge/path facts, but downstream authority begins only after explicit records/statuses are stored. |
| Record fields | Include stable path/key fields, all `lir_producer_*`, proof branch/compare identity, selected outcome, edge tuple, path coverage, dominance/guard validity, same-block unavailable status, and optional bound contribution fields. |
| Status vocabulary | Require visible statuses for available, missing key/path/proof source/edge/outcome, non-covering path, non-dominating proof, unsupported boundary, same-block ordering, coordinate confusion, raw-shape-only, and target/final-home-only. |
| Visibility | Direct backend/BIR tests are sufficient for the first packet; printer exposure is optional display work. |
| Step 3 rule | Implement unless targeted inspection finds a concrete BIR/prealloc ownership or API blocker. |

Artifact: `build/agent_state/492_step2_selected_proof_edge_record_status_contract/contract.md`.

## Suggested Next

Execute Step 3: implement the BIR local-array selected proof-edge path
record/status API with focused backend/BIR tests, or route only if a concrete
ownership/API blocker prevents adding the record home.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Do not treat helper-local reachability/dominance queries as durable proof
  facts unless this runbook publishes explicit records/statuses.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block ordering remains unavailable unless a later bridge truthfully
  relates LIR producer coordinates to the relevant execution coordinate.
- Do not populate idea 489 proof facts or idea 486 checker inputs from this API
  packet; those are later consumers.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
