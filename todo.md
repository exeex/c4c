Status: Active
Source Idea Path: ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the compatibility status baseline

# Current Packet

## Just Finished

Step 1 - Established the compatibility status baseline for active idea 246 by
read-only inspection of the prepared helper, x86 route-debug, and riscv
prepared edge-publication surfaces.

Inspected proof files and owned surfaces:
- `src/backend/prealloc/publication_plans.hpp`: prepared source-memory,
  edge-copy source facts, current-block join, aggregate-stack-source, and
  typed-stack-source status names.
- `src/backend/prealloc/value_locations.hpp`: current-block-entry publication
  status names, including `publication_unavailable`.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: helper-oracle
  coverage for prepared edge/source memory facts, current-block join
  publication facts, current-block publication and entry publication identity,
  typed/aggregate stack-source authority, Route 4/Route 5 additions, and
  Route 6 helper rows.
- `src/backend/mir/x86/debug/debug.cpp` and
  `tests/backend/bir/backend_x86_route_debug_test.cpp`: x86 route-debug
  `ConsumedPlans` compatibility plus Route 6 scalar argument diagnostic rows.
- `src/backend/mir/riscv/codegen/emit.cpp` and
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`: riscv
  prepared-backed edge-publication fallback, Route 5/Route 3 agreement fields,
  and wrapper-visible instruction text.

Retained prepared compatibility families:
- Prepared edge-publication source-memory status names:
  `available`, `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, and `unavailable`. Current proof lives
  in `verify_edge_publication_source_producer_facts()` via complete load-local
  memory facts and source-memory mismatch/missing-result rejection checks.
- Prepared edge-copy source facts statuses: `missing_prepared_lookups`,
  `missing_predecessor_label`, `missing_successor_label`,
  `missing_destination_value`, `missing_publication`,
  `ambiguous_publication`, `publication_unavailable`, `edge_mismatch`,
  `unsupported_move`, `move_edge_mismatch`,
  `publication_move_mismatch`, `missing_source_value`,
  `missing_source_home`, `missing_source_producer`,
  `missing_source_memory_access`, and `incomplete_source_memory_access`.
  Current proof is mostly semantic helper behavior in
  `backend_prepared_lookup_helper_test.cpp`; later steps should add explicit
  string-name assertions for the public compatibility rows that are currently
  only proven through enum behavior.
- Typed stack-source publication status names:
  `unsupported_source_home`, `missing_same_width_i32_type`, and
  `missing_destination_gpr_bank` are public compatibility rows. Current riscv
  proof observes the target fallback as `UnsupportedSourceHome` and empty
  instruction text in stack-source fail-closed tests; helper-level string-name
  assertions for those exact prepared names are still missing.
- Aggregate stack-source authority status name:
  `missing_aggregate_copy_authority` is directly protected by
  `verify_aggregate_stack_source_authority_status()`, alongside enum behavior
  for unavailable, incomplete concrete source, incomplete destination mapping,
  and unsupported move authority.
- Current-block join/source publication compatibility is protected by
  `verify_current_block_join_parallel_copy_source_query()`, including prepared
  fallback when Route 5 is absent, no-source, source-mismatched,
  destination-mismatched, memory-source, duplicate, or wrong-predecessor.
- Current-block-entry publication compatibility includes
  `publication_unavailable` in `PreparedCurrentBlockEntryPublicationStatus`.
  Current proof exercises entry-publication behavior in
  `backend_prepared_lookup_helper_test.cpp`, but an exact string-name assertion
  for `publication_unavailable` should be added before close.

x86 route-debug and helper-oracle status baseline:
- The retained x86 compatibility surface is still the route-debug/summary text
  around `ConsumedPlans`, grouped authority, wrapper-output assertions, and
  prepared dump rows. Route 6 scalar rows are additions, not replacements.
- Existing Route 6 diagnostic rows in `backend_x86_route_debug_test.cpp`:
  `status=available gate=agreed`,
  `status=missing_source_relationship gate=blocked`,
  `status=available gate=missing_source_value`,
  `status=available gate=missing_source_name`,
  `status=duplicate_relationship gate=blocked`,
  `status=available gate=prepared_source_mismatch`, and
  `status=available gate=source_value_mismatch`.
- `src/backend/mir/x86/debug/debug.cpp` derives the gate strings from Route 6
  evidence and prepared-source agreement, then emits the source row only for
  `gate=agreed`. That keeps the rows diagnostic-only beside the retained
  prepared compatibility surface.
- Wrapper-output/assertion coverage remains in the x86 route-debug test via
  direct extern memory-return prepared dump rows and the prior wrapper-output
  assertions; later x86 Step 3 should not replace or rename these rows.

riscv fallback and route-native addition baseline:
- `backend_riscv_prepared_edge_publication_test.cpp` protects fallback strings
  and wrapper-visible output such as `mv a1, a0`, `lw a1, 12(s2)`,
  `addi a1, s2, 12`, `sw a0, 24(sp)`, and empty output for fail-closed
  unsupported paths.
- `src/backend/mir/riscv/codegen/emit.cpp` remains prepared-backed:
  Route 5/Route 3 facts only attach `route5_edge_status`,
  `route5_edge_source_agrees`, and `route3_source_memory_agrees`; they do not
  own register, stack, scratch, offset, or instruction spelling policy.
- Existing Route 5/Route 3 rows are additions, not replacements:
  `available`, `memory_source`, `no_source`, `no_match`,
  `missing_publication`, and `missing_source_producer` are observed beside
  prepared fallback behavior. Duplicate Route 5 evidence is currently exposed
  by index-size and fail-closed checks rather than as an independently named
  duplicate status.

Missing compatibility assertions later steps should add:
- Direct string-name assertions for retained prepared source-memory and
  edge-copy compatibility rows where only enum/status behavior is currently
  covered, especially `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, and `publication_unavailable`.
- Direct string-name assertions for typed stack-source rows
  `unsupported_source_home`, `missing_same_width_i32_type`, and
  `missing_destination_gpr_bank`.
- Direct current-block-entry string-name coverage for
  `publication_unavailable`.
- Explicit checks that route-native diagnostic rows remain additions beside
  prepared rows, not replacements, when later x86/riscv diagnostic naming is
  added.

## Suggested Next

Execute Step 2: add focused prepared compatibility assertions for the missing
string-name gaps without changing status names or wrapper-visible output.

## Watchouts

Likely narrow proof commands:
- Default build prepared/riscv proof:
  `cmake --build build --target backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication)$' | tee test_after.log`
- x86 route-debug proof, if Step 3 or close scope includes x86:
  `cmake --build build-x86 --target backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_prepared_lookup_helper)$' | tee test_after.log`

The x86 route-debug test is guarded by the x86-enabled test build; use
`build-x86` if the default `build` directory does not contain
`backend_x86_route_debug_test`. Do not rename or weaken prepared compatibility
statuses. Do not implement x86 or riscv adapters, claim deletion readiness,
open draft 155, or treat route-native diagnostic string equality as replacement
authority.

## Proof

Analysis-only packet; no build or ctest proof required and no `test_after.log`
was generated. Validation was read-only inspection of the files listed above,
plus `rg`/`sed` source and test queries.
