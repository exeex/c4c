Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract calls, variadic, storage, carriers, helpers, control flow, and module

# Current Packet

## Just Finished

Step 4 continued by extracting the control-flow schema family from
`src/backend/prealloc/prealloc.hpp` into
`src/backend/prealloc/control_flow.hpp`.

`control_flow.hpp` now owns the prepared route enum/name helper,
join/branch/parallel-copy schema, computed value and materialized compare
helper records, control-flow aggregate, inline resolve/find/classification and
render-contract helpers, prepared BIR invariant vocabulary, and the
out-of-SSA parallel-copy bridge helpers that can depend directly on focused
headers. It includes direct focused dependencies for prepared names,
value-location move bundles, and BIR payload types. `prealloc.hpp` includes it
and remains the compatibility umbrella.

## Suggested Next

Continue Step 4 by extracting the module schema and module-level lookup helpers
into a focused header while keeping `prealloc.hpp` as the public compatibility
umbrella.

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
- `control_flow.hpp` includes `value_locations.hpp` directly for the
  out-of-SSA parallel-copy bridge helpers; do not route that edge back through
  `prealloc.hpp`.
- Remaining Step 4 family: module schema.
- Ambiguous boundary: `PreparedBirInvariant` moved with the route/control-flow
  vocabulary because `PreparedBirModule` depends on it, but the concrete
  module aggregate and module-level lookup helpers remain in `prealloc.hpp` for
  the next packet.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.
