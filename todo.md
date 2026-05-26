Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Semantic Residue And Prepared Fact Gaps

# Current Packet

## Just Finished

Step 1 inventory completed for idea 39a without implementation edits.

Blocked helper to source-fact map:

- `find_latest_narrow_store_for_wide_local_load`: recovers a prior narrow
  `StoreLocalInst` source for a later wide `LoadLocalInst` by scanning the
  current BIR block backward, matching local slot ids/names, prepared frame
  object names, and trailing lane suffixes, then accepting only a latest I8
  store feeding a wider integer load.
- `store_local_recovered_narrow_store_source`: recovers the store-local source
  value by finding the same-block `LoadLocalInst` producer of the stored value
  and delegating to `find_latest_narrow_store_for_wide_local_load`.
- `store_local_value_is_wide_load_from_narrow_local_store`: is only a boolean
  wrapper around the recovered narrow-store source path.
- `store_local_value_is_byval_frame_slot_load`: recovers that a stored value
  came from a same-block `LoadLocalInst` whose prepared memory access is a
  pointer-value load from a byval formal.
- `store_local_value_has_select_producer`: recovers that the stored value is
  produced by a same-block select.
- `store_local_value_has_scalar_fp_binary_producer`: recovers that the stored
  value is produced by a same-block binary and then applies the AArch64 scalar
  FP operation predicate.
- `select_chain_contains_direct_global_load`: recovers that a select dependency
  chain contains a direct global load by walking same-block producer
  dependencies. `lower_store_local_value_publication` uses it for local-store
  publication gating and memory-target publication; `lower_pending_store_global`
  and nearby dispatch paths use the same source-shape family for global-store
  stack publication.
- `emit_pointer_base_plus_offset_to_register`: first handles prepared global
  symbol and value-home cases, but still has a same-block producer fallback
  that treats the pointer base as a `LoadLocalInst` and reloads the producer's
  prepared local-load frame offset.

Existing prepared facts:

- `PreparedStoreSourcePublicationPlan` already records source value identity,
  destination memory access, destination frame/object metadata, source home,
  pointer-base home, pointer-store-writeback flags, recovered source value and
  instruction index, and source producer fields.
- `PreparedEdgePublicationSourceProducerLookups` already index source producer
  kinds for `LoadLocal`, `Cast`, `Binary`, and `SelectMaterialization`, with
  block label, instruction index, and producer pointers.
- `plan_prepared_store_source_publication` can carry a supplied
  `PreparedEdgePublicationSourceProducer`, but `memory_store_sources.cpp`
  currently supplies only the cast producer through
  `prepared_store_source_cast_producer`.
- Prepared memory accesses already expose destination/source address facts,
  stored/result value names, frame-slot ids, pointer-value base names,
  byte offsets, sizes, alignment, volatility, and base-plus-offset support.
- Prepared value homes already represent stack slots, registers, and
  `PointerBasePlusOffset` with base value name and byte delta.
- Existing focused tests prove some prepared facts already flow:
  `backend_store_source_publication_plan` covers source identity, recovered
  source records, cast source producers, pointer writeback, global pending
  flags, and missing source/access statuses; `backend_aarch64_prepared_memory_operand_records`
  covers store-local cast publication consuming a prepared source producer.

Missing prepared authority:

- No target-neutral fact currently computes the narrow-store recovery relation
  for a wide local load. The prepared store-source plan can carry
  `recovered_source_value`, but AArch64 still computes that value locally.
- No prepared store-source path currently supplies load-local/byval-formal
  source classification for store-local publication; AArch64 still combines
  same-block producer lookup, prepared memory access, and BIR byval parameter
  inspection.
- Store-source planning does not yet consume the existing generic prepared
  source producer facts for `Binary`, `SelectMaterialization`, or `LoadLocal`;
  only cast is threaded into local store-source plans.
- The scalar-FP binary case has a generic prepared `Binary` producer fact, but
  no target-neutral store-source predicate records whether that binary is a
  supported scalar FP store-publication source.
- The direct-global-load select-chain case has no prepared source dependency
  fact for store-source publication; AArch64 still walks same-block select
  dependencies.
- The pointer-base-plus-offset load-local fallback has no prepared value-home
  or source-producer authority saying the pointer base should be reloaded from
  the producer load-local frame offset.

External callers affected:

- `dispatch.cpp` calls
  `lower_store_local_value_publication`,
  `lower_pending_store_global_stack_value_publications`,
  `lower_store_global_value_publication`, and
  `lower_pointer_base_plus_offset_store_local_publication`.
- `dispatch_value_materialization.cpp` calls
  `find_latest_narrow_store_for_wide_local_load` and
  `emit_prepared_pointer_value_load_to_register`; the direct
  `find_latest_narrow_store_for_wide_local_load` caller must move to prepared
  recovered-source authority with the local-store path.
- `backend_aarch64_prepared_memory_operand_records_test.cpp` calls
  `lower_store_local_value_publication` directly for store-source publication
  coverage.
- The semantic helpers also depend on `dispatch_producers.*` same-block
  producer queries and `dispatch_publication.*` prepared local-load/byval
  helpers.

Focused test handles:

- `backend_store_source_publication_plan`: target-neutral store-source plan
  identity, recovered source, source producer, pointer writeback, global
  pending flags, and fail-closed statuses.
- `backend_aarch64_prepared_memory_operand_records`: direct
  `lower_store_local_value_publication` coverage and prepared cast producer
  consumption.
- `backend_aarch64_instruction_dispatch`: local conversion store publication,
  stack-homed scalar/select local and global store publication, selected global
  load before local store, binary snapshot local-store publication, wide local
  load from latest narrow byte store, pointer-base/global store cases, and
  missing store-source authority diagnostics.

## Suggested Next

Recommended first semantic implementation packet: extend the local store-source
plan construction to pass the existing generic prepared source producer lookup
instead of only casts, then replace local-store select/binary/cast gating in
`lower_store_local_value_publication` with `PreparedStoreSourcePublicationPlan`
producer facts where the existing prepared authority is complete. Leave
narrow-store recovery, byval load-local recovery, direct-global select-chain
recovery, and pointer-base load-local fallback untouched until separate facts
are added or an explicit fail-closed policy is chosen.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
Treat generic producer-kind consumption as a first cleanup of existing prepared
facts, not as proof that the missing recovered-source/direct-global/pointer-base
facts are solved.

## Proof

No build required for inventory-only work.
