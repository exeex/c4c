Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract calls, variadic, storage, carriers, helpers, control flow, and module

# Current Packet

## Just Finished

Step 4 finished by extracting the module schema and public prepare entry
declarations from
`src/backend/prealloc/prealloc.hpp` into
`src/backend/prealloc/module.hpp`.

`module.hpp` now includes the focused family headers it needs and owns
`PreparedBirModule`, module-level `find_prepared_*` helpers, `BirPreAlloc`,
`infer_call_arg_abi`, stack-layout prepare declarations, and the
`prepare_*_module_with_options` declarations. `prealloc.hpp` now includes
`module.hpp` and remains the public compatibility umbrella.

## Suggested Next

Proceed to Step 5 by tightening includes and proving the decomposed prealloc
headers remain compile-safe and behavior-preserving.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- Step 5 should verify focused headers do not accidentally rely on
  `prealloc.hpp` include order.
- `module.hpp` intentionally centralizes stable prepare entry declarations and
  the concrete aggregate after the focused member-family headers.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.
