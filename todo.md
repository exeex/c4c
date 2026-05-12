Status: Active
Source Idea Path: ideas/open/179_byval_copy_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate and Summarize

# Current Packet

## Just Finished

Completed Step 2 implementation for the selected incoming byval parameter
materialization route. `collect_aggregate_params` now uses
`function_.signature_param_type_refs[index]` and
`lookup_backend_aggregate_type_ref_layout_result` only when an explicit incoming
byval parameter has a structured `StructNameId`; no-ID incoming byval params and
ABI-generated/direct aggregate params keep the rendered-text layout fallback.
`materialize_aggregate_param_aliases` rejects invalid aggregate-param layouts
before declaring local slots.

Added focused notes-test coverage for incoming byval params: matching
`StructNameId` metadata materializes leaf-slot copies from the pointer param,
and no-ID incoming byval params preserve the legacy layout path so generated
direct aggregate/byval ABI routes such as `00204.c::fa_s17` do not regress.

## Suggested Next

Step 3 packet: review the adjacent incoming-param local-store copy loop in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` against the now-structured
`aggregate_params_` contract, and add guard coverage only if it consumes
aggregate-param layout facts differently from the materialization route.

## Watchouts

- This packet intentionally left direct/indirect byval call-argument lowering
  unchanged; existing call-argument metadata fail-closed tests remain guard
  coverage.
- `declare_local_aggregate_slots` still recurses by layout type spelling after
  the top-level byval param layout has been resolved. The invalid-layout guard
  now applies only after an ID-bearing structured lookup fails, not to legacy
  fallback aggregate params.
- `clang-format` is not installed in this environment, so formatting was kept
  manual.

## Proof

Ran the supervisor-selected proof exactly:

`set -o pipefail; { cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes|backend_codegen_route_x86_64_(byval_member_array_params|aggregate_param_return_pair|indirect_aggregate_param_return_pair|aggregate_param_return_pair_fn_param)_observe_semantic_bir' --output-on-failure; } 2>&1 | tee test_after.log`

Result: passed. `test_after.log` contains the green build/no-op and six-test
CTest run.

Also ran the delegated broader validation:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee /tmp/c4c_backend_validation_byval_step2_fix.log`

Result: passed. `/tmp/c4c_backend_validation_byval_step2_fix.log` contains the
green build and `^backend_` CTest run.
