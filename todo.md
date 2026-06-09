Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Update Consumers And Narrow Includes

# Current Packet

## Just Finished

Step 5 updated direct AArch64 consumers of the moved value-location declarations
without broad residual include cleanup.

`src/backend/mir/aarch64/module/module.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`, and
`src/backend/mir/aarch64/codegen/memory.cpp` now include
`src/backend/prealloc/value_locations.hpp` for moved value-location APIs instead
of relying on `src/backend/prealloc/prepared_lookups.hpp`.

`src/backend/mir/aarch64/module/module.hpp`,
`src/backend/mir/aarch64/codegen/alu.cpp`, and
`src/backend/mir/aarch64/codegen/calls.cpp` now include
`src/backend/prealloc/value_locations.hpp` explicitly while preserving existing
residual prepared-lookup access where the file still needs
`PreparedFunctionLookups`, `make_prepared_function_lookups`, or producer query
helpers.

No behavior or test expectation files changed.

## Suggested Next

Supervisor should decide whether the Step 6 proof already run for this packet is
sufficient for review/commit, or delegate any final lifecycle recording needed
for idea 142.

## Watchouts

- `src/backend/mir/aarch64/codegen/alu.cpp` still needs
  `prepared_lookups.hpp` for `PreparedFunctionLookups` and
  `make_prepared_function_lookups`.
- `src/backend/mir/aarch64/codegen/calls.cpp` still includes
  `prepared_lookups.hpp` for residual producer query helpers while including
  `value_locations.hpp` for moved move-bundle access.
- Broader residual prepared-lookup include cleanup remains deferred to idea 149.

## Proof

Ran `cmake --build --preset default` from the repo root: passed.

Ran `ctest --test-dir build -R '^backend_' --output-on-failure > test_after.log 2>&1`
from the repo root: passed, `100% tests passed, 0 tests failed out of 179`.

Proof log: `test_after.log`.
