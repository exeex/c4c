Status: Active
Source Idea Path: ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add x86 Route 6 diagnostic naming beside compatibility rows

# Current Packet

## Just Finished

Step 3 - Inspected the existing x86 Route 6 scalar call-use diagnostic surface
and confirmed it already satisfies this packet without code changes.

Existing additive rows in `src/backend/mir/x86/debug/debug.cpp` and
`tests/backend/bir/backend_x86_route_debug_test.cpp` keep route-native source
identity diagnostics separately named as `route6 scalar arg status ... status=...
gate=...` beside retained compatibility evidence. The current x86 test surface
already covers:
- positive agreed source identity: `status=available gate=agreed`
- missing relationship fallback: `status=missing_source_relationship gate=blocked`
- missing source id/name: `gate=missing_source_value` and `gate=missing_source_name`
- duplicate relationship fallback: `status=duplicate_relationship gate=blocked`
- prepared/source mismatch: `gate=prepared_source_mismatch` and
  `gate=source_value_mismatch`

The packet is proof-only because adding another assertion would duplicate the
existing route-debug contract instead of expanding coverage. No prepared
compatibility rows, `ConsumedPlans` rows, prepared dump rows, wrapper-output
assertions, or x86 implementation behavior were renamed, hidden, weakened, or
removed.

## Suggested Next

Execute Step 4: add or verify riscv Route 5/Route 3 diagnostic naming beside
retained prepared edge-publication compatibility rows.

## Watchouts

Step 4 should stay riscv-only and preserve prepared fallback strings plus exact
wrapper-visible output. Do not treat the proof-only Step 3 result as permission
to demote prepared compatibility rows.

## Proof

Ran the delegated proof exactly:

`cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' | tee test_after.log`

Result: passed 2/2:
`backend_x86_route_debug` and `backend_prepared_lookup_helper`.

Proof log: `test_after.log`.
