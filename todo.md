# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve recursive aggregate facts and all aggregate forms

## Just Finished

- Plan Step 2 declaration/store fact capture completed: `build_type_decls`
  snapshots each registered aggregate's ordered structured LIR field types,
  packed/opaque flags, and direct/byte-storage/union layout kind into its
  canonical module-owned store entry before recording the declaration. Focused
  lowering coverage proves a nested typed aggregate child, direct padding,
  union byte storage, packed byte storage, and the existing repeated/missing/
  incomplete/foreign fail-closed cases.

## Suggested Next

- Continue Step 2 with one bounded aggregate occurrence producer migration,
  using the store's captured declaration facts without widening into consumer
  migration.

## Watchouts

- Fact capture is declaration/store-only: downstream consumers and legacy
  owner-key/tag occurrence adapters remain untouched. Do not recover refs or
  fields from tags, parser pointers, rendered text, or owner keys.

## Proof

- Fresh `cmake --build --preset default` plus
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 6/6;
  `test_after.log` is the proof log.
