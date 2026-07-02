Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Register-Source Move Evidence

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 from
`plan.md`. No implementation packet has run yet.

## Suggested Next

Execute Step 1: reproduce `src/20010518-1.c` and `src/pr27073.c`, then record
the prepared move-bundle, source-home, destination-home, storage-plan,
cardinality, and rejection-site evidence.

## Watchouts

Do not treat either representative row as a named special case. Keep the
single-move register-source case separate from the multi-move bundle if their
prepared facts imply different owners.

## Proof

Lifecycle-only activation. No build or test proof was required for this
plan-owner packet.
