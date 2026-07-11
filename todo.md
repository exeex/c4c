# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Complete the prepared block-entry publication payload

## Just Finished

- Step 3.1 enriched `PreparedCurrentBlockEntryPublication` with owned successor
  label text/ID, destination name text/name ID/value ID/type, and publication
  bundle instruction index, plus explicit missing/unavailable/ambiguous/
  mismatched/incomplete proof statuses.
- The producer now grants `Available` only after complete semantic proof agrees;
  no raw BIR pointer was added to the result authority.
- The active focused block-entry contract now covers the complete positive
  payload and fail-closed no-proof, unavailable, ambiguous, and mismatched
  evidence paths.
- Supervisor review correction moved successor spelling authority to
  `PreparedNameTables` and destination type authority to the uniquely matched
  `PreparedRegallocValue`; pointer-bearing BIR proof now only checks agreement
  with those prepared-owned fields.

## Suggested Next

- Supervisor review of the completed Step 3.1 producer slice, then select the
  bounded Step 3.2 common-adapter compatibility packet.

## Watchouts

- `backend_prepared_lookup_helper_test` remains a separate pre-existing compile
  blocker outside this packet; its source was not touched and it was excluded
  from the corrected focused proof selected by the supervisor.

## Proof

- `cmake --build --preset default --target
  backend_prealloc_block_entry_publications_test && ctest --test-dir build
  --output-on-failure -R '^backend_prealloc_block_entry_publications$' >
  test_after.log 2>&1` passed: 1/1 focused tests, with the real registered test
  recorded in `test_after.log`.
