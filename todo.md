Status: Active
Source Idea Path: ideas/open/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Route 5 Metadata Only Under Prepared Agreement

# Current Packet

## Just Finished

Step 1 - Locate The Row And Agreement Boundary completed as a discovery-only
packet.

Exact selected helper-oracle row:

- File/function: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  `verify_current_block_join_parallel_copy_source_query()`.
- Row: `route5_supported_query.facts[0]`, asserted by
  "current-block join helper row should carry agreeing Route 5 evidence".
- Prepared fact identity: predecessor `current_join.pred`, successor
  `current_join.succ`, destination `%current.destination`
  (`destination_id` 103, `locations.value_homes[1]`, register `x12`, move
  destination register `w12`), source `%current.incoming` (`incoming_id` 101,
  `locations.value_homes[0]`, register `x10`).
- Route 5 identity: PHI in `block.insts[0]` for `%current.destination` has
  incoming `%current.incoming`; Route 5 source producer is the `BinaryInst` at
  `block.insts[3]`, `Route5PublicationSourceKind::Binary`, instruction index 3.
- Eligibility bits for the selected fact: `Available`, non-immediate,
  `source_is_incoming_expression`, `destination_is_source_value`,
  not `source_is_source_value`, and not stack-sourced.

Existing Route 5 agreement boundary:

- `src/backend/prealloc/publication_plans.cpp` owns the fact-level boundary:
  `attach_route5_current_block_join_source_if_agrees(...)` calls
  `bir::route5_find_current_block_join_source(...)`, records the lookup status,
  then accepts only a unique
  `route5_join_source_record_agrees_with_prepared_fact(...)` match.
- The attach guard rejects absent index/block/publication, non-available facts,
  immediate sources, non-incoming-expression sources, non-source-value
  destinations, source-value facts, and stack sources before Route 5 can attach.
- Agreement requires prepared `Available`, Route 5 `Available`, matching
  predecessor/successor labels, destination value name/type, valid matching
  destination home, valid matching source home for named sources, source
  value name/type agreement, and a non-null source producer with an instruction
  index.
- Duplicate/conflicting agreeing records fail closed by setting
  `route5_join_source_status` to `NoMatch` and leaving
  `route5_join_source_agrees == false`.

Existing coverage inventory:

- Positive: `route5_supported_query.facts[0]` carries the agreeing Route 5
  record; `bir::route5_find_current_block_join_source(...)` also finds the
  named `%current.destination` / `%current.incoming` Route 5 row.
- Prepared-only: the original `query.facts[0]` has no Route 5 index and keeps
  `route5_join_source_status == Unavailable`.
- Absent/incomplete Route 5: empty index keeps the prepared fact and reports
  `MissingSuccessor`; indexed BIR fails closed with `MissingPublication`.
- Fallbacks already covered on the selected row: no-source (`NoSource`),
  source mismatch (`MissingSourceValue`), destination type mismatch (`NoMatch`),
  memory-source (`MemorySource`), duplicate agreeing records (`NoMatch`), and
  wrong predecessor (lookup status `Available`, agreement false).
- Nearby same-feature rows: `facts[1]` immediate source and `facts[2]` stack
  source remain prepared-owned; `facts[3]` remains `UnsupportedMove`.
- Broader nearby behavior: `backend_aarch64_current_block_join_routing` covers
  normal, missing-predecessor, mismatched-source, no-source, duplicate-source,
  memory-source, and absent-route routing shapes; `backend_aarch64_instruction_dispatch`
  covers the wrapper/dispatch fallback shape.
- No-change surfaces to keep stable: `backend_prepared_printer`,
  `backend_prealloc_block_entry_publications`,
  `backend_prepare_authoritative_join_ownership`, and x86 joined/handoff
  expected-string surfaces are retained proof targets, not implementation
  targets for this row.

## Suggested Next

Delegate Step 2 as a narrow implementation-or-confirmation packet:

- First confirm whether the active branch already contains the row behavior
  from the previous Route 5 closure. Current discovery found the fact-level
  Route 5 attachment path and positive/fallback helper row already present.
- If implementation is still required, keep code targets to
  `src/backend/prealloc/publication_plans.cpp` and only touch
  `src/backend/prealloc/publication_plans.hpp` if the existing
  `route5_join_source`, `route5_join_source_status`, and
  `route5_join_source_agrees` fields are insufficient.
- Keep tests focused on `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  only if a missing fallback assertion must be added. Treat
  `backend_prepared_printer`, `backend_prealloc_block_entry_publications`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_instruction_dispatch` as no-change proof surfaces unless the
  supervisor explicitly widens the packet.

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
- Step 2 proof gaps to handle explicitly: there is no direct helper-row
  assertion for an invalid Route 5 source-producer pointer/index mismatch, and
  branch/parallel-copy ownership is proven by nearby retained-surface tests
  rather than by the selected helper row itself.
- Wrong-predecessor fallback is subtle: the Route 5 lookup can return
  `Available` for the same successor/destination/source, but the prepared
  agreement boundary rejects it by predecessor-label mismatch and leaves the
  row prepared-owned.

## Proof

Discovery packet proof was `git diff --check -- todo.md && git status --short`;
no new proof log was produced.
