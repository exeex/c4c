Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Local Store Source Recovery To Prepared Authority

# Current Packet

## Just Finished

Step 2 packet completed for byval load-local source classification authority.

- Added shared prepared helper
  `prepared_store_source_load_local_is_byval_formal_pointer_source`, which
  classifies a prepared `LoadLocal` source producer through prepared memory
  access facts and BIR byval formal metadata.
- Extended `PreparedStoreSourcePublicationPlan` with
  `byval_load_local_source` so local-store publication can consume the
  classification as prepared plan data.
- `plan_store_local_source_publication` now populates the byval flag from the
  prepared source-producer/addressing records.
- `lower_store_local_value_publication` now gates this route with
  `store_source_plan.byval_load_local_source` instead of the AArch64-local
  `store_local_value_is_byval_frame_slot_load` same-block producer lookup.
- Removed the obsolete AArch64 declaration/definition for
  `store_local_value_is_byval_frame_slot_load`.
- Added focused store-source publication plan coverage for the prepared byval
  classification path.

## Suggested Next

Next semantic packet: address direct-global select-chain source dependency.
That route still uses AArch64-local same-block select-chain inspection at the
local store publication consumption site.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
The remaining semantic residue is still live: direct-global select-chain
source dependency and pointer-base load-local fallback. Recovered narrow-store
source and byval load-local classification now use shared prepared helper code,
but the relations are still computed on demand rather than persisted as
precomputed prepared tables.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 focused tests green.
