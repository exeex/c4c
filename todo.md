Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract frame, addressing, liveness, and regalloc schema

# Current Packet

## Just Finished

Step 3 fixed the baseline-review fallout from moving
`find_prepared_stack_frame_offset_by_name` into
`src/backend/prealloc/frame.hpp`.

`scripts/string_authority_classifications.json` now classifies the exact
current declaration path for the helper without weakening the compatibility
bridge rationale.

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
- The string-authority classification for
  `find_prepared_stack_frame_offset_by_name` now follows the helper's current
  declaration in `frame.hpp`; do not reintroduce the stale `prealloc.hpp`
  metadata path.
- Addressing types now live in `addressing.hpp`; dependent Step 3 headers can
  include that focused header instead of relying on `prealloc.hpp`.
- Ambiguous boundary: control-flow helpers include substantial inline analysis
  over BIR and prepared name tables; keep them together initially rather than
  distributing tiny helpers across unrelated family headers.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '\''^(string_authority_guard|backend_)'\''' > test_after.log 2>&1`

Result: passed. Build completed and 141/141 selected tests passed, including
`string_authority_guard` and backend tests.
Proof log: `test_after.log`.
