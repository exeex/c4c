Status: Active
Source Idea Path: ideas/open/622_repeated_stack_destination_fan_in_order_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Repeated Stack-Destination Residuals

# Current Packet

## Just Finished

Step 1 classification completed without implementation changes.

Examined the current backend proof log plus per-case RV64 gcc torture logs for
repeated stack-destination residuals:

- Ambiguous non-parallel register fan-in to one stack destination:
  `build/rv64_gcc_c_torture_backend/*/case.log` currently has `153` logs with
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
  The detailed form appears in `130` logs and reports
  `unsupported_prepared_move_bundle_classification: non-parallel`,
  `event_kind=before_instruction_copies`, `phase=before_instruction`,
  `authority=none`, `move_count=2`, `parallel_copy=no`,
  `move[*].source_home_kind=register`,
  `move[*].destination_home_kind=stack_slot`, owner
  `rv64_prepared_move_bundle_consumer`, and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
  Representative rows:
  `src/pr43236.c` has candidate order `move[0] 6 -> 5`, then
  `move[1] 4 -> 5`; `src/20000113-1.c` has candidate order
  `move[0] 45 -> 48`, then `move[1] 47 -> 48`. Current rejection reason:
  the stack destination has multiple register sources but no published
  ordering, mutual-exclusion, merge, or selected-final-state producer
  authority.
- Short ambiguous classifier rows: `23` of the current ambiguous logs report
  the same prepared consumer category but only the short rejection
  `prepared move-bundle classifier rejected ambiguous non-parallel multi-source
  stack-destination authority`. Representative `src/pr48814-2.c` exposes the
  row owner/category but not destination stack slot, source homes, or candidate
  order in today's log. Current rejection reason: same missing producer
  authority, with insufficient per-move payload in the short diagnostic.
- `src/pr71631.c`: current evidence exists at
  `build/rv64_gcc_c_torture_backend/src_pr71631.c/case.log`, but it is not the
  ambiguous non-parallel `authority=none` family. It reports
  `unsupported_move_bundle_target_shape`, `event_kind=pre_terminator_copies`,
  `phase=block_entry`, `authority=out_of_ssa_parallel_copy`,
  `move_count=10`, `parallel_copy=yes`,
  `parallel_copy_execution_site=predecessor_terminator`, and
  `fragment_status=generic_move_bundle_materialization_failed`. Repeated
  stack-destination evidence is visible for destination value `28`:
  candidate order `move[0] 26 -> 28` and `move[6] 27 -> 28`; both sources are
  registers and both destinations are stack slots. Current blocker: existing
  `out_of_ssa_parallel_copy` authority does not provide the explicit
  destination fan-in/order fact RV64 needs for repeated stack destinations, so
  the generic prepared move-bundle materializer remains fail-closed.

## Suggested Next

Implement one producer/prealloc authority fact family for repeated
register-source fan-in to a single stack destination. The fact should be
shape-independent and should publish authority kind, row owner, destination
stack slot/value, source homes, candidate order when relevant, and the legal
ordering, mutual-exclusion, merge, or selected-final-state semantics. Do not
target `src/pr71631.c` directly; use it only as evidence that the same
authority gap also appears inside an out-of-SSA parallel-copy bundle.

## Watchouts

Do not infer destination order, mutual exclusion, or last-writer behavior in
RV64. Do not special-case `src/pr71631.c` or weaken expectations to claim
capability progress. The short classifier rows currently lack destination
slot/source/candidate details, so producer work should improve structured
authority/diagnostic payloads instead of making RV64 guess from row order.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. `test_after.log` is preserved as the proof log. Because the
backend subset is green, detailed residual evidence came from the current
per-case logs under `build/rv64_gcc_c_torture_backend/*/case.log`, not from
CTest failure output.
