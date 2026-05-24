Status: Active
Source Idea Path: ideas/open/target-neutral-publication-plan-record.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Adapt AArch64 Publication Consumption

# Current Packet

## Just Finished

Completed Step 3: `Adapt AArch64 Publication Consumption`.

Adapted the adjacent join-parallel-copy and
current-block-entry-publication home fallbacks in
`emit_value_publication_to_register` to construct
`prepare::PreparedScalarPublicationPlan` with the source BIR value and prepared
destination home, then consume it through the same narrow AArch64 lowering
adapter used by the no-producer fallback.

The adapter accepts only neutral register-home and stack-slot hooks today. It
checks plan availability, hook kind, storage encoding, and stack-offset facts,
then delegates to the existing AArch64 home emitter so register parsing,
scalar register views, frame-slot memory operand spelling, and instruction text
construction remain target-local.

## Suggested Next

Continue Step 3 by reviewing the remaining direct prepared-home publication
fallbacks, starting with the load-local/load-global reload-sensitive paths, and
only migrate those whose reload and memory-source semantics can stay entirely
target-local.

## Watchouts

The adapter intentionally does not lower `RematerializableImmediate` or
`PointerBasePlusOffset` hooks yet because current behavior for those cases is
target-policy-adjacent. Immediate materialization, scratch choice, recursive
producer lowering, pointer-address construction, global-load sequences,
narrow-store recovery, register alias checks, and machine instruction
construction should stay in AArch64 for this plan. The direct
`current_block_entry_publication_register` fast path and join-publication
clobber check still parse AArch64 registers directly and should remain
target-local unless a later plan introduces a neutral register-identity record.

## Proof

Ran supervisor-selected proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: build succeeded and `157/157` backend tests passed, `0` failed.
Proof log: `test_after.log`.
