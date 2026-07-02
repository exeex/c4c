# Current Packet

Status: Complete - ready for plan-owner lifecycle review
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Checkpoint

## Just Finished

Step 5: Acceptance Checkpoint.

Acceptance checkpoint recorded for the completed route6 call-publication body
extraction.

Final diff confirmation:

- Only the mapped route6 call-use, call-argument publication, call-result
  source, and publication-routing bodies were moved into
  `src/backend/bir/bir_route6_call_publication.cpp`.
- Route6 public declarations and public route6 types stayed in `bir.hpp`.
- Route-index facade bodies stayed in `bir.cpp`.
- Memory provenance and memory lowering surfaces stayed in place.
- Call ABI lowering, LIR-to-BIR call generation, route1-route5 owner behavior,
  and idea 422 producer behavior did not change.

Accepted proof coverage:

- Step 2 backend proof: build plus `^backend_` test subset passed, and the
  supervisor regression guard accepted matching before/after backend logs.
- Step 4 focused proof: build plus the four focused tests passed
  (`backend_lir_to_bir_notes`, `backend_prepared_lookup_helper`,
  `backend_prealloc_call_boundary_classification`,
  `backend_aarch64_call_boundary_owner`), and the supervisor regression guard
  accepted matching before/after focused logs.
- Accepted proof logs were rolled forward to canonical `test_before.log`.

No implementation files, tests, logs, `plan.md`, or source idea files were
edited in this acceptance-summary packet.

## Suggested Next

Ready for plan-owner lifecycle review of the completed active runbook. No
additional executor implementation packet is requested.

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
- Accepted proof logs were rolled forward to canonical `test_before.log`; this
  acceptance packet did not inspect or mutate logs.

## Proof

No new command was required or run for Step 5.

Acceptance basis: the committed Step 2 backend proof/regression guard and the
committed Step 4 focused four-test proof/regression guard were already accepted
by the supervisor and rolled forward to canonical `test_before.log`.
