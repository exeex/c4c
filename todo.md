Status: Active
Source Idea Path: ideas/open/213_route6_call_source_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fail-Closed Call-Use Diagnostics

# Current Packet

## Just Finished

Step 3 independently proved the fail-closed Route 6 diagnostics for idea 213's
selected consumer: `record_call_result_source_register(...)` primary
call-result source-register publication.

No additional implementation or test edit was needed. The existing
`backend_aarch64_call_boundary_owner_test` coverage already proves:
- matching Route 6/prepared agreement returns `Agreed` while preserving the
  prepared `x0` call-result source publication
- null Route 6 index and missing result fact return `Fallback` and retain the
  prepared source publication
- invalid call-boundary key and duplicate/conflicting result facts return
  `Fallback`
- route/prepared result-value mismatch returns `Fallback` while retaining the
  prepared home selected by the call plan
- adjacent call-plan behavior remains prepared-owned for ABI register
  selection, byval aggregate transport, stack-result stores, f128 carriers, and
  preservation republication

The delegated printer proof also stayed green, so Step 3 found no remaining
fail-closed implementation or focused AArch64 test gap.

## Suggested Next

Step 4 should prove byte-stable prepared printer and x86 route-debug surfaces
for the selected call-result source consumer, or record the exact unavailable
x86 target/test gap if the current build still cannot run that proof.

## Watchouts

- Step 3 did not add code; only `todo.md` changed.
- Remaining Step 4 gaps are byte-stability proof surfaces: prepared printer is
  included in the current green subset, while x86 route-debug proof was not
  part of this delegated command and remains the named follow-up.
- Keep Route 6 scoped to evidence for this selected source consumer; do not
  expand into result-lane binding, after-call move lowering, stack-result
  stores, wrapper kind, aggregate transport, printer output, x86, or
  emitted-output changes.

## Proof

Passed. Proof log: `test_after.log`. This proof was sufficient for Step 3's
fail-closed diagnostics and adjacent prepared-owned AArch64 behavior; it does
not close the Step 4 x86 route-debug byte-stability gap.

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test backend_aarch64_call_boundary_owner_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepared_printer)$' --output-on-failure | tee test_after.log
```
