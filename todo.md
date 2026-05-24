Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert AArch64 to Consume the Shared Helper

# Current Packet

## Just Finished

Step 3 in `plan.md` is complete: converted AArch64 to construct and consume
the shared Prepared lookup helper while preserving AArch64 target policy.

Conversion details:

- Removed AArch64-local lookup record ownership from
  `src/backend/mir/aarch64/module/module.hpp`; `FunctionLoweringContext` now
  carries `prepare::PreparedFunctionLookups` plus pointers to the shared
  call-plan, address-materialization, move-bundle, and value-home lookup
  families.
- Removed the duplicate AArch64 lookup builders and position-key helpers from
  `src/backend/mir/aarch64/codegen/traversal.cpp`; each function now builds
  `prepare::make_prepared_function_lookups(...)` and wires its family views
  into the lowering context.
- Routed call-plan lookup through
  `prepare::find_indexed_prepared_call_plan(...)`.
- Routed prior preserved call values and first stack-preserved call values
  through `prepare::find_latest_indexed_prior_preserved_value(...)`,
  `prepare::find_dominating_indexed_prior_preserved_value(...)`, and
  `prepare::first_indexed_stack_preserved_values_for_call(...)`.
- Routed move-bundle lookup through
  `prepare::find_indexed_prepared_move_bundle(...)`.
- Routed address-materialization block lookup through
  `prepare::find_indexed_prepared_address_materializations(...)` and the shared
  fallback collector.
- Routed value-home lookup through the shared `prepare::PreparedValueHomeLookups`
  surface used by the existing indexed value-home helpers.

AArch64 still owns target emission and policy: register parsing/conversion,
ABI lane policy, address-materialization MIR kind selection, GOT/TLS/global
policy checks, frame-pointer/addressing legality, diagnostics, and assembly
printing remain in AArch64 codegen.

## Suggested Next

Proceed to Step 4: name and prove the x86 consumption path for the shared
Prepared lookup helper, using the existing prepared-plan consumption surfaces
instead of copying AArch64 context objects or policy.

## Watchouts

`src/backend/mir/aarch64/codegen/calls.hpp` needed declaration cleanup because
the removed AArch64-local lookup types were exposed there. The remaining
AArch64 fallback scans are for manually constructed contexts without shared
lookup pointers; normal prepared traversal now wires the shared helper for each
function.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. The proof rebuilt the AArch64 codegen conversion and ran 149
matching backend tests with 0 failures. Proof log: `test_after.log`.
