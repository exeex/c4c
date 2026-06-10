Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Semantic Publication Identity Boundary

# Current Packet

## Just Finished

Completed Step 3 for `plan.md`: added BIR block-entry publication identity
request/result records and `find_bir_block_entry_publication_identity`. The
query is keyed by successor block label/id plus destination value identity and
derives availability only from leading BIR PHI results at successor entry.

Added test-only prepared/BIR semantic comparison in
`backend_prealloc_block_entry_publications_test.cpp` and
`backend_prepared_lookup_helper_test.cpp` for available entry publication,
missing/unpublished destination, wrong successor, and wrong destination value
cases. Added call-contract coverage in
`backend_prepare_frame_stack_call_contract_test.cpp` to compare prepared
current-block entry publication destination fields against the BIR PHI-entry
identity.

No consumers were switched. The BIR shape carries only semantic successor and
destination identity fields; it does not import `PreparedBlockEntryPublication`
payload such as hook kind, value-home pointers/kinds, storage encoding,
stack-source policy, register-view conversion, immediate/move payloads,
destination register spelling, emitted-storage availability, or scalar emission
policy.

## Suggested Next

Next packet: execute rewritten Step 4 by tightening the test-only proof around
the semantic boundary. Preserve exact prepared/BIR equivalence only where both
sides answer the same-block source/value/producer identity question.
For block-entry publication, prove PHI-entry destination identity and add
boundary coverage documenting that prepared-only move/home/storage/register
readiness positives remain prepared oracle/fallback behavior, while BIR
PHI-entry positives do not imply prepared emission readiness.

## Watchouts

- BIR entry-publication availability is intentionally PHI-entry semantic
  availability. Exact prepared availability from move/value-home/register facts
  remains outside BIR and must stay prepared-owned.
- Step 4 must not compare prepared hook kind, destination home, storage
  encoding, stack-source policy, register view, immediate/move payload,
  emitted-storage availability, destination register spelling, or scalar
  emission policy as BIR facts.
- Treat prepared-only readiness positives as oracle/fallback behavior, not as
  a missing BIR semantic availability case.
- Leave register parsing plus destination storage decisions in existing
  prepared/AArch64 consumers until a later explicit switch packet.
- Escalate validation if any future packet touches dispatch publication, calls,
  scalar publication planning, value homes, or emission behavior.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test backend_prealloc_block_entry_publications_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_block_entry_publications)' > test_after.log 2>&1
```

`test_after.log`: 3/3 targeted tests passed.
