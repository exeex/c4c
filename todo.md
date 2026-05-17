Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract calls, variadic, storage, carriers, helpers, control flow, and module

# Current Packet

## Just Finished

Step 4 continued by extracting the runtime helper schema family from
`src/backend/prealloc/prealloc.hpp` into
`src/backend/prealloc/runtime_helpers.hpp`.

`runtime_helpers.hpp` now owns the f128/i128 runtime helper families, kinds,
ABI transitions, result ownership enums, marshal direction/name helpers, helper
binding structs, runtime helper function/container records, and associated
helper ownership/policy records. It includes direct focused dependencies for
prepared names, frame/register placement, value homes, regalloc move
vocabulary, call preservation/clobber records, special carrier vocabulary, and
BIR payload types. `prealloc.hpp` includes it and remains the compatibility
umbrella.

## Suggested Next

Continue Step 4 by extracting the control-flow schema into a focused header
while keeping `prealloc.hpp` as the public compatibility umbrella.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- `names.hpp` intentionally forward-declares `PreparedBirModule`; the concrete
  aggregate should move later with `module.hpp` after member-family headers
  exist.
- `regalloc.hpp` now owns the move-resolution and ABI-binding vocabulary used
  by value locations and later call plans; dependent headers can include it
  directly instead of relying on `prealloc.hpp`.
- Ambiguous boundary: `PreparedStorageEncodingKind` is introduced just before
  call plans but is reused by storage plans and indirect callee plans; extract
  with `calls.hpp` only if later includes stay acyclic, otherwise use the
  lowest shared focused header that avoids recreating a helper monolith.
- Register bank/placement types now live in `frame.hpp`; dependent Step 3
  headers can include that focused header instead of relying on `prealloc.hpp`.
- The string-authority classification for
  `find_prepared_stack_frame_offset_by_name` now follows the helper's current
  declaration in `frame.hpp`; do not reintroduce the stale `prealloc.hpp`
  metadata path.
- Addressing types now live in `addressing.hpp`; dependent Step 3 headers can
  include that focused header instead of relying on `prealloc.hpp`.
- `PreparedStorageEncodingKind` now lives in `calls.hpp`; later storage and
  carrier extraction packets should include that focused header directly if
  they reuse the encoding vocabulary.
- `variadic.hpp` includes `value_locations.hpp` directly for
  `PreparedValueHome`; dependent code should not rely on `prealloc.hpp` for
  that edge.
- `special_carriers.hpp` includes `value_locations.hpp`, which supplies
  `PreparedValueHome` and `PreparedTargetRegisterIdentity` through the focused
  value-location/regalloc dependency chain.
- `runtime_helpers.hpp` includes `calls.hpp` for call preservation/clobber
  records and `special_carriers.hpp` for i128/f128 carrier vocabulary; do not
  route those dependencies back through `prealloc.hpp`.
- Remaining Step 4 families: control-flow and module schema.
- Ambiguous boundary: control-flow helpers include substantial inline analysis
  over BIR and prepared name tables; keep them together initially rather than
  distributing tiny helpers across unrelated family headers.
- The out-of-SSA parallel-copy bridge helpers that need
  `PreparedParallelCopyBundle` still live in `prealloc.hpp` until Step 4 can
  move control-flow and module-level helpers together without a cycle.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.
