# RV64 Object Function Traversal Facade Cleanup

## Goal

Create a small RV64 object-function traversal facade around `prepared_function_to_object_function` after family helpers are peeled out, without changing admission semantics, diagnostics, or traversal order.

## Why This Exists

`prepared_function_to_object_function` is the central high-coupling anchor in RV64 object emission. It should move only after lower-level helper families have stable APIs, and the resulting facade must expose dependencies instead of becoming a second monolith.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_function_emit.hpp`
  - possible new compiled object-route function facade files
- Create a small object-function traversal facade around `prepared_function_to_object_function`.
- Delegate to already-extracted family APIs where available.
- Keep prepared lookup/context dependencies visible in interfaces.

## Out Of Scope

- Moving `fragment_for_prepared_instruction` before family helper APIs exist.
- Changing admission semantics, diagnostics, block traversal order, function name matching, prepared lookup construction, or object bytes.
- RV64 capability repair, gcc_torture expectation changes, unsupported marker changes, target-side inference, or broad function conversion rewrites.

## Acceptance Criteria

- Slices for frame, local/global memory, scalar/select, and call-related helpers are complete or an explicit reviewer-approved waiver documents why this facade is safe earlier.
- The facade reduces central coupling without creating a second all-purpose object file.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'`
- Consider full `^backend_` proof if traversal order or diagnostic aggregation is touched.

## Reviewer Reject Signals

- The diff is mostly mechanical relocation but leaves the same hidden coupling.
- Diagnostics, block traversal order, function admission results, or emitted object behavior change.
- `fragment_for_prepared_instruction` fanout is hidden behind another monolithic dispatcher.
- Unsupported expectations or pass/fail accounting are rewritten to claim progress.
- The facade buries prepared lookup/context dependencies instead of exposing them.
