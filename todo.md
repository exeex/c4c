Status: Active
Source Idea Path: ideas/open/147_comparison_prealloc_fact_owner.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Comparison Fact Ownership

# Current Packet

## Just Finished

Step 1 - Inspect Comparison Fact Ownership completed as an inspection-only
packet.

Current declarations:

- `src/backend/prealloc/prepared_lookups.hpp`: declares
  `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`,
  and `PreparedMaterializedConditionProducer` at the top of the prepared lookup
  header. It also declares these comparison helper APIs:
  `find_prepared_fused_compare_operand_producer`,
  `find_prepared_fused_compare_operand_producer_facts`, and
  `find_prepared_materialized_condition_producer`.

Current definitions:

- `src/backend/prealloc/prepared_lookups.cpp`: defines the three helper APIs
  near the same-block/source-producer helper definitions.
- AST-backed definition/signature checks confirmed:
  `find_prepared_fused_compare_operand_producer` returns
  `std::optional<PreparedFusedCompareOperandProducer>` and takes
  `PreparedNameTables`, `PreparedEdgePublicationSourceProducerLookups*`,
  `BlockLabelId`, `bir::Block*`, a `bir::Value`, and
  `before_instruction_index`.
- `find_prepared_fused_compare_operand_producer_facts` returns
  `std::optional<PreparedFusedCompareOperandProducerFacts>` and takes the same
  context plus `PreparedBranchCondition`.
- `find_prepared_materialized_condition_producer` returns
  `std::optional<PreparedMaterializedConditionProducer>` and takes the same
  context plus the materialized condition `bir::Value`.

Direct include/call consumers:

- `src/backend/mir/aarch64/codegen/comparison.cpp` is the only non-prealloc
  file that directly references the comparison fact structs or these helper
  names. It should switch from relying on `prepared_lookups.hpp` for these
  declarations to including the new comparison owner once Step 2 creates it.
- `src/backend/prealloc/prepared_lookups.cpp` is the current implementation
  owner and includes `prepared_lookups.hpp`; Step 2 should move only the
  comparison helper definitions out of this file.
- `src/backend/prealloc/prepared_lookups.hpp` currently exposes the comparison
  facts transitively to broader prepared-lookup consumers. Other
  `prepared_lookups.hpp` include sites were not direct users of the moved
  comparison names by text search.
- AST-backed caller checks in `src/backend/mir/aarch64/codegen/comparison.cpp`
  found target-local wrappers and lowering consumers. These are consumers, not
  move candidates: local wrappers at lines for
  `find_prepared_fused_compare_operand_producer_facts`,
  `find_prepared_fused_compare_operand_producer`, and
  `find_prepared_materialized_condition_producer`; lowering consumers include
  `lower_prepared_conditional_branch_terminator`,
  `lower_missing_fused_compare_operand_publication`,
  `lower_fused_compare_branch_from_emitted_cast`,
  `lower_constant_rhs_fused_compare_branch`,
  `lower_stack_home_fused_compare_branch`, and
  `lower_materialized_compare_condition_branch`.

Same-block/source-producer dependencies:

- The comparison helpers depend on the existing source-producer lookup model:
  `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedEdgePublicationSourceProducerKind`, and producer payload fields from
  `publication_plans.hpp`.
- The shared materialization boundary is already named in
  `src/backend/prealloc/select_chain_lookups.hpp`, which declares
  `PreparedSameBlockScalarProducer`,
  `PreparedSameBlockValueMaterializationQuery`,
  `find_prepared_same_block_scalar_producer`, and
  `evaluate_prepared_same_block_integer_constant`.
- AST-backed callee checks confirmed
  `find_prepared_fused_compare_operand_producer` calls
  `existing_prepared_value_name_id`,
  `find_prepared_same_block_scalar_producer`, and
  `evaluate_prepared_same_block_integer_constant`.
- `find_prepared_fused_compare_operand_producer_facts` calls only
  `find_prepared_fused_compare_operand_producer`.
- `find_prepared_materialized_condition_producer` calls
  `existing_prepared_value_name_id` and
  `find_prepared_same_block_scalar_producer`.

Owner decision:

- Create `src/backend/prealloc/comparison.hpp` and
  `src/backend/prealloc/comparison.cpp` in Step 2.
- Do not extend `control_flow.hpp` for these facts. `control_flow.hpp` owns
  branch-condition and branch-target semantics and is already broad and
  inline-heavy. The comparison producer facts sit between control-flow branch
  conditions and target-local compare lowering, and their concrete dependencies
  are source-producer/same-block materialization APIs, not control-flow target
  resolution.
- Keep target-local AArch64 compare instruction selection and condition-code
  emission in `src/backend/mir/aarch64/codegen/comparison.cpp`; only the shared
  producer fact structs and lookup helper declarations/definitions belong in
  the new prealloc comparison owner.

## Suggested Next

Step 2 should add `src/backend/prealloc/comparison.hpp` and
`src/backend/prealloc/comparison.cpp`, move the three comparison fact structs
and the three shared helper declarations/definitions there, include
`select_chain_lookups.hpp` for same-block materialization dependencies, and
update `src/backend/mir/aarch64/codegen/comparison.cpp` to include the new
owner while leaving target-local lowering code in place.

## Watchouts

- CMake currently globs `src/backend/prealloc/*.cpp`, so a new
  `comparison.cpp` should be picked up without target-source edits.
- `prepared_lookups.hpp` still owns `PreparedFunctionLookups`,
  return-chain helpers, edge-publication lookups, and
  `PreparedSameBlockLoadLocalStoredValueSource`; do not move those with the
  comparison slice.
- `src/backend/mir/aarch64/codegen/comparison.cpp` already includes
  `select_chain_lookups.hpp` for other materialization helpers. The new
  comparison owner should not become an umbrella header that hides the
  same-block/source-producer owner.
- Do not move AArch64 compare instruction selection or condition-code emission
  into shared prealloc ownership.
- Do not change branch-condition semantics.

## Proof

Inspection-only packet. Used `c4c-clang-tool` and `c4c-clang-tool-ccdb` for
AST-backed signatures, definitions, callers, and callees around the comparison
helpers. No build or tests were required by the delegated proof, and no
`test_after.log` was produced.
