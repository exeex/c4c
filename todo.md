Status: Active
Source Idea Path: ideas/open/179_byval_copy_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fail Closed on Missing or Mismatched Structured Metadata

# Current Packet

## Just Finished

Completed Step 3 for the selected incoming byval parameter materialization route.
Explicit incoming byval params with a `StructNameId` now always use the
structured type-ref lookup path, so missing, stale/mismatched, or opaque
structured metadata fails closed instead of falling back to rendered type text.

Added focused notes-test coverage for ID-bearing incoming byval parameters with
missing structured layout entries, mismatched `StructNameId` entries, and opaque
structured declarations. Kept the no-ID incoming byval compatibility test fenced
as the legacy rendered-text fallback path for routes that do not carry
structured identity metadata.

## Suggested Next

Step 4 packet: review the adjacent incoming-param local-store copy loop in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` against the now-structured
`aggregate_params_` contract, and add guard coverage only if it consumes
aggregate-param layout facts differently from the materialization route.

## Watchouts

- This packet intentionally left direct/indirect byval call-argument lowering
  unchanged; existing call-argument metadata fail-closed tests remain guard
  coverage.
- `declare_local_aggregate_slots` still recurses by layout type spelling after
  the top-level byval param layout has been resolved. Step 3 closes the
  ID-bearing incoming-byval selection gap, but no-ID incoming byval parameters
  intentionally continue through the legacy rendered-text path.
- `clang-format` is not installed in this environment, so formatting was kept
  manual.

## Proof

Ran the supervisor-selected proof exactly:

`set -o pipefail; { cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir' --output-on-failure; } 2>&1 | tee test_after.log`

Result: passed. `test_after.log` contains the green build/no-op and six-test
CTest run.

Also ran the delegated broader validation:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee /tmp/c4c_backend_validation_byval_step3.log`

Result: passed. `/tmp/c4c_backend_validation_byval_step3.log` contains the
green build and `^backend_` CTest run.
