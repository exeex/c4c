Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Route 2 Consumer

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Inspect Route 2 select-chain/direct-global prepared helper consumers, choose
one bounded AArch64 consumer path, and record the selected prepared fallback,
Route 2 query surface, and proof subset here before implementation.

## Watchouts

Do not accept `test_baseline.new.log` as a baseline. Keep prepared
select-chain/direct-global helpers public, and reject target-policy movement,
broad BIR rescans, name matching, expectation downgrades, or multiple-consumer
migration in one slice.

## Proof

Activation-only lifecycle change; no build or test proof run.
