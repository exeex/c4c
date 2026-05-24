Status: Active
Source Idea Path: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add The Shared Bookkeeping Helper

# Current Packet

## Just Finished

Step 2 added the selected prealloc block-entry publication bookkeeping helper.

`PreparedBlockEntryPublication` and
`collect_prepared_block_entry_publications(...)` now live with
`PreparedValueLocationFunction` in `src/backend/prealloc/value_locations.hpp`.
The helper filters block-entry `OutOfSsaParallelCopy` bundles by successor
label and returns fact/status records for each matching move.

The returned facts preserve source `PreparedMoveBundle`, source
`PreparedMoveResolution`, value home authority, destination value id/name,
destination role, destination storage kind, and register-name availability.
Statuses cover available publications, unsupported operation/destination
forms, unsupported non-register storage, missing value home, and missing
register name. The helper does not parse target registers, construct MIR
operands, choose scratches, emit machine records, or select diagnostics.

Added direct backend/prealloc coverage in
`backend_prealloc_block_entry_publications` for available explicit-register
publications, value-home register fallback, stack/non-register destinations,
missing home/register facts, wrong phase/authority/label filtering, and source
Prepared record preservation.

## Suggested Next

Step 3 should adapt AArch64 block-entry/publication consumers to use the
prealloc helper for the repeated Prepared move-bundle bookkeeping only.
Keep concrete move emission and target register handling local.

## Watchouts

Do not broaden Step 3 into edge producer walking, select-chain materialization,
target register spelling/parsing, scratch-register selection, machine records,
target operands, or move diagnostics. Those remain target-local.

The helper intentionally reports facts and statuses for target consumers; it
does not silently downgrade unsupported destination/storage forms to fallback
emission.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 155/155 backend tests. Proof log: `test_after.log`.
