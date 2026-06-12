Status: Active
Source Idea Path: ideas/open/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fallback And Nearby Same-Feature Stability

# Current Packet

## Just Finished

Step 2 - Consume Route 5 Metadata Only Under Prepared Agreement completed as a
confirmation-only packet; no semantic gap was found and no code/test files were
changed.

Confirmed selected helper-oracle row:

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

Confirmed agreement boundary:

- `attach_route5_current_block_join_source_if_agrees(...)` runs only after the
  prepared fact is `Available` and only for the selected shape: non-immediate,
  incoming-expression source, destination-as-source-value, not source-value
  reuse, and not stack-sourced.
- It records the Route 5 lookup status, then attaches `route5_join_source` and
  sets `route5_join_source_agrees` only when exactly one Route 5 join record
  passes `route5_join_source_record_agrees_with_prepared_fact(...)`.
- Agreement requires Route 5 `Available`, matching predecessor/successor labels,
  matching destination name/type, matching prepared destination/source homes,
  matching named source value/type, and a non-null source producer with an
  instruction index.
- Invalid source-producer evidence is fail-closed: normal Route 5 record
  construction emits `MissingSourceProducer` when a named PHI source has no
  producer, and the prepared attachment predicate rejects any otherwise
  `Available` Route 5 record missing a producer pointer or instruction index.
- Duplicate/conflicting agreeing Route 5 records set the selected row status to
  `NoMatch` and leave `route5_join_source_agrees == false`.

Confirmed fallback/no-change boundaries:

- Positive selected row consumes Route 5 metadata only after prepared agreement:
  `route5_supported_query.facts[0]` carries the agreeing record and the indexed
  Route 5 lookup finds `%current.destination` / `%current.incoming`.
- Prepared-only path remains prepared-owned: the original `query.facts[0]` has
  no Route 5 index, no Route 5 record, and `route5_join_source_status ==
  Unavailable`.
- Absent or incomplete Route 5 remains prepared-owned: empty index reports
  `MissingSuccessor`, indexed BIR reports `MissingPublication`, and the prepared
  row keeps its source/destination homes.
- Invalid Route 5 remains prepared-owned through `MissingSourceProducer` or the
  producer pointer/index agreement guard.
- Duplicate/conflicting, memory-source, source mismatch, destination mismatch,
  wrong-predecessor, unsupported-move, immediate-source, and stack-source paths
  all leave the row or adjacent rows prepared-owned.
- Branch/parallel-copy and wrapper/dispatch behavior remains outside this row's
  ownership and is retained by the delegated no-change proof surfaces.
- Prepared-printer output, wrapper output, helper-oracle strings, expected
  strings, baselines, and prepared-printer/wrapper implementation files were not
  changed.

## Suggested Next

Delegate Step 3 - Prove Fallback And Nearby Same-Feature Stability as a proof
packet. Keep it focused on confirming the existing helper-oracle fallback rows
and nearby retained surfaces rather than editing implementation or expectations.

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

## Proof

Delegated proof passed:

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log 2>&1`

Result: 3/3 tests passed (`backend_prepared_lookup_helper`,
`backend_aarch64_current_block_join_routing`, and
`backend_aarch64_instruction_dispatch`).

Proof log path: `test_after.log`.
