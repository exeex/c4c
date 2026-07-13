Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement builders, RawBir publication, and foundation verification

# Current Packet

## Just Finished

- Step 4B completed the `FoundationRaw` verifier for exact epoch/owner/
  generation/kind resolution, storage/order membership, parameters and value
  definitions, function shape, terminators, bounded alternatives, and the
  bidirectionally exact link-name index.
- `publish() &&` now returns structured `PublishFailure` diagnostics, preserves
  an open repairable builder after verification failure, and consumes storage
  into move-only `RawBir` only after zero-error verification.
- Preserved `bir.hpp` as the requested grouped include/declaration reading map;
  no mutable storage or test-only mutation surface was exposed.

## Suggested Next

- Begin Step 5 with the new `bir/lir_to_bir.hpp` public entry and the bounded
  import-spine packet; keep not-yet-migrated semantic forms on explicit
  rejection paths.

## Watchouts

- Foundation deliberately rejects every live ordinary instruction until a
  semantic opcode family and its verifier rules land atomically.
- The default build remains blocked outside this packet because
  `src/backend/backend.hpp` includes missing `bir/lir_to_bir.hpp`; do not route
  around this by compiling legacy BIR or prealloc.

## Proof

- Packet-local C++17 runtime proof passed valid declaration and minimal
  definition publication, active-edit rejection, post-success consumption,
  structured verification failure, repair-and-republish, and reachable
  malformed cases for every foundation diagnostic family. Verifier-private
  mutation existed only in the temporary proof.
- C++17 facade/implementation syntax and negative public probes passed:
  `RawBir` is not default constructible and builder storage remains private.
- `git diff --check` and compile-database `src/backend/legacy/` search passed.
- `cmake --build --preset default -j 2` reached the known external seam and
  failed only because `src/backend/backend.hpp` includes missing
  `bir/lir_to_bir.hpp`.
- The delegated owned-file set excluded logs, so no `test_after.log` was
  written; temporary proof sources, outputs, and executable were removed.
