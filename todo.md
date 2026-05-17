Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract frame, addressing, liveness, and regalloc schema

# Current Packet

## Just Finished

Step 3 partially extracted the addressing schema family into
`src/backend/prealloc/addressing.hpp`.

`addressing.hpp` now owns prepared address base/materialization/schema records,
memory access records, addressing function aggregate, and associated lookup/name
helpers.

`src/backend/prealloc/prealloc.hpp` now includes `addressing.hpp` and remains
the compatibility umbrella.

## Suggested Next

Continue Step 3 by extracting the liveness schema family into a focused header
while keeping `prealloc.hpp` as the public compatibility umbrella.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- `names.hpp` intentionally forward-declares `PreparedBirModule`; the concrete
  aggregate should move later with `module.hpp` after member-family headers
  exist.
- Step 3 still needs focused extraction for liveness, regalloc, and
  value-location schema.
- Ambiguous boundary: `PreparedStorageEncodingKind` is introduced just before
  call plans but is reused by storage plans and indirect callee plans; extract
  with `calls.hpp` only if later includes stay acyclic, otherwise use the
  lowest shared focused header that avoids recreating a helper monolith.
- Register bank/placement types now live in `frame.hpp`; dependent Step 3
  headers can include that focused header instead of relying on `prealloc.hpp`.
- Addressing types now live in `addressing.hpp`; dependent Step 3 headers can
  include that focused header instead of relying on `prealloc.hpp`.
- Ambiguous boundary: control-flow helpers include substantial inline analysis
  over BIR and prepared name tables; keep them together initially rather than
  distributing tiny helpers across unrelated family headers.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^backend_'\''' > test_after.log 2>&1`

Result: passed. Build completed and 139/139 backend tests passed.
Proof log: `test_after.log`.
