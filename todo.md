Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Local Store Source Recovery To Prepared Authority

# Current Packet

## Just Finished

Step 2 packet completed for recovered narrow-store source authority.

- Added shared prepared helper
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`, returning
  the existing recovered-source payload shape used by
  `PreparedStoreSourcePublicationPlan`.
- Moved the slot/name/lane matching logic for narrow-store recovery out of
  AArch64 `memory_store_sources.*` and into shared prepared store-source
  planning.
- `plan_store_local_source_publication` now populates recovered source fields
  through the shared prepared helper, using the prepared `LoadLocal` source
  producer fact when it is complete for the current block.
- `lower_store_local_value_publication` now gates recovered-source publication
  by consuming `store_source_plan.recovered_source_value` instead of recomputing
  the narrow-store relation locally.
- The direct `dispatch_value_materialization.cpp` path now also uses the shared
  prepared recovered-source helper before emitting the stored narrow value.
- Removed the obsolete AArch64 public declarations and definitions for
  `find_latest_narrow_store_for_wide_local_load`,
  `store_local_value_is_wide_load_from_narrow_local_store`, and their private
  slot/lane matching helpers.

## Suggested Next

Next semantic packet: address byval load-local source classification. That
path still combines an AArch64 same-block producer lookup, prepared memory
access, and BIR byval parameter inspection in
`store_local_value_is_byval_frame_slot_load`.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
The remaining semantic residue is still live: byval load-local source
classification, direct-global select-chain source dependency, and pointer-base
load-local fallback. Recovered narrow-store source now uses shared prepared
helper code, but the relation is still computed on demand rather than persisted
as a precomputed prepared table.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 focused tests green.
