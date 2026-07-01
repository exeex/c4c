Status: Active
Source Idea Path: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Selected Proof-Edge Record Homes

# Current Packet

## Just Finished

Step 1 audited candidate homes and helper inputs for a selected proof-edge path
record/status API keyed to dynamic local-array `lir_producer_lookup_key`.

| Area | Step 1 classification |
| --- | --- |
| Preferred record home | `bir::Function` local-array metadata, as a sibling to `local_array_element_paths`. |
| Existing anchor | `LocalArrayElementPathRecord` already carries all `lir_producer_*` fields and `lir_producer_lookup_key`. |
| Proof-source inputs | `PreparedBranchCondition` and fused compare operand producer facts provide branch/compare identity. |
| Helper inputs | Prepared successor, reachability, and dominance helper algorithms exist but are helper-local, not durable facts. |
| Visibility | Existing BIR backend tests directly inspect local-array records; prepared-printer exposure was not found and can remain optional unless Step 2 requires dump visibility. |
| Step 2 readiness | A bounded record/status API contract can be defined now; no lower blocker prevents contract definition. |

Artifact: `build/agent_state/492_step1_selected_proof_edge_record_homes_audit/audit.md`.

## Suggested Next

Execute Step 2: define the selected proof-edge path record/status API contract.
Use the BIR local-array metadata family as the preferred home, require exact
`lir_producer_lookup_key` matching, include proof branch/compare identity,
selected outcome, edge tuple, path coverage, dominance/guard validity, and the
fail-closed status vocabulary.

## Watchouts

- Keep dynamic-index interval effect/no-clobber classification out of this
  runbook; it remains a later owner.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Do not treat helper-local reachability/dominance queries as durable proof
  facts unless this runbook publishes explicit records/statuses.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block ordering remains unavailable unless a later bridge truthfully
  relates LIR producer coordinates to the relevant execution coordinate.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
