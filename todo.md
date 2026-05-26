Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Global Store And Pointer-Base Recovery To Prepared Authority

# Current Packet

## Just Finished

Step 3 packet completed for direct-global select-chain dependency authority.

- Added shared prepared helper
  `find_prepared_store_source_direct_global_select_chain_dependency`, returning
  the direct-global dependency fact plus root-select instruction metadata used
  by local store-source publication.
- Extended `PreparedStoreSourcePublicationPlan` with
  `direct_global_select_chain_source` and root-select metadata so AArch64
  publication consumers can consume the dependency as plan data.
- `plan_store_local_source_publication` now populates the direct-global
  select-chain fields from the shared helper.
- `lower_store_local_value_publication` now gates and delays this route from
  `store_source_plan` fields instead of calling
  `select_chain_contains_direct_global_load` and rechecking the same-block
  producer locally.
- Added focused store-source publication plan coverage for the direct-global
  select-chain dependency fact.

## Suggested Next

Next semantic packet: address the pointer-base load-local fallback that still
uses AArch64-local producer recovery in the store-source publication path.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
The remaining semantic residue is still live: pointer-base load-local fallback.
Recovered narrow-store source, byval load-local classification, and
direct-global select-chain dependency now use shared prepared helper code, but
the relations are still computed on demand rather than persisted as precomputed
prepared tables.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 focused tests green.
