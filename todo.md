Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add a Target-Neutral Prepared Lookup Helper

# Current Packet

## Just Finished

Step 2 in `plan.md` is complete: added the target-neutral read-only Prepared
lookup helper in `src/backend/prealloc/prepared_lookups.hpp` and
`src/backend/prealloc/prepared_lookups.cpp`, exported through
`src/backend/prealloc/module.hpp`.

Helper API added:

- `PreparedCallPlanLookups`, `PreparedAddressMaterializationLookups`,
  `PreparedMoveBundleLookups`, `PreparedValueHomeLookups`, and aggregate
  `PreparedFunctionLookups`.
- `make_prepared_call_plan_lookups`,
  `make_prepared_address_materialization_lookups`,
  `make_prepared_move_bundle_lookups`,
  `make_prepared_value_home_lookups`, and
  `make_prepared_function_lookups`.
- Read-only accessors for indexed call plans, address-materialization groups,
  move bundles, prior preserved call values, first stack-preserved call values,
  and value-home maps compatible with the existing indexed value-home helpers.
- Target-neutral position-key helpers for call plans and move bundles plus a
  prepared prior-preserved entry ordering helper.

Target-neutrality evidence:

- The helper depends only on `PreparedBirModule`, `PreparedCallPlansFunction`,
  `PreparedControlFlowFunction`, `PreparedAddressingFunction`, and
  `PreparedValueLocationFunction` facts from `src/backend/prealloc`.
- The call-plan side index copies only Prepared control-flow dominance and
  preserved-value reachability rules; it does not mention AArch64 registers,
  ABI classification, instruction records, diagnostics, or printing.
- Address materialization lookup groups existing Prepared materialization facts
  by block label and leaves MIR kind selection, GOT/TLS/global policy checks,
  relocation spelling, and target emission to consumers.
- Move-bundle and value-home lookup surfaces key by Prepared phase/position,
  `PreparedValueId`, and durable `ValueNameId`; they do not lower moves or
  parse target register spellings.

## Suggested Next

Proceed to Step 3: convert AArch64 to construct and consume
`prepare::PreparedFunctionLookups`/family-specific lookup helpers while keeping
all target emission and policy in AArch64.

## Watchouts

AArch64 still owns its local index structs and builders; this packet only added
the shared helper and did not convert consumers. During Step 3, use the new
prealloc types instead of adding wrappers around the AArch64 map names, and do
not move register spelling, AAPCS64 policy, address-materialization MIR kind
selection, GOT/TLS/global policy checks, frame-pointer/addressing legality,
diagnostics, or assembly printing out of AArch64.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. The proof built `src/backend/prealloc/prepared_lookups.cpp` and
ran 149 matching backend tests with 0 failures. Proof log:
`test_after.log`.
