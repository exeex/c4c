Status: Active
Source Idea Path: ideas/open/159_bir_producer_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Producer Identity Surfaces

# Current Packet

## Just Finished

Step 1 - Inventory Producer Identity Surfaces completed as an inspection-only
packet.

Concrete current Route 1 surfaces:

- `src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp` already define
  BIR-local producer identities for comparison and call-argument paths:
  `ComparisonProducerKind`, `ComparisonOperandProducer`,
  `MaterializedConditionProducerIdentity`,
  `CallArgumentSourceProducerKind`, and
  `CallArgumentSourceProducerMaterialization`.
- `src/backend/mir/query.hpp` / `src/backend/mir/query.cpp` currently host the
  generic same-block identity vocabulary and query API:
  `SameBlockProducerKind`, `SameBlockValueIdentity`,
  `SameBlockProducerRecord`, `SameBlockValueMaterializationQuery`,
  `find_same_block_scalar_producer`, `evaluate_same_block_integer_constant`,
  and the BIR select-chain identity helpers.
- `src/backend/prealloc/publication_plans.hpp` owns the prepared producer-kind
  authority that Route 1 must stop depending on:
  `PreparedEdgePublicationSourceProducerKind` with `Unknown`, `Immediate`,
  `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, and
  `SelectMaterialization`.
- `src/backend/prealloc/select_chain_lookups.cpp` currently populates
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name` from
  `LoadLocalInst`, `LoadGlobalInst`, `CastInst`, `BinaryInst`, and
  `SelectInst`, resolving through prepared value-name ids.
- `src/backend/prealloc/prepared_lookups.cpp` currently computes the prepared
  same-block oracle in `prepared_same_block_source_producer`,
  `find_prepared_same_block_scalar_producer`, and
  `evaluate_prepared_same_block_integer_constant`.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` already contains
  prepared-vs-BIR comparison helpers for scalar producers, integer constants,
  select-chain answers, call-argument source materialization, global-load
  access, and load-local source identity.

## Suggested Next

Execute Step 2 as the first code-changing packet:

- Edit `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp` to add the
  target-neutral Route 1 BIR producer vocabulary and typed annotation records.
- Prefer consolidating around the existing generic MIR same-block vocabulary in
  `src/backend/mir/query.hpp` / `src/backend/mir/query.cpp` only if the next
  packet keeps BIR as the durable owner and avoids cloning prepared structures.
- Add or adjust oracle checks in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for the new BIR
  records before switching consumers.
- First code-changing proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.

## Watchouts

- Producer kinds needing equivalence coverage: `Immediate` as constant-only
  source, `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`,
  `SelectMaterialization`, plus `Unknown`/missing-fact fail-closed behavior and
  future-producer fail-closed behavior.
- Current same-block constant computation is duplicated between
  `bir::evaluate_comparison_integer_constant`,
  `prepare::evaluate_prepared_same_block_integer_constant`, and
  `mir::evaluate_same_block_integer_constant`; the Step 2 record shape should
  preserve immediate integer facts without making prepared the source of truth.
- Current materialization availability is split across
  `mir::same_block_producer_kind_has_materialization`,
  `mir::find_bir_select_chain_scalar_materialization_eligibility`, prepared
  `find_prepared_scalar_select_chain_materialization`, and
  `bir::find_call_argument_source_producer_materialization`. Keep the new
  payload about semantic availability only; do not import prepared homes,
  registers, storage, frame slots, emitted state, spill/reload behavior,
  operand views, or final instruction records.
- Prepared oracle helpers to keep for migration checks:
  `find_prepared_same_block_scalar_producer`,
  `evaluate_prepared_same_block_integer_constant`,
  `find_prepared_select_chain_source_producer`,
  `find_prepared_scalar_select_chain_materialization`, and
  `find_prepared_call_argument_source_producer_materialization`.
- Existing consumer sites that still read prepared producer facts include
  `src/backend/mir/aarch64/codegen/alu.cpp` and
  `src/backend/mir/aarch64/codegen/calls.cpp`; do not switch them until the
  BIR/prepared oracle checks are green.

## Proof

Inspection-only packet. No build or test proof was required, and no
`test_after.log` was generated.
