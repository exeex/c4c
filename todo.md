# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve recursive aggregate facts and all aggregate forms

## Just Finished

- Plan Step 1 completed in `a50d35d4e` (`lir: register aggregate lowering
  refs`): `lower()` registers each HIR aggregate definition before lowering,
  and `build_type_decls` consumes that definition-owned HIR ref to intern the
  owning LIR module's aggregate store. Repeated declaration lowering reuses
  the entry; missing, incomplete, and foreign refs fail closed. Legacy
  owner-key/tag occurrence adapters remain untouched.

## Suggested Next

- Begin Step 2 by inventorying named, anonymous, local, template, and
  typedef/alias aggregate occurrence producers; migrate one bounded producer
  while preserving ordered fields, layout, projections, and recursive children.

## Watchouts

- `build_type_decls` is the registration-before-use precedent and currently
  the only migrated producer. Do not recover refs from tags, parser pointers,
  rendered text, or owner keys; retain legitimate no-owner rendered
  compatibility only at its named consumer.

## Proof

- Fresh `cmake --build --preset default` plus
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 6/6 at
  `a50d35d4e`; the canonical guard accepted the packet with
  `--allow-non-decreasing-passed` (log: `test_after.log`).
