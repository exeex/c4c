# Current Packet

Status: Active
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Route6 Proof

## Just Finished

Step 3: Repair Includes And Internal Linkage.

Verification-only checkpoint completed for include/build registration/linkage
fallout after the route6 call-publication owner split.

The already-passing Step 2 backend build plus `^backend_` proof verified that
the new `src/backend/bir/bir_route6_call_publication.cpp` owner was included in
the core backend build and linked by the standalone BIR notes test source list.
No additional include, registration, or internal-linkage repair was needed in
this checkpoint.

No implementation files, tests, logs, `plan.md`, or source idea files were
edited in this packet.

## Suggested Next

Supervisor can delegate Step 4 from `plan.md`: run the focused route6 proof for
call-publication, call-result, publication-routing, and link-time backend
coverage.

## Watchouts

- Keep this activation aligned to
  `ideas/open/528_bir_route6_call_publication_body_extraction.md`.
- Do not move route6 public declarations, route-index facade bodies, memory
  provenance headers, or implementation bodies outside the route6
  call-publication scope in the first mapping packet.
- Do not change call ABI lowering, LIR-to-BIR call generation, route1-route5
  behavior, or idea 422 producer capability.
- `src/backend/CMakeLists.txt` uses `GLOB_RECURSE` for `src/backend/bir/*.cpp`,
  so no core backend registration edit was needed.
- Step 3 was metadata-only by delegation. The accepted Step 2 backend proof log
  has been rolled forward to canonical `test_before.log`; the next proof packet
  may create a new `test_after.log`.

## Proof

No new command was required or run for Step 3.

Verification basis: the already-recorded Step 2 command
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
passed, and the supervisor regression guard accepted matching before/after
backend logs. The accepted proof log has been rolled forward to canonical
`test_before.log`; the next proof packet may create a new `test_after.log`.
