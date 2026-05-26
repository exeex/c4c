Status: Active
Source Idea Path: ideas/open/39a_aarch64_store_source_semantic_residue_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Local Store Source Recovery To Prepared Authority

# Current Packet

## Just Finished

Step 2 packet completed for local store-source producer authority.

- `plan_store_local_source_publication` now passes the existing generic
  prepared source producer lookup into
  `prepare::plan_prepared_store_source_publication` instead of passing only
  cast producers.
- `lower_store_local_value_publication` now gates complete cast, select, and
  supported scalar-FP binary store-source publication through
  `PreparedStoreSourcePublicationPlan` source producer facts.
- Removed the public declarations for the old local same-block select/binary
  producer probes because local-store gating no longer calls them.
- Left narrow-store recovery, byval load-local recovery, direct-global
  select-chain recovery, and pointer-base load-local fallback untouched. The
  direct-global select-chain duplicate-publication guard still uses the
  existing same-block path because this packet did not add shared prepared
  authority for that family.

## Suggested Next

Next semantic packet: move the narrow-store recovery relation toward prepared
authority. Start with `find_latest_narrow_store_for_wide_local_load` and its
`dispatch_value_materialization.cpp` caller, because the store-source plan can
already carry a recovered source value but AArch64 still computes that value
locally.

## Watchouts

Do not fold `memory_store_sources.*` into `memory.cpp` in this plan. Do not
rename local source rediscovery as prepared authority. Preserve diagnostics,
fail-closed behavior, ABI/memory semantics, and existing supported behavior.
The remaining semantic residue is still live: recovered narrow-store source,
byval load-local source classification, direct-global select-chain source
dependency, and pointer-base load-local fallback. Do not remove their local
queries until real shared prepared facts or explicit fail-closed policy exists.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 focused tests green.
