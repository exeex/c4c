Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Prerequisite Completion

# Current Packet

## Just Finished

Step 4 validation packet completed for the idea 39a prerequisite.

- Searched `memory_store_sources.*` for remaining original blocked-family
  source-choice residue:
  `find_same_block`, `producer_instruction_index`,
  `select_chain_contains_direct_global_load`, `prepared_local_load_offset`,
  `LoadLocalInst`, `StoreLocalInst`, `recovered`, `source_producer`,
  `PointerBasePlusOffset`, `byval`, `direct_global`, and `select_chain`.
- No remaining `memory_store_sources.*` calls to same-block producer recovery,
  local-load offset recovery, or direct-global select-chain AArch64-local
  inspection were found.
- Remaining matches are prepared plan/source-producer consumption,
  non-semantic load/store lowering, or shared prepared-helper result
  consumption.
- Step 4 is complete from the executor view: the 39a prerequisite no longer has
  the originally blocked AArch64-local store-source semantic recovery in
  `memory_store_sources.*`.

## Suggested Next

Next packet: hand lifecycle back to the plan owner to decide whether idea 39a
can close and whether idea 39 can resume its mechanical memory fold-back.

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

Search:

`rg -n "find_same_block|producer_instruction_index|select_chain_contains_direct_global_load|prepared_local_load_offset|LoadLocalInst|StoreLocalInst|same-block|same block|recovered|source_producer|PointerBasePlusOffset|byval|direct_global|select_chain" src/backend/mir/aarch64/codegen/memory_store_sources.cpp src/backend/mir/aarch64/codegen/memory_store_sources.hpp`

Proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed, 163/163 backend tests green. Proof log: `test_after.log`.
