Status: Active
Source Idea Path: ideas/open/497_dynamic_local_array_lir_producer_endpoint_bridge_effect_scan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Stored Ordered Effect Stream

# Current Packet

## Just Finished

Step 4: Consume Stored Ordered Effect Stream is implemented. The interval
effect consumer now treats function-level
`local_array_ordered_effect_source_streams` lookup as the stored-stream
authority, requires exactly one matching selected-path and endpoint-bridge
record, and reports duplicate matching streams as a distinct fail-closed
status instead of collapsing them into missing evidence.

## Suggested Next

Execute Step 5 from `plan.md`: write the residual disposition and decide
whether idea 494 can resume available interval fact publication, or whether
another lower endpoint/effect owner remains first.

## Watchouts

- Step 4 still only produces lower endpoint/effect evidence. Keep downstream
  proof-fact publication, checker input population, packaging, scalar loads,
  and RV64/MIR lowering out of this plan.
- The stored stream must stay production-populated; do not reintroduce
  caller-supplied coverage booleans, caller-supplied effect vectors, selected
  path availability, synthetic path-only records, or the legacy
  `endpoint_bridge_available` flag as availability evidence.
- Step 5 should preserve the current fail-closed representatives: empty or
  missing stream, synthetic/path-only evidence, duplicate stream, missing
  endpoint, clobber, alias/phi, unknown effect, missing coordinate, unordered
  boundary, and coordinate confusion.

## Proof

Step 4 code proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
