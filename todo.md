# Current Packet

Status: Complete
Source Idea Path: ideas/open/524_bir_route4_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Final Drift Check

## Just Finished

Step 4 completed the final drift check for the route4 publication body
extraction. The committed range `395b6249f^..HEAD` only moved route4
publication bodies into `src/backend/bir/bir_route4_publication.cpp`, left the
shared `route4_record_matches_block` helper available for route5, and added the
new route4 source file to the focused BIR test target.

No drift was found: the diff did not move public route4 declarations,
route-index facade bodies, route6 source-selection policy, test expectations,
unsupported markers, diagnostics, or route4 record shape/order/optionality.

## Suggested Next

Return to the supervisor for lifecycle/commit handling with Step 4 complete.

## Watchouts

- Keep this as behavior-preserving body extraction.
- Do not move public route4 declarations out of `bir.hpp`.
- Do not move route-index facade bodies, route6 source-selection policy, or
  route-specific validation records.
- Do not rewrite expectations or weaken tests as proof.
- Do not make `route4_record_matches_block` route4-only unless route5 is also
  updated to use an equivalent shared helper with unchanged block matching
  semantics.
- This final drift-check packet made no implementation, tests, CMake,
  `plan.md`, or source-idea edits.

## Proof

No build or CTest rerun was required for this review-only packet. Inspected
`git diff 395b6249f^..HEAD` and used the existing green Step 3 proof preserved
in `test_before.log`:
`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper)$' --output-on-failure) > test_before.log 2>&1`.

Result: build completed and CTest ran
`backend_prepare_frame_stack_call_contract` and
`backend_prepared_lookup_helper`, passing 2/2 tests.
