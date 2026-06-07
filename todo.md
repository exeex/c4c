Status: Active
Source Idea Path: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Expose Materialized Compare And Current-Block Publication Facts

# Current Packet

## Just Finished

Step 4 - Expose Materialized Compare And Current-Block Publication Facts moved
current-block entry-publication register lookup out of `comparison.cpp` and
exposed the missing destination-register fact on the shared current-block join
query.

Migrated facts:

- `prepare::PreparedCurrentBlockJoinParallelCopySourceFact` now carries the
  block-entry move's `destination_register_name`, so shared prepared query
  consumers can observe the current-block publication register spelling without
  re-reading the move.
- `backend_prepared_lookup_helper` proves the current-block join query exposes
  the destination register name on an available block-entry fact.
- `comparison.cpp` no longer collects or rematches current-block
  `PreparedBlockEntryPublication` records locally; it delegates register lookup
  and publication-presence checks to the reusable AArch64 dispatch-publication
  adapter.
- The existing AArch64 adapter remains the owner for parsing AArch64 register
  names, coercing register views, and preserving the fallback contract for
  prepared block-entry publication facts that do not have edge-publication
  join-transfer lookups.

Keep-local decisions:

- AArch64 register parsing, register-bank filtering, register view coercion,
  compare/branch emission, and fallback policy stay target-local in the
  dispatch-publication and comparison lowering code.
- Materialized compare join-target lookup remains shared prepared-authority
  based through the existing prepared conditional branch facts path.

## Suggested Next

Delegate Step 5 as a final audit/closure package: check `comparison.cpp` for
remaining prepared-contract ownership drift, verify the Step 1-4 facts line up
with the source idea, run the supervisor-selected acceptance subset or
regression guard, and either prepare closure notes or identify the smallest
remaining route gap.

## Watchouts

- `comparison.cpp` still has context adapters for fallback construction of
  prepared producer lookups when prebuilt function lookups are absent; those
  adapters are fallback plumbing, not the fact ownership itself.
- Do not force the dispatch-publication adapter through edge-publication
  join-transfer lookups only: focused AArch64 dispatch coverage still relies on
  the already-authoritative `PreparedBlockEntryPublication` facts when no
  join-transfer lookup exists.

## Proof

Passed. Exact delegated proof command:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepare_authoritative_join_ownership|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure) > test_after.log 2>&1`

Result: `100% tests passed, 0 tests failed out of 5`; proof log is
`test_after.log`.

Supervisor acceptance validation:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: monotonic focused regression guard passed with no pass/fail delta, and
the broader backend subset passed `179/179`.
