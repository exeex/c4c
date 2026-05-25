Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 - Remove The Selected Local Decision: moved block-entry callee-saved
preservation republication selection into shared prepared call-plan lookups.
`PreparedCallPlanLookups` now owns per-block
`PreparedCallBoundaryEffectPlan` republication selections derived from
dominating prior callee-saved preservation effects plus pre-call non-call use in
the target block.

AArch64 `lower_value_moves(PreparedMovePhase::BlockEntry)` now consumes the
prepared block-entry republication effects through
`indexed_block_entry_republication_effects_for_block`; it no longer walks
`call_plans->calls` to select prior republication effects and no longer calls
`preserved_value_has_block_entry_non_call_use` for this decision. Target-local
code still owns machine emission through
`make_callee_saved_preservation_home_republication_instruction`.

## Suggested Next

Delegate a follow-up review or next implementation packet to confirm whether
the new prepared selection should be surfaced in prepared printing or covered
by a focused prepared-lookup unit test before broader call-emission
consolidation continues.

## Watchouts

- The shared selection intentionally lives in prepared lookups, not target
  emission; AArch64 should keep consuming it rather than recreating the
  call-plan walk or BIR-use scan.
- The selection currently reuses `PreparedCallBoundaryEffectPlan` data but is
  not printed as a durable prepared section.
- No expectation downgrades or call-printing changes were made.

## Proof

Proof passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

`test_after.log` reports 162 backend tests passed, 0 failed.
