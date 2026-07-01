Status: Active
Source Idea Path: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Publish Available Interval Facts From Stored Stream

# Current Packet

## Just Finished

Implemented Step 5 from `plan.md`: BIR functions now have a
`local_array_interval_effects` fact surface, and
`populate_local_array_interval_effects` publishes those facts only by consuming
the selected proof-edge path, its endpoint bridge, and exactly one matching
stored production `local_array_ordered_effect_source_streams` record. Repair
follow-up wired the hook into
`BirPreAlloc::publish_contract_plans()` immediately after ordered effect-source
stream population, with focused structural coverage through
`publish_contract_plans()`.

## Suggested Next

Execute Step 6 from `plan.md`: record the residual disposition for idea 490
path/no-clobber certification after the stored-stream interval facts are now
published.

## Watchouts

- Step 5 intentionally does not populate idea 489 proof facts, idea 486 checker
  inputs, idea 490 certification, packaging, scalar loads, or RV64/MIR lowering.
- `populate_local_array_interval_effects` does not synthesize or populate the
  ordered stream itself; callers must have production stream records present or
  the interval fact remains fail-closed as missing stream evidence.
- The focused production-wiring test clears preseeded rows, calls
  `BirPreAlloc::publish_contract_plans()`, and asserts production publication
  rebuilds selected paths, endpoint bridges, ordered streams, and interval
  facts through the public API.

## Proof

Implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
