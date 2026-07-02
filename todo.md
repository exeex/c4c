# Current Packet

Status: Complete
Source Idea Path: ideas/open/524_bir_route4_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Route4 Bodies

## Just Finished

Step 2 completed the behavior-preserving extraction into
`src/backend/bir/bir_route4_publication.cpp`. The mapped route4 publication
bodies and route4-private validation helpers now live in that focused route4
owner translation unit.

`src/backend/bir/bir.cpp` retains `route4_record_matches_block` with normal
namespace linkage for route5 plus the retained route-index facade and route6
publication-source selection bodies. The direct-source
`backend_lir_to_bir_notes_test` target now explicitly compiles
`src/backend/bir/bir_route4_publication.cpp` so retained `bir.cpp` facade/route6
callers can resolve the moved route4 definitions without broader target
rewiring.

## Suggested Next

Continue with the next plan packet selected by the supervisor, using the green
Step 2 proof in `test_after.log` as the current extraction baseline.

## Watchouts

- Keep this as behavior-preserving body extraction.
- Do not move public route4 declarations out of `bir.hpp`.
- Do not move route-index facade bodies, route6 source-selection policy, or
  route-specific validation records.
- Do not rewrite expectations or weaken tests as proof.
- Do not make `route4_record_matches_block` route4-only unless route5 is also
  updated to use an equivalent shared helper with unchanged block matching
  semantics.
- `backend_lir_to_bir_notes_test` is the only direct-source BIR test target
  changed in this packet; no test expectations or broad target structure were
  rewritten.

## Proof

Delegated proof passed and is preserved in `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -R '^backend_prepare_frame_stack_call_contract$' --output-on-failure) > test_after.log 2>&1`.

Result: build completed and CTest ran
`backend_prepare_frame_stack_call_contract`, passing 1/1 tests.
