Status: Active
Source Idea Path: ideas/open/521_bir_route8_return_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Apply Minimal Route8 Body Extraction Or Preserve Decision

# Current Packet

## Just Finished

Step 3 - Apply Minimal Route8 Body Extraction Or Preserve Decision is
complete. The selected route8 return-chain bodies now live in
`src/backend/bir/bir_route8.cpp`, and the public route8 declarations and records
remain in `src/backend/bir/bir.hpp`.

Implemented boundary:

- Added private `src/backend/bir/bir_private.hpp` with the narrow
  route-neutral `route_block_matches` helper. It preserves the existing
  id-or-label semantics exactly: compare `BlockLabelId` when either side has a
  valid id, otherwise compare label text.
- Updated route7 code in `bir.cpp` to use `route_block_matches` in the same
  places that previously used the anonymous `route7_block_matches` helper.
- Moved the route8 public definitions and route8-local anonymous helpers into
  `bir_route8.cpp` without changing route8 route1 identity use, status
  selection, duplicate-record handling, or return-chain finder behavior.
- Added `bir_route8.cpp` to the direct-source
  `backend_lir_to_bir_notes_test` metadata; the main `c4c_backend` target
  already picks up BIR sources through the existing glob.

## Suggested Next

Suggested next packet: run the next plan step or supervisor review packet for
the route8 body extraction slice. No implementation blocker remains from Step 3.

## Watchouts

The new helper is intentionally private and route-neutral. Keep route8
declarations in `bir.hpp`; do not widen this into route7 public APIs, facade
coupling, route6 behavior, tests, or expectation rewrites.

## Proof

Proof commands run:

```sh
git diff --check
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: both commands passed. Proof log: `test_after.log`.
