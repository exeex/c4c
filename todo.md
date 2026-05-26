Status: Active
Source Idea Path: ideas/open/34_aarch64_store_source_publication_planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit AArch64 Store-Source Decisions

# Current Packet

## Just Finished

Step 1 audited AArch64 store-source source-selection decisions in
`src/backend/mir/aarch64/codegen/memory_store_sources.*` without implementation
edits.

Source-selection categories currently inferred there:

- Pointer-value load publication:
  `emit_prepared_pointer_value_load_to_register()` and
  `lower_stack_homed_pointer_value_load_publication()` consume existing
  prepared addressing (`PreparedMemoryAccess` with `PointerValue`) and prepared
  value homes. Shared fact exists for the address/source home; no new
  store-source authority needed for this load-publication path.
- Narrow-store recovery:
  `store_local_recovered_narrow_store_source()`,
  `find_latest_narrow_store_for_wide_local_load()`,
  `local_slot_reference_matches()`, `store_local_targets_logical_slot()`, and
  `parse_trailing_dot_offset()` scan same-block prior stores and spelling/slot
  shape to recover the narrow source for a wide load feeding a store. The
  current publication plan can carry the result as
  `recovered_source_value` plus `recovered_source_instruction_index`, and shared
  tests already prove those fields, but the recovery decision itself is still
  AArch64-local. Intended shared authority: prepared store-source publication
  planning fed by prepared memory access, slot identity, and BIR producer/use
  facts before target codegen.
- Byval frame-slot load source:
  `store_local_value_is_byval_frame_slot_load()` rediscovers that a store value
  came from a same-block `LoadLocalInst` whose prepared access is a byval
  `PointerValue` base plus offset. Prepared addressing and byval formal naming
  already exist, but there is no shared store-source plan field saying this
  store value is a byval frame-slot load source. Intended shared authority:
  prepared store-source publication plan, likely by reusing the existing
  destination/source-home fields with a published source-producer/access fact.
- Cast producer publication:
  `store_local_value_cast_producer()` selects same-block `CastInst` producers,
  and `emit_scalar_conversion_cast_to_register()` emits the conversion. Shared
  `PreparedEdgePublication` producer facts already classify `Cast` producers,
  but store-local publication still consults the same-block AArch64 helper.
  Intended shared authority: prepared edge publication source-producer facts,
  reused by a store-source publication plan/query.
- Select producer publication:
  `store_local_value_has_select_producer()` and the reserved-scratch/memory
  paths in `lower_store_local_value_publication()` select same-block
  `SelectInst` producers. Shared `PreparedEdgePublication` producer facts
  already classify `SelectMaterialization`, but store-local publication still
  re-queries AArch64/MIR producer shape. Intended shared authority: prepared
  edge publication source-producer facts plus the existing store-source
  publication plan destination facts.
- Scalar-FP binary producer publication:
  `store_local_value_has_scalar_fp_binary_producer()` rediscovers same-block
  `BinaryInst` producers and filters with
  `is_prepared_scalar_float_alu_operation()`. Shared
  `PreparedEdgePublication` producer facts already classify `Binary`, but the
  scalar-FP operation eligibility is still target/helper-local. Intended shared
  authority: prepared edge publication producer facts with an added shared
  scalar-FP eligibility/result-home fact if needed.
- Direct global-load select chain:
  `select_chain_contains_direct_global_load()` gates store-local publication
  for select chains that can be rematerialized from a direct global load.
  Prepared address materialization/global-symbol facts exist, and
  `PreparedStoreSourcePublicationPlan` carries destination/source homes, but the
  direct-global-load-in-select-chain decision is still rediscovered from local
  producer shape. Intended shared authority: prepared edge publication producer
  facts plus prepared address materialization identity for the load source.
- Pointer store writeback:
  `lower_stack_homed_pointer_store_writeback()` uses
  `plan_stack_homed_pointer_store_writeback()` and already requires an
  available `PreparedStoreSourcePublicationPlan` with
  `PointerStoreWriteback`, pointer destination, and stack-homed pointer-base
  facts. Shared authority already exists; this is the most complete current
  consumer of the plan.
- Pointer-base-plus-offset local store publication:
  `lower_pointer_base_plus_offset_store_local_publication()` uses
  `plan_pointer_base_plus_offset_store_local_publication()` and consumes
  `PreparedValueHomeKind::PointerBasePlusOffset`, prepared global symbol
  identity, prepared addressing, and prepared value homes. Shared facts exist
  for the source home/address, but `emit_pointer_base_plus_offset_to_register()`
  still has one AArch64 fallback that rediscovers a pointer base from a
  same-block `LoadLocalInst`. Intended shared authority: prepared value home
  plus prepared memory access/source producer facts.
- Store-global value publication:
  `lower_store_global_value_publication()` publishes named store-global values
  from register or prepared stack-home memory operands by calling
  `emit_value_publication_to_register()`. `PreparedStoreSourcePublicationPlan`
  has `StoreGlobalPublication`, `pending_publication`, `stack_homes_only`, and
  `duplicate_publication` fields, and shared tests cover those flags, but this
  AArch64 path currently does not build/consume that plan.
- Pending store-global stack publications:
  `lower_pending_store_global_stack_value_publications()` scans forward through
  `SelectInst`, `LoadGlobalInst`, `BinaryInst`, `CastInst`, and
  `StoreGlobalInst` using `is_store_global_select_snapshot_run_instruction()`
  to publish delayed stack values. Shared plan flags exist for pending
  store-global publication, but the scan boundary and publication decision are
  AArch64-local.

Selected first migration target: cast producer publication for store-local
values. Current AArch64 helper path:
`lower_store_local_value_publication()` ->
`store_local_value_cast_producer()` -> `find_same_block_named_producer()`, with
emission through `emit_scalar_conversion_cast_to_register()`. Intended shared
authority: prepared edge publication source-producer facts
(`PreparedEdgePublicationSourceProducerKind::Cast`) exposed through the
prepared store-source publication plan/query before AArch64 codegen consumes it.
This target already has a shared producer category and avoids broad memory,
slot, byval, or global-chain representation churn.

## Suggested Next

Execute Step 2 for the selected cast-producer target: publish a store-local
source-producer fact upstream by reusing prepared edge publication producer
classification, then add focused shared coverage that the cast producer is
available before AArch64 emission.

## Watchouts

- Do not merge `memory_store_sources.*` into `memory.cpp` during this semantic
  migration.
- Do not add AArch64-only rediscovery under a new helper name.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  or fixture-specific matching.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_x86_store_source_publication_plan_reuse|backend_aarch64_prepared_memory_operand_records)$' | tee test_after.log`

Result: passed. Build completed with no work to do, and all 3 targeted tests
passed. Proof log: `test_after.log`.
