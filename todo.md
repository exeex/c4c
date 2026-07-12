# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove positive completeness and negative rejection

## Just Finished

- Completed plan Step 3 exact prepared-call lookup integrity: duplicate cursor
  entries now poison the position index, and indexed lookup requires one unique
  call at the requested cursor whose address matches the indexed owner.
- Added dedicated missing, duplicate, stale-pointer, mismatched-owner, and
  mutated-cursor rejection coverage. Strengthened the adjacent direct-extern
  producer fixture to assert exact block/cursor/callee/argument identity and
  identity preservation under unique compatible refinement.

## Suggested Next

- Review the Step 3 slice against the source idea, then route the known later
  baseline failures to their owning initiatives before broader acceptance.

## Watchouts

- The new lookup-helper assertions execute before its existing prepared-MIR
  join-identity failure and pass; no target consumer synthesis or expectation
  downgrade was introduced.
- The joined-branch module contains no calls, and the baseline-isolated runtime
  failure predates commits `80a5388a8` and `0b44f508e`.
- The block-entry failure uses manually assembled publication data and common
  MIR lookup code; its failing test section and `mir/query.cpp` are untouched
  by this packet, and this packet's call-plan index change is disjoint from the
  block-entry publication query.
- Do not absorb either blocker into idea 716. Final closure still requires the
  source idea's focused and broader acceptance contract to be satisfied, so
  unresolved baseline blockers must be handed to their owning initiative or
  otherwise cleared before closure.

## Proof

- Ran the exact delegated command: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^(backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$'
  | tee test_after.log`. Build succeeded and all new Step 3 assertions passed.
  Three pre-existing later baseline failures remain in `test_after.log`:
  prepared-MIR join identity in `backend_prepared_lookup_helper`, block-entry
  publication identity in `backend_prepare_frame_stack_call_contract`, and x86
  joined-branch edge publication in `backend_x86_handoff_boundary`.
