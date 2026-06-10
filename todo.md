Status: Active
Source Idea Path: ideas/open/170_route4_block_entry_publication_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff
Lifecycle State: Blocked pending broader backend validation decision

# Current Packet

## Just Finished

Plan-owner lifecycle decision completed for Step 5 of the Route 4 block-entry
publication migration.

Completed migration state:
- Step 1 selected `mir::find_bir_block_entry_publication_identity(...)` in
  `src/backend/mir/query.cpp` as the residual semantic consumer.
- Step 2 added selected-consumer MIR negative coverage for missing destination
  keys and destination type mismatch.
- Step 3 migrated the selected MIR helper to Route 4 publication availability
  records and `bir::route4_find_block_entry_publication(...)` for typed
  destination lookup, while preserving fail-closed statuses.
- Step 4 re-scanned the prepared helper surface and found no safe contraction:
  production consumers remain in AArch64 dispatch/publication paths, prepared
  printer, x86 prepared wrapper, and scalar publication planning, with oracle
  tests still using the public helper surface.

Focused Route 4 proof passed 3/3 before the Step 3 commit, and regression
guard with `--allow-non-decreasing-passed` passed.

Lifecycle decision:
- Keep `plan.md` and `todo.md` active and mapped to
  `ideas/open/170_route4_block_entry_publication_migration.md`.
- Do not close or move the source idea while broader backend validation has a
  current-state blocker.
- Do not retire or replace the runbook from plan-owner state alone; the
  blocker is outside the Route 4 migration packet and needs supervisor
  decision or a separate investigation route.

## Suggested Next

The supervisor should decide whether the
`backend_aarch64_instruction_dispatch` failure is an unrelated ambient backend
blocker that can be handled separately, or whether to delegate a focused
investigation before re-running close-level validation.

Closure can be reconsidered only after the close gate has clean enough
regression evidence for the chosen scope. Until then, the active lifecycle
state is blocked, not closed.

## Watchouts

- Broader backend validation command
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_before.log 2>&1`
  failed current-state with `backend_aarch64_instruction_dispatch`.
- Direct rerun
  `ctest --test-dir build --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`
  also failed with `expected selected f64 global readback to feed call ABI move`.
- The Route 4 migration itself has focused green proof, but the active plan
  remains blocked pending supervisor decision unless the supervisor accepts the
  broader backend blocker as unrelated and supplies acceptable close-gate
  evidence.

## Proof

No new test run required in this lifecycle decision packet.

Recorded supervisor validation facts:
- Focused Route 4 proof command
  `(cmake --build build --target backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract)$' --output-on-failure) > test_after.log 2>&1`
  passed 3/3 before the Step 3 commit.
- Regression guard with `--allow-non-decreasing-passed` passed.
- Broader backend validation command
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_before.log 2>&1`
  failed current-state with `backend_aarch64_instruction_dispatch`.
- Direct rerun
  `ctest --test-dir build --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`
  also failed with `expected selected f64 global readback to feed call ABI move`.

Proof log paths were not touched by this packet. Existing focused proof path:
`test_after.log`. Existing broader validation path: `test_before.log`.
