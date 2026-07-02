# Current Packet

Status: Active
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Checkpoint

## Just Finished

Step 4: Focused Route6 Proof.

Focused route6 proof completed for the separated route6 call-publication owner.

The proof covered all four delegated focused tests:

- `backend_lir_to_bir_notes`
- `backend_prepared_lookup_helper`
- `backend_prealloc_call_boundary_classification`
- `backend_aarch64_call_boundary_owner`

No implementation files, tests, build registration, `plan.md`, or source idea
files were edited in this validation-only packet.

## Suggested Next

Supervisor can delegate Step 5 from `plan.md`: record the acceptance checkpoint
for the completed route6 call-publication body extraction.

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
- Step 4 overwrote `test_after.log` with the focused proof requested for this
  packet.

## Proof

Ran exactly:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_aarch64_call_boundary_owner)$"' > test_after.log 2>&1`

Result: passed, 4/4 focused tests passed. Proof log: `test_after.log`.
