Status: Active
Source Idea Path: ideas/open/135_shared_current_block_entry_publication_query.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the Shared Query

# Current Packet

## Just Finished

Completed Step 2 shared-query slice. Added
`PreparedCurrentBlockEntryPublicationQueryInputs`,
`PreparedCurrentBlockEntryPublication`, and
`find_prepared_current_block_entry_publication(...)` overloads in
`src/backend/prealloc/prepared_lookups.{hpp,cpp}`. The query resolves either a
prepared value id or a BIR named value through `PreparedValueHomeLookups`, then
reuses `collect_prepared_block_entry_publications` to return the matching
available prepared block-entry publication and destination value-home facts.
It keeps AArch64 register parsing, register-view choice, operand construction,
and emission policy out of shared code.

Extended
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` with
focused coverage for available publication lookup, unsupported destination
storage, missing destination register spelling, missing destination value-home
identity, missing current-block publication, and BIR named-value resolution.

## Suggested Next

Step 3 should replace the local value-home rediscovery plus block-entry
publication scan in
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp:
current_block_entry_publication_register` with the shared query, while keeping
AArch64 register-name parsing, GP bank/view selection, and `RegisterOperand`
construction local.

## Watchouts

- The shared query returns `PublicationUnavailable` with the first matching
  prepared publication when a current-block publication exists but the collector
  classifies it as unsupported or incomplete. Step 3 should preserve the
  current AArch64 behavior of only accepting `Available`.
- The returned `destination_register_name` is still an unparsed prepared
  spelling. AArch64 must continue to parse and coerce it locally.
- `value_has_current_block_entry_publication`,
  `record_current_block_entry_publication_registers`, and
  `block_entry_move_clobbers_current_join_publication` remain separate possible
  follow-on consumers; this packet intentionally did not alter them.

## Proof

Delegated proof command:
`(cmake --build --preset default && ctest --test-dir build -R '^backend_' --output-on-failure) > test_after.log 2>&1`

Proof status: passed. `test_after.log` contains the delegated build plus
backend subset proof: 179/179 backend tests passed.
