# Current Packet

Status: Complete
Source Idea Path: ideas/open/524_bir_route4_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Publication Behavior Is Unchanged

## Just Finished

Step 3 completed the delegated publication-behavior proof after the route4 body
extraction. The focused tests `backend_prepare_frame_stack_call_contract` and
`backend_prepared_lookup_helper` both passed, covering the retained route-index
facade behavior plus route6-consuming publication coverage.

## Suggested Next

Continue with the next plan packet or lifecycle action selected by the
supervisor, using the green Step 3 proof in `test_after.log` as the current
publication-behavior proof.

## Watchouts

- Keep this as behavior-preserving body extraction.
- Do not move public route4 declarations out of `bir.hpp`.
- Do not move route-index facade bodies, route6 source-selection policy, or
  route-specific validation records.
- Do not rewrite expectations or weaken tests as proof.
- Do not make `route4_record_matches_block` route4-only unless route5 is also
  updated to use an equivalent shared helper with unchanged block matching
  semantics.
- This was a proof-only packet; no implementation, tests, CMake, `plan.md`, or
  source-idea edits were made.

## Proof

Delegated proof passed and is preserved in `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`.

Result: build completed and CTest ran
`backend_prepare_frame_stack_call_contract` and
`backend_prepared_lookup_helper`, passing 2/2 tests.
