Status: Active
Source Idea Path: ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Record/Status API

# Current Packet

## Just Finished

Step 3 implemented the bounded BIR local-array selected proof-edge path
record/status API and repaired its acceptance checks before supervisor
acceptance.

| Area | Step 3 outcome |
| --- | --- |
| Record home | Added `Function::local_array_selected_proof_edge_paths` beside existing local-array metadata. |
| API surface | Added selected proof-edge status/outcome/bound/predicate enums, inputs, record, stringifiers, and evaluator. |
| Available case | Synthetic cross-block record requires exact path/key, proof source, selected outcome/edge tuple, path coverage, and dominance/guard validity. |
| Acceptance repair | The evaluator now rejects proof function mismatch, non-available LIR producer coordinate status, and non-`address_derivation` LIR producer role before a record can become `available`. |
| Fail-closed coverage | Tests cover missing path/key/coordinate, unsupported LIR producer role, proof function mismatch, missing proof source/outcome/edge, same-block ordering, non-covering path, non-guarding proof, coordinate confusion, and raw-shape-only evidence. |
| Boundaries | No interval/no-clobber classification, downstream proof/checker population, packaging, scalar-load, or RV64/MIR behavior changed. |

Artifact: `build/agent_state/492_step3_selected_proof_edge_record_api/summary.md`.

## Suggested Next

Execute Step 4: record residual disposition for idea 492, including whether the
selected proof-edge path certificate work can resume or whether a collector
population/printer packet remains first.

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

Step 3 validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
