Status: Active
Source Idea Path: ideas/open/111_store_source_publication_dump_visibility.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Tighten Acceptance

# Current Packet

## Just Finished

Step 4 acceptance validation is complete for the store-source publication dump
visibility runbook.

Acceptance review:

- Step 2 printer output is bounded to retained
  `PreparedStoreSourcePublicationPlan` facts stored on `PreparedBirModule`; the
  printer formats stable names, instruction indices, statuses, booleans, and
  source-producer/direct-global fields without raw producer pointers.
- Step 3 tests assert concrete prepared-printer rows, not only the
  `prepared-store-source-publications` header:
  `source_producer=binary` for the `derived.seed` store-source row and
  `source_producer=select_materialization` plus
  `direct_global_select_chain=yes` / `direct_global_root_inst=1` for the
  direct-global select-chain store-source row.
- Existing `backend_store_source_publication_plan` coverage remains in the
  proof subset for source-producer planning, direct-global dependency
  detection, and missing-input fail-closed behavior.
- Diff review found no expectation downgrade, header-only contract,
  printer-side semantic re-derivation, or architecture expansion outside the
  source idea.

## Suggested Next

Recommend lifecycle review/closure consideration. The runbook steps are
complete and the backend acceptance proof is green.

## Watchouts

- The new retained `PreparedBirModule::store_source_publications` collection is
  populated during `publish_contract_plans()`, so future dump visibility
  changes should keep that population read-only with respect to semantic
  authority.
- If future work wants finer absent/incomplete dump rows, that is separate from
  this idea's bounded visibility contract and should not be silently expanded
  here.

## Proof

Step 3 focused proof:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_printer|backend_store_source_publication_plan)$"' > test_after.log 2>&1`;
passed 2 of 2 tests.

Step 4 supervisor acceptance proof:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`;
passed 179 of 179 backend tests. Proof log: `test_after.log`.
