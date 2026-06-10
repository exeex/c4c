Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Prepared Query Surface And Evidence Set

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: located the prepared same-block producer and
integer-constant query surface, direct callers, representative proof cases, and
the first code-changing packet.

Inspected query owners:
- `src/backend/prealloc/prepared_lookups.cpp`
  - core producer helper: anonymous `prepared_same_block_source_producer(...)`
  - public producer wrappers:
    `find_prepared_same_block_scalar_producer(names, source_producers,
    block_label, block, value_name, value_type, before_instruction_index)`,
    `find_prepared_same_block_scalar_producer(names, source_producers,
    block_label, block, value, before_instruction_index)`, and
    `find_prepared_same_block_scalar_producer(query, value)`
  - core constant helper: anonymous
    `evaluate_prepared_same_block_integer_constant(..., depth)`
  - public constant wrappers:
    `evaluate_prepared_same_block_integer_constant(names, source_producers,
    block_label, block, value, before_instruction_index)` and
    `evaluate_prepared_same_block_integer_constant(query, value)`
- `src/backend/prealloc/select_chain_lookups.hpp`
  - `PreparedSameBlockScalarProducer`
  - `PreparedSameBlockValueMaterializationQuery`
- `src/backend/prealloc/publication_plans.hpp`
  - `PreparedEdgePublicationSourceProducerKind`
  - `PreparedEdgePublicationSourceProducer`
  - `PreparedEdgePublicationSourceProducerLookups`
- `src/backend/mir/query.hpp` and `src/backend/mir/query.cpp`
  - existing target-neutral same-block BIR query surface:
    `SameBlockProducerKind`, `SameBlockProducerRecord`,
    `find_same_block_named_producer_record`,
    `find_same_block_binary_producer`,
    `find_same_block_select_producer`, and
    `evaluate_same_block_integer_constant`

Prepared query data and return shape:
- Producer query consumes `PreparedNameTables`,
  `PreparedEdgePublicationSourceProducerLookups`, `BlockLabelId`,
  `bir::Block*`, value name/type or named `bir::Value`, and
  `before_instruction_index`.
- Producer query validates lookup presence, same block, producer index before
  the requested instruction, instruction/result pointer agreement, named value
  identity, and type identity.
- Producer query returns `PreparedSameBlockScalarProducer` with copied prepared
  producer facts, the producer `bir::Inst*`, instruction index, and value name.
- Constant query consumes the same query identity plus a `bir::Value`; immediate
  values are the base case, named values recurse through same-block binary
  producers up to depth 4.
- Constant query returns `std::optional<std::int64_t>` for supported binary
  integer operations and fails closed for missing facts, future producers,
  mismatched facts, non-binary producers, unsupported depth, and invalid shifts
  or division/remainder by zero.

AST-backed direct caller evidence:
- `src/backend/prealloc/prepared_lookups.cpp`
  - producer wrappers call the core/wider overloads; same-TU caller
    `find_prepared_same_block_load_local_stored_value_source`
  - constant wrappers call the recursive constant helper
- `src/backend/prealloc/call_plans.cpp`
  - `find_prepared_call_argument_source_producer_materialization`
- `src/backend/prealloc/comparison.cpp`
  - `find_prepared_fused_compare_operand_producer`
  - `find_prepared_materialized_condition_producer`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  - anonymous `prepared_same_block_scalar_producer`
  - anonymous `prepared_same_block_integer_constant`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  - anonymous `prepared_same_block_publication_source_producer`
- `src/backend/mir/aarch64/codegen/alu.cpp`
  - anonymous `find_prepared_control_same_block_scalar_producer`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
  - anonymous `prepared_same_block_scalar_producer`

Representative proof cases:
- Scalar/binary/constant oracle:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_prepared_same_block_scalar_source_facts` builds a same-block binary
  chain, proves `%sum` producer identity, folds `%product` to `42`, checks
  immediate fused-compare RHS facts, and verifies future/missing/mismatched
  facts fail closed.
- Load/cast/binary/select producer facts:
  `verify_edge_publication_source_producer_facts` covers `LoadLocal`, `Cast`,
  `Binary`, `SelectMaterialization`, and immediate/unknown materialization
  rejection through prepared source-producer facts.
- Load-global/select-chain facts:
  `verify_direct_global_select_chain_dependency_query` covers `LoadGlobal`,
  select root identity, direct global load root identity, and missing-root
  fail-closed behavior.
- Target-neutral BIR query baseline:
  `tests/backend/mir/backend_x86_shared_producer_query_test.cpp` covers
  `mir::SameBlockProducerKind::{Binary, Cast, Select, LoadLocal}`,
  same-block integer constant evaluation, producer instruction indexes, and
  select-chain dependency traversal without prepared/prealloc enum authority.
- AArch64 codegen consumer proof:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  `branch_condition_publication_uses_prepared_source_producer_authority` proves
  a prepared binary source producer materializes a missing branch condition and
  fails closed when source-producer lookups are removed.

## Suggested Next

Execute Step 2 as the first code-changing packet: extend the target-neutral
`src/backend/mir/query.hpp` / `src/backend/mir/query.cpp` same-block query
surface into a prepared-equivalence BIR identity vocabulary and record shape,
without importing `PreparedEdgePublicationSourceProducerKind`, value homes,
storage, registers, operand views, or final emission state.

Smallest proposed implementation scope:
- Add/adjust BIR-owned producer identity records around the existing
  `mir::SameBlockProducerKind` surface so the record can carry named value,
  type, block/before-index, producer instruction pointer/index, immediate
  constant availability, and materialization availability.
- Preserve the existing prepared queries as the oracle; do not switch any
  AArch64 consumer in Step 2.
- Add or extend targeted query tests first, with comparison-ready cases for
  binary, cast, load-local, load-global, select, immediate, recursive constant,
  future-producer, missing-fact, and mismatched-type/name failure behavior.

## Watchouts

- Keep BIR identity target-neutral; do not import register homes, storage
  state, publication-owned enum authority, or final emission order.
- Treat the prepared queries as the oracle until BIR/prepared equivalence is
  proven.
- Do not switch broad MIR emission or rewrite expectations as progress.
- Leave `review/phase_a_steps_1_4_route_review.md` untouched.
- Existing `mir::evaluate_same_block_integer_constant` currently finds binary
  producers by named value without an explicit `before_instruction_index`;
  Step 2 should account for prepared-equivalent before-index semantics before
  using it as an oracle-equivalent query.
- `PreparedEdgePublicationSourceProducerKind::Immediate` is not represented as
  a producer instruction in BIR; model immediate constants as value facts, not
  as publication enum authority.

## Proof

Analysis-only packet; no build or test proof was delegated, and no
`test_after.log` was created.

Recommended narrow backend/codegen proof command for the next implementation
packet:

```bash
cmake --build build --target backend_x86_shared_producer_query backend_prepared_lookup_helper backend_aarch64_instruction_dispatch && ctest --test-dir build -R '^(backend_x86_shared_producer_query|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Use matched `test_before.log` / `test_after.log` if the supervisor promotes the
next packet from query-shape work into accepted code-changing proof.
