Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Block-Entry Republication Authority Leak

# Current Packet

## Just Finished

Step 1 - Select The Block-Entry Republication Authority Leak: selected the
combined block-entry republication authority leak in
`lower_value_moves`: the `call_plans->calls` walk reconstructs the latest prior
callee-saved `PreservationRepublication` effect per value before the current
block, and `preserved_value_has_block_entry_non_call_use` then scans retained
BIR instructions, terminator, and branch condition spelling to decide whether
the value is needed at block entry before another call.

Existing prepared facts cover only pieces of the decision:
`plan_prepared_call_boundary_effects` can produce a per-call
`PreparedCallBoundaryEffectPlan` for `PreservationRepublication`;
`PreparedCallPlanLookups::prior_preserved_by_value` can answer prior preserved
values relative to a call; value-home and move-bundle data can emit/register
normal block-entry moves. No existing prepared fact owns the block-entry
selection: "for this block entry, these latest dominating callee-saved
preserved values have a non-call use before the first call/terminator/branch
and therefore need republication."

Prepared replacement route for Step 2: add/consume a shared prepared
block-entry preservation republication selection, keyed by block-entry phase
and expressed as prepared boundary-effect data or a directly equivalent
prepared lookup/collection, so AArch64 block-entry lowering consumes prepared
effects and remains emission-only. This selects both local decisions for
removal: the call-plan walk and the BIR-use scan. The surviving
`make_callee_saved_preservation_home_republication_instruction` can remain only
as an emission helper once it receives already-selected prepared effects.

## Suggested Next

Delegate Step 2 to remove the selected local decision by moving the block-entry
preservation republication selection into shared prepared data, then change
AArch64 `lower_value_moves(PreparedMovePhase::BlockEntry)` to consume that
prepared selection instead of walking `call_plans->calls` or calling
`preserved_value_has_block_entry_non_call_use`.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- Missing prepared fact, not an implementation blocker: shared prepared data
  currently lacks a block-entry preservation-republication-needed selection
  derived from dominance/latest-prior preservation plus pre-call non-call use
  within the entry block.
- Treat `PreparedCallBoundaryEffectPlan` as the preferred representation if it
  can carry the selected block-entry effect cleanly; otherwise add the smallest
  shared prepared lookup/collector with equivalent contents.
- Keep AArch64 register, frame-slot, and instruction emission details
  target-local.
- Do not fold in call printing unless this preservation boundary work exposes a
  small direct dependency.

## Proof

Selection-only lifecycle scratchpad update; no build was required and no
`test_after.log` was written.

Evidence commands used:

- `rg -n "lower_value_moves|selected_preservation_effects|preserved_value_has_block_entry_non_call_use|make_callee_saved_preservation_home_republication_instruction|PreparedMovePhase::BlockEntry|find_prior_preserved_value_for_value" src/backend/mir/aarch64/codegen src/backend/prealloc tests/backend/mir`
- `sed -n '3280,3545p' src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `sed -n '140,245p' src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `sed -n '300,370p' src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `sed -n '443,590p' src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `sed -n '1680,1820p' src/backend/prealloc/call_plans.cpp`
- `sed -n '1,145p' src/backend/prealloc/prepared_lookups.hpp`
- `sed -n '330,410p' src/backend/prealloc/value_locations.hpp`

Next Step 2 proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
