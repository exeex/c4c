Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Global Store And Pointer-Base Recovery To Prepared Authority

# Current Packet

## Just Finished

Step 3 packet completed for pointer-base load-local fallback removal.

- Removed the same-block `LoadLocalInst` producer fallback from
  `emit_pointer_base_plus_offset_to_register`.
- `emit_pointer_base_plus_offset_to_register` now emits pointer-base
  materialization only from prepared authority that already exists: global
  symbol spelling or prepared value-home data for the pointer base.
- The helper fails closed when the pointer base has no prepared value home
  instead of recovering a stack load source from AArch64-local same-block
  producer state.
- Removed the now-unused `instruction_index` parameter from the helper and its
  single caller.

## Suggested Next

Next packet: inventory whether Step 3 now has any remaining AArch64-local
semantic source recovery before returning to the original idea 39 mechanical
fold-back decision.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
Recovered narrow-store source, byval load-local classification, and
direct-global select-chain dependency now use shared prepared helper code, but
the relations are still computed on demand rather than persisted as precomputed
prepared tables. Pointer-base store-local publication now depends on existing
global symbol or prepared value-home authority and fails closed otherwise.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 focused tests green.
