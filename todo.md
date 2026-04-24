Status: Active
Source Idea Path: ideas/open/01_bir-memory-header-vocabulary-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Consolidate Shared Layout Projection Helpers

# Current Packet

## Just Finished

Completed `Step 4: Consolidate Shared Layout Projection Helpers` by replacing
the local scalar-array repeated-extent lambda with the shared memory helper
surface. `addressing.cpp` now owns one aggregate extent traversal
implementation with an explicit struct-field-run policy, preserving the
existing broader `find_repeated_aggregate_extent_at_offset` semantics while
adding `find_nested_repeated_aggregate_extent_at_offset` for the narrower
local-slots traversal semantics.

## Suggested Next

Execute `Step 5: Validate Header Budget and Behavior` by confirming only
`memory_types.hpp` and `memory_helpers.hpp` exist as new memory headers,
checking `lowering.hpp` remains the complete private lowerer index, and
reviewing the diff for behavior-preserving scope.

## Watchouts

- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Preserve behavior and testcase expectations.
- Scalar subobject checks and byte-storage reinterpretation policy remain at
  their existing call boundaries where the helper semantics are caller-specific
  or already shared through the existing member declaration.
- The local scalar-array helper intentionally uses the new nested-only extent
  variant; the existing aggregate extent helper still includes contiguous
  struct-field runs for callers that already depended on that broader query.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Passed: backend subset reported 97 passed tests, 0 failed, 12 disabled not run.
Proof log: `test_after.log`.
