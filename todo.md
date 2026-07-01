Status: Active
Source Idea Path: ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Collector Population

# Current Packet

## Just Finished

Step 3 implemented the bounded selected proof-edge path collector/population
packet.

| Area | Step 3 outcome |
| --- | --- |
| Population hook | `BirPreAlloc::publish_contract_plans()` now populates `Function::local_array_selected_proof_edge_paths` after local-array path records and prepared control-flow facts are available. |
| Matching | Collector matches prepared function spelling plus exact `lir_producer_lookup_key`; duplicate keys and multiple available proof candidates fail closed as `unsupported_boundary`. |
| Proof source | Accepts only prepared fused branch compares with predicate/type/lhs/rhs and true/false successors, and only when one compare operand is the dynamic local-array index and the other is an immediate bound. |
| Path facts | Reachability and dominance are computed from the prepared CFG and published as durable record/status fields through `evaluate_local_array_selected_proof_edge_path`. |
| Selection repair | Available proof candidates are tracked separately from non-available fallbacks, so an earlier irrelevant/non-covering branch cannot block a later valid proof source; only multiple available candidates are ambiguous. |
| Covered statuses | Focused tests cover a real available cross-block record, earlier non-available before later available, multiple available ambiguity, plus `missing_same_block_ordering`, `non_covering_path`, `proof_function_mismatch`, `unsupported_lir_producer_role`, and `missing_lir_producer_coordinate`. |
| Preserved boundary | No interval/no-clobber classification, idea 489 proof facts, idea 486 checker inputs, idea 484 packaging, scalar-load consumption, or RV64/MIR lowering were added. |

Artifact: `build/agent_state/493_step3_collector_population/summary.md`.

## Suggested Next

Execute Step 4: residual disposition and close-readiness classification for
idea 493 after the collector population surface landed.

## Watchouts

- Dynamic-index interval effect/no-clobber classification remains a later
  owner; Step 3 intentionally did not classify or consume those facts.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly in
  the residual packet.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate, not
  a prepared/BIR instruction index.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  ordering bridge exists.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 3 validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.

Proof log: `test_after.log`.
