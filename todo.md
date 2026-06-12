Status: Active
Source Idea Path: ideas/open/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Acceptance Notes

# Current Packet

## Just Finished

Step 3 - Prove Fallback And Nearby Same-Feature Stability completed as a
proof-only packet. Existing tests already cover the selected Route 5 agreement
row, the required fallback matrix, and nearby same-feature stability; no
implementation, expected-string, prepared-printer, wrapper, baseline, or test
edits were made.

Focused selected-row proof:

- File/function: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  `verify_current_block_join_parallel_copy_source_query()`.
- Positive selected row: `route5_supported_query.facts[0]` asserts
  "current-block join helper row should carry agreeing Route 5 evidence" for
  `%current.destination` sourced from `%current.incoming` with the Route 5
  `Binary` producer at instruction index 3.
- Nearby rows in the same helper stay prepared-owned:
  `route5_supported_query.facts[1]`, `[2]`, and unsupported `[3]` assert
  "current-block join helper row should keep adjacent facts prepared-owned".

Fallback matrix proof:

- Prepared-only: the original `query.facts[0]` has no Route 5 record and
  `route5_join_source_status == Unavailable`.
- Absent/incomplete Route 5: `empty_route5_join_index` keeps the prepared row
  available with `MissingSuccessor`, while indexed BIR reports
  `MissingPublication`.
- Invalid source evidence: `missing_source_block` and
  `route5_missing_join_index` diagnose `MissingSourceProducer` and preserve that
  status through the Route 5 join index.
- Duplicate/conflicting evidence: `duplicate_route5_join_index` falls back with
  `NoMatch`, and indexed BIR rejects duplicate Route 5 join records.
- Memory-source evidence: `memory_source_route5_join_index` falls back with
  `MemorySource`.
- Mismatch evidence: `source_mismatch_route5_join_index`,
  `destination_type_mismatch_route5_join_index`, indexed wrong destination/type,
  missing source/destination, and `wrong_predecessor_route5_join_index` all
  fail closed while retaining prepared facts.
- Unsupported move: `query.facts[3]` remains
  `PreparedEdgeCopySourceFactsStatus::UnsupportedMove`.
- Branch/parallel-copy and prepared-printer stability:
  `tests/backend/bir/backend_prepared_printer_test.cpp` keeps join-transfer and
  parallel-copy printer rows stable, and
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` keeps
  neighboring block-entry/parallel-copy publication fallback stable.
- Wrapper/dispatch and nearby same-feature stability:
  `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`,
  `verify_current_join_routing(...)`, covers normal, missing-predecessor,
  mismatched-source, no-source, duplicate-source, memory-source, and absent-route
  shapes with prepared fallback; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  retains the same current-block join routing path through
  `current_block_join_query_routing_uses_bir_identity_with_prepared_fallback`.

## Suggested Next

Delegate Step 4 - Validate And Prepare Acceptance Notes. Summarize the selected
row, the retained fallback matrix, same-feature proof, no-change surfaces, and
the fresh `test_after.log` result for supervisor review.

## Watchouts

- Keep source intent unchanged unless durable intent actually changes.
- Do not migrate the adjacent prepared-printer row, prepared-printer output,
  wrapper output, helper-oracle strings, expected strings, baselines, or
  supported/unsupported contracts to claim progress.
- Reject testcase-shaped matching; positive proof must use Route 5
  current-block join-source metadata under prepared edge/join agreement with
  fail-closed fallback.
- Preserve prepared authority for absent, invalid, duplicate/conflicting,
  memory-source, mismatch, unsupported, branch/parallel-copy, and prepared-only
  paths.
- Invalid Route 5 source-producer pointer/index fallback is semantic guard
  coverage rather than a dedicated selected-row mutation assertion; widen tests
  only if the supervisor wants a direct corrupted-index regression row.
- Branch/parallel-copy ownership is proven by nearby retained-surface tests
  rather than by the selected helper row itself.
- Wrong-predecessor fallback is subtle: the Route 5 lookup can return
  `Available` for the same successor/destination/source, but the prepared
  agreement boundary rejects it by predecessor-label mismatch and leaves the
  row prepared-owned.
- Step 3 made no code, expectation, prepared-printer, wrapper, helper-string, or
  baseline changes.

## Proof

Delegated proof passed:

`cmake --build build --target backend_prepared_lookup_helper_test backend_prepared_printer_test backend_prealloc_block_entry_publications_test backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_prealloc_block_entry_publications|backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`

Result: build target was up to date and 5/5 tests passed:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_current_block_join_routing`, `backend_prepared_printer`,
`backend_prepared_lookup_helper`, and
`backend_prealloc_block_entry_publications`.

Proof log path: `test_after.log`.
