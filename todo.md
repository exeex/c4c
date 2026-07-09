Status: Active
Source Idea Path: ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Fan-In Evidence And Select Contract

# Current Packet

## Just Finished

Step 1 refreshed the six idea-630 spillover rows with the supervisor-selected
allowlist probe and confirmed the current first owner is still missing
prepared/prealloc stack-destination fan-in authority, not string-constant
local-memory admission or source freshness:

- `src/20011109-2.c`: `rv64_prepared_move_bundle_consumer` reports
  `stack-destination register fan-in requires explicit prepared
  stack_destination_fan_in_authority fact`, `authority=none`,
  `move_count=3`, and
  `fragment_status=missing_stack_destination_fan_in_authority_fact`.
- `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, and `src/ptr-arith-1.c`: the detailed first owner is
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  with two register sources to one stack-slot destination, `authority=none`,
  `parallel_copy=no`, `diagnostic_owner=rv64_prepared_move_bundle_consumer`,
  and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Selected contract: semantic merge authority, using the existing prepared
select-materialization family:
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` plus
`PreparedStackDestinationFanInSemantics::SelectMaterializationPreservedStackFallback`.
This is the best first producer family because the refreshed rows have no
predicate/edge metadata for mutual-exclusion authority, ordered final-state
authority would be unsafe without explicit final-state proof, and the current
prepared/RV64 surface already has a legal select-materialized fan-in fact plus
fail-closed missing-fact diagnostics.

## Suggested Next

Start Step 2 by publishing only the selected semantic-merge/select-materialized
destination authority from prepared/prealloc. Legal proof shape: a
select-materialized stack-destination fan-in bundle whose producer proves the
select result and preserved stack fallback, then marks the bundle and every
participating move as `StackDestinationRegisterFanIn` and exposes a
`stack_destination_fan_in_authority` fact with
`select_materialization_preserved_stack_fallback` semantics. Negative proof
shape: the same select-shaped fan-in before publication, or with bundle/move
authority facts removed or mismatched, must remain rejected with
`authority=none`, `missing_stack_destination_fan_in_authority_fact`, or the
existing producer-authority-missing diagnostic.

## Watchouts

Keep this producer-side: do not implement new RV64 materialization, expectation
rewrites, unsupported markers, ABI/runtime/accounting changes, allowlist
changes, or named-case handling. Do not use move-vector order as ordered
final-state proof, and do not infer mutual exclusion from filenames, block
labels, or observed runtime paths. Source freshness remains a separate required
check for participating sources, but it is not destination fan-in authority.
Focused evidence is recorded in
`build/agent_state/637_step1_evidence.md`; proof log is `test_after.log`.

## Proof

Command run:

```text
cmake --build --preset default && ALLOWLIST=build/agent_state/637_step1_destination_fan_in.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1
```

Result: build succeeded with no work; the allowlisted probe returned nonzero as
current fail-closed evidence with `total=6 passed=0 failed=6`. Root proof log:
`test_after.log`. Per-row case logs are under
`build/rv64_gcc_c_torture_backend/`.
