Status: Active
Source Idea Path: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The Existing Authority Surface

# Current Packet

## Just Finished

Step 1: Map The Existing Authority Surface completed as a discovery-only
packet.

Producer-side owner and current authority model:
- `src/backend/prealloc/regalloc.hpp` owns the shared move-bundle taxonomy:
  `PreparedMoveAuthorityKind`, `prepared_move_authority_kind_name`,
  `PreparedMoveResolution`, and `PreparedMoveBundle`.
- Current authority kinds are `None`, `OutOfSsaParallelCopy`, and
  `StackSlotWideningConversion`; there is no explicit prepared authority kind
  yet for non-parallel register-source fan-in to one stack destination.
- `src/backend/prealloc/value_locations.hpp` owns `PreparedMoveBundle` storage
  and per-bundle evidence fields: function, phase, authority, block/instruction
  position, optional parallel-copy predecessor/successor labels, moves, and ABI
  bindings.
- `src/backend/prealloc/prepared_object_traversal.cpp` currently owns the
  fail-closed producer/consumer classifier for this gap:
  `prepared_move_bundle_has_ambiguous_multi_source_stack_destination`,
  `prepared_move_bundle_is_select_materialization_stack_destination`, and
  `classify_prepared_object_move_bundle_consumer`.
- The current missing-authority classifier status is
  `PreparedObjectMoveBundleConsumerStatus::
  AmbiguousNonParallelMultiSourceStackDestination` in
  `src/backend/prealloc/prepared_object_traversal.hpp`.

RV64 consumer surface:
- `src/backend/mir/riscv/codegen/object_emission.cpp` consumes the prepared
  object traversal classification in `prepared_function_to_object_function`,
  then either accepts `classification.move_bundle` or rejects before
  `fragment_for_prepared_move_bundle`.
- The current missing-authority diagnostic string is produced by
  `rv64_prepared_move_bundle_classification_failure_diagnostic` in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, with
  `diagnostic_owner=rv64_prepared_move_bundle_consumer` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
- Existing legal RV64 prepared stack-destination publication support is in
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`:
  `prepared_select_publication_gpr_to_stack_destination_is_admitted`,
  `prepared_select_publication_gpr_to_stack_destination_matches_bundle`,
  `prepared_predecessor_select_publication_bundle_is_stack_join_materialized`,
  and `prepared_predecessor_select_publication_bundle_is_rv64_object_admitted`.

Focused test targets found:
- Negative missing-authority target:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`,
  `rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle`.
- Positive existing RV64 stack-destination publication target:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, the
  `stack_destination` section in
  `builds_prepared_select_publication_move_object`.
- Producer/classifier target for taxonomy/fail-closed coverage:
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp`, especially
  `check_select_edge_source_producer_placement_contract`, plus any new focused
  prepared-object move-bundle classifier assertions near the existing
  stack-destination classifier setup.

## Suggested Next

Delegate Step 2. Define the minimal prepared authority taxonomy/evidence for
stack-destination register-source fan-in by editing
`src/backend/prealloc/regalloc.hpp`,
`src/backend/prealloc/value_locations.hpp` if new evidence fields are needed,
`src/backend/prealloc/prepared_object_traversal.hpp`, and
`src/backend/prealloc/prepared_object_traversal.cpp`. Add focused producer
tests proving explicit missing/unknown authority remains fail-closed and the
new supported authority spelling is representable.

## Watchouts

- Keep idea 585 inactive; it is documentation/research and says activation is
  out of scope unless requested later.
- Do not accept value-id-, filename-, function-, block-, offset-, or
  diagnostic-string-specific authority.
- Preserve fail-closed behavior for missing, unknown, unsupported, and
  genuinely ambiguous stack-destination fan-in.
- Do not make RV64 infer legality from the current diagnostic path; the next
  implementation packet should publish producer authority in prealloc first,
  then a later RV64 packet can consume it.
- The existing diagnostic producer is RV64, but the missing authority itself is
  exposed by the prepared object move-bundle classifier. Keep that distinction
  explicit in the taxonomy packet.
- Existing positive stack-destination materialization is select-publication
  backed; it should not be generalized by shape alone into arbitrary
  non-parallel bundles.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the proof output; CTest reports
`Total Test time (real) = 1.96 sec`.
