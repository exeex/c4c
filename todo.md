Status: Active
Source Idea Path: ideas/open/213_route6_call_source_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route-Native Evidence for the Consumer

# Current Packet

## Just Finished

Step 2 added agreement-gated Route 6 evidence for idea 213's selected consumer:
`aarch64::codegen::record_call_result_source_register(...)` primary
call-result source-register publication, limited to the non-`result_lanes_only`
branch.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`

Implementation completed:
- `record_call_result_source_register(...)` now returns a small
  `CallResultSourceRegisterRoute6Evidence` status and accepts an optional
  `bir::Route6CallUseSourceIndex`.
- The new reader uses `bir::route6_find_call_result_source(...)` only after
  confirming a unique Route 6 result record for the selected call key and only
  to validate that the BIR result value identity agrees with the prepared
  destination value home.
- Prepared late-publication data remains authoritative for the actual
  source-register publication: ABI register name/bank/view, destination home,
  occupied registers, result-lane binding, after-call moves, stores, wrapper
  behavior, final records, printer output, and emitted output are not supplied
  or overridden by Route 6.
- Dispatch builds a `Route6CallUseSourceIndex` lazily from the retained BIR
  function and passes it only to the primary non-`result_lanes_only` publication
  call; the later result-lane-only pass remains on the prepared path.
- `backend_aarch64_call_boundary_owner_test` now covers matching Route 6
  agreement plus null index, missing fact, invalid boundary, duplicate/conflict,
  and route/prepared result-value mismatch fallbacks. Every fallback case
  verifies the prepared x0 source-register publication is preserved.

## Suggested Next

Step 3 should independently review that the Route 6 evidence path stays
consumer-local and byte-stable, then decide whether Step 4 should close the
idea or add one more non-x86 proof slice for any supervisor-selected gap.

## Watchouts

- Do not expand this slice into result-lane binding, after-call move lowering,
  stack-result stores, wrapper kind, aggregate transport, printer output, x86,
  or emitted-output changes.
- The evidence status is intentionally diagnostic/test-observable; production
  callers may ignore it because prepared publication remains authoritative.
- The delegated proof intentionally omitted x86 because this build has no
  `backend_x86_route_debug` target/test. x86 route-debug byte-stability remains
  a supervisor follow-up gap, not an executor blocker for this packet.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test backend_aarch64_call_boundary_owner_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepared_printer)$' --output-on-failure | tee test_after.log
```
