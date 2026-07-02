# Current Packet

Status: Active
Source Idea Path: ideas/open/527_bir_route7_comparison_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Route7 Comparison Owner

## Just Finished

Step 1: Map Route7 Comparison Boundaries completed as a mapping-only packet.
AST-backed queries were run against `src/backend/bir/bir.cpp` and
`src/backend/bir/bir.hpp` with `c4c-clang-tool-ccdb` / `c4c-clang-tool` for
signatures, callers, callees, and route7 comparison type references.

Exact route7 record-construction bodies to move in the next implementation
packet:

- `route7_comparison_operand_record` at `src/backend/bir/bir.cpp:1182`.
- `route7_comparison_instruction_record` at `src/backend/bir/bir.cpp:1236`.
- `route7_branch_condition_record` at `src/backend/bir/bir.cpp:1278`.
- Both `route7_build_comparison_condition_index` overloads at
  `src/backend/bir/bir.cpp:1487` and `src/backend/bir/bir.cpp:1519`.

Route7 lookup / validation bodies are route7-facing but are not
record-construction bodies for this slice; keep them in place unless the
supervisor explicitly widens the packet:

- `route7_find_comparison_instruction`, `route7_find_comparison_operand`,
  `route7_find_materialized_condition`, `route7_find_branch_condition`.
- `route7_validate_comparison_instruction_reference`,
  `route7_validate_comparison_operand_reference`,
  `route7_validate_materialized_condition_reference`,
  `route7_validate_branch_condition_reference`.

Facade-backed public query helpers and facade bridge bodies that must not move
with the record-construction slice:

- `route_index_reference_facade` overloads at `src/backend/bir/bir.cpp:1989`,
  `:1996`, and `:2003`.
- `route_index_validate_comparison_operand_reference` at
  `src/backend/bir/bir.cpp:2073`.
- `route_index_validate_materialized_condition_reference` at
  `src/backend/bir/bir.cpp:2105`.
- Raw/public fused helper `find_fused_compare_operand_producer_facts` at
  `src/backend/bir/bir.cpp:2140`.
- Route7 facade-backed fused helper
  `route7_find_fused_compare_operand_producer_facts` at
  `src/backend/bir/bir.cpp:2175`.
- Materialized-condition public consumer
  `find_materialized_condition_producer_identity` at
  `src/backend/bir/bir.cpp:2222`.
- Anonymous `route7_operand_record_to_public` at `src/backend/bir/bir.cpp:2158`
  should stay with those public consumers for this slice because both fused and
  materialized-condition helpers depend on it.

AST direct callers/callees summary:

- `route7_comparison_operand_record` is called only by
  `route7_comparison_instruction_record`; it calls `route1_source_value_identity`,
  anonymous `find_unique_comparison_producer`,
  anonymous `comparison_producer_kind_for_inst`, and anonymous
  `evaluate_comparison_integer_constant`.
- `route7_comparison_instruction_record` is called by
  `route7_branch_condition_record` and both
  `route7_build_comparison_condition_index` overloads; it calls
  `route1_source_value_identity`, `binary_operand_type`,
  anonymous `is_comparison_binary_opcode`, and
  `route7_comparison_operand_record`.
- `route7_branch_condition_record` is called by both
  `route7_build_comparison_condition_index` overloads; it calls
  `route1_source_value_identity`, anonymous
  `find_unique_comparison_producer`, anonymous
  `is_comparison_binary_opcode`, and
  `route7_comparison_instruction_record`.
- `route7_build_comparison_condition_index` is directly called in this TU only
  by `find_materialized_condition_producer_identity`; cross-file users include
  AArch64 comparison lowering and route7-focused backend tests.
- `route_index_reference_facade` direct callers in this TU are
  `route7_find_fused_compare_operand_producer_facts` and
  `find_materialized_condition_producer_identity`; cross-file route4 facade
  users also exist in query/prealloc code and tests.
- `route_index_validate_comparison_operand_reference` direct caller in this TU
  is `route7_find_fused_compare_operand_producer_facts`; cross-file users
  include AArch64 comparison lowering and route7 tests.
- `route_index_validate_materialized_condition_reference` direct caller in this
  TU is `find_materialized_condition_producer_identity`; cross-file users
  include AArch64 comparison lowering and route7 tests.

Comparison type/reference map:

- `Route7ComparisonConditionIndex` is referenced by route7 missing/reference
  helpers, the two build overloads, route7 finders, route7 validators, facade
  overloads, `route7_find_fused_compare_operand_producer_facts`, and
  `find_materialized_condition_producer_identity`.
- `Route7ComparisonInstructionRecord`, `Route7ComparisonOperandRecord`, and
  `Route7BranchConditionRecord` are constructed in the move-target bodies and
  then consumed by existing find/validate/facade-backed helpers.
- `RouteIndexReferenceFacade` references start at the facade overloads and are
  consumed by route-index validators and the tail public query helpers.
- `FusedCompareOperandProducerFacts`,
  `MaterializedConditionProducerIdentity`, and
  `ComparisonOperandProducer` belong to the tail public query/helper boundary
  for this slice, not the route7 record-construction move.

Private helper declarations likely needed after moving bodies to a new
`bir_route7_comparison.cpp`:

- Anonymous comparison producer helpers currently in `bir.cpp`:
  `produced_value_for_comparison_producer`,
  `comparison_producer_kind_for_inst`, `is_comparison_binary_opcode`,
  `find_unique_comparison_producer`, and
  `evaluate_comparison_integer_constant`.
- `SameBlockComparisonProducer` is anonymous-local state used by
  `find_unique_comparison_producer`.
- Existing public helpers `route1_source_value_identity` and
  `binary_operand_type` remain available through `bir.hpp`.
- `route_block_matches` is already in `bir_private.hpp`; it is needed by
  find/validate bodies only if a later packet moves those too.

## Suggested Next

Supervisor can delegate Step 2 to add `src/backend/bir/bir_route7_comparison.cpp`
and move only the five route7 record/index construction bodies listed above,
with minimal private helper exposure for the anonymous comparison producer
helpers.

## Watchouts

- Keep this activation aligned to
  `ideas/open/527_bir_route7_comparison_body_extraction.md`.
- Do not move route-index facade bodies, materialized-condition public query
  helpers, fused-compare public query helpers, route7 public declarations, or
  tests in the next body-move packet.
- Build registration for the prior route extraction pattern is in
  `tests/backend/bir/CMakeLists.txt`; any new route7 owner will need analogous
  registration in the implementation packet.
- Cross-file route7/facade users found by search include
  `src/backend/mir/aarch64/codegen/comparison.cpp`,
  `src/backend/mir/query.cpp`,
  `src/backend/prealloc/prepared_lookups.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.

## Proof

Mapping-only packet; no implementation files were edited and no build/tests
were run. No `test_after.log` was produced because the supervisor proof said no
build proof is required for a `todo.md`-only mapping update.

Focused proof recommendation for the body-move packet:

- Build/link proof that includes the new BIR route7 translation unit and the
  backend consumers that still link against the facade-backed helpers.
- Comparison condition indexing coverage from
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` route7 sections.
- Materialized-condition consumer coverage from the AArch64 comparison lowering
  tests in `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.
- Link-time backend coverage that exercises
  `src/backend/mir/aarch64/codegen/comparison.cpp` users of
  `route7_build_comparison_condition_index`,
  `route7_find_fused_compare_operand_producer_facts`,
  `find_materialized_condition_producer_identity`, and the route-index
  materialized/operand validators.
