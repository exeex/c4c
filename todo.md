Status: Active
Source Idea Path: ideas/open/135_shared_current_block_entry_publication_query.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace the AArch64 Rediscovery Path

# Current Packet

## Just Finished

Completed Step 3 AArch64 replacement slice. Replaced the local
`current_block_entry_publication_register` value-home rediscovery plus
current-block block-entry publication scan with
`prepare::find_prepared_current_block_entry_publication(...)`.

The AArch64 path now accepts only
`PreparedCurrentBlockEntryPublicationStatus::Available` with an available
publication, a destination home, and a destination register spelling. Register
name parsing, GP-bank filtering, expected-view coercion, and `RegisterOperand`
construction remain local to AArch64. The now-unused
`current_block_entry_publication_home` helper was removed.

## Suggested Next

Step 4 should run the supervisor-selected backend proof review/acceptance path
for the shared current-block entry publication query route, including
regression-log comparison if the supervisor treats this as acceptance-ready.

## Watchouts

- `value_has_current_block_entry_publication`,
  `record_current_block_entry_publication_registers`, and
  `block_entry_move_clobbers_current_join_publication` remain separate possible
  follow-on consumers; this packet intentionally did not alter them.
- `current_block_entry_publication_register` now depends on the shared query's
  named-value overload and still returns `std::nullopt` for unavailable,
  unparseable, or non-GP prepared publications.

## Proof

Delegated proof command:
`(cmake --build --preset default && ctest --test-dir build -R '^backend_' --output-on-failure) > test_after.log 2>&1`

Proof status: passed. `test_after.log` contains the delegated build plus
backend subset proof: 179/179 backend tests passed.
