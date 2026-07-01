Status: Active
Source Idea Path: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Step 4 recorded residual disposition for the BIR local-array selected
proof-edge path record/status API.

| Area | Step 4 disposition |
| --- | --- |
| Implemented surface | Step 3 added `Function::local_array_selected_proof_edge_paths`, selected proof-edge record fields, API-specific statuses/stringifiers, evaluator, and focused available/fail-closed coverage. |
| Close readiness | Idea 492 is close-ready as the independent selected proof-edge path record/status API surface. |
| Remaining first owner | Real collector/population work must produce records from prepared branch/compare facts, selected edge tuples, path coverage, dominance/guard facts, and matching `lir_producer_lookup_key` rows. |
| Printer/display | Optional display-only exposure can follow if needed for probes, but it is not the first semantic blocker. |
| Downstream boundaries | Interval/no-clobber classification, idea 489 proof facts, idea 486 checker input population, idea 484 packaging, scalar-load consumption, and RV64/MIR remain out of scope. |

Artifact: `build/agent_state/492_step4_residual_disposition/disposition.md`.

## Suggested Next

Hand off to lifecycle review/close for idea 492, or activate the next precise
owner: `dynamic local-array selected proof-edge path record collector/population`.

## Watchouts

- The next collector/population owner must publish helper-derived selected
  edge/path/dominance facts as explicit records/statuses before downstream
  proof population consumes them.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate; do
  not reinterpret it as a prepared traversal or BIR instruction index.
- Same-block ordering remains unavailable unless a later bridge truthfully
  relates LIR producer coordinates to the relevant execution coordinate.
- Do not populate idea 489 proof facts or idea 486 checker inputs from this
  API/disposition packet; those are later consumers.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 4 validation:

```sh
git diff --check
```

Result: passed.
