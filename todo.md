Status: Active
Source Idea Path: ideas/open/174_aggregate_abi_classification_structured_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Legacy and Mismatch Behavior Explicit

# Current Packet

## Just Finished

Completed Step 3: made the BIR aggregate byval call-argument boundary explicit
about legacy/no-metadata compatibility and metadata-rich fail-closed behavior
next to the Step 2 classifier.

Implementation notes:
- Named the empty-`arg_type_refs` path as legacy hand-built LIR compatibility in
  `src/backend/bir/lir_to_bir/calling.cpp`; rendered byval text remains
  authoritative only for that no-metadata case.
- Kept nonempty `arg_type_refs` on the structured route and made a known
  rendered byval struct name that disagrees with the `StructNameId` fail closed
  instead of falling back to rendered type parsing.
- Added focused `backend_lir_to_bir_notes_test` coverage for legacy/no-metadata
  byval lowering success, missing `StructNameId` failure, and mismatched
  `StructNameId` failure.

## Suggested Next

Exercise the next coherent packet against producer coverage: inspect or extend
the LIR call-argument mirror producer for fixed byval aggregate arguments, then
prove metadata-rich byval calls arrive with usable `StructNameId` mirrors on
real frontend-produced LIR.

## Watchouts

- Nonempty `arg_type_refs` now fail the direct or indirect call family if an
  aggregate byval argument lacks a usable `StructNameId`, names a known
  different rendered byval struct, or lacks a structured layout. That is
  intentional: metadata-rich calls should not silently fall back to rendered
  type parsing.
- Existing generated fixed byval aggregate calls may still need producer work if
  their `OwnedLirTypedCallArg::type_ref` is empty; this packet only implemented
  the selected BIR boundary route.
- Keep any producer follow-up semantic. Do not add byval-string parsing or
  named-testcase shortcuts.

## Proof

Ran the delegated proof command exactly:

```bash
cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|frontend_lir_call_type_ref|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir)$' --output-on-failure > test_after.log
```

Result: passed. `test_after.log` contains 6/6 passing tests.
