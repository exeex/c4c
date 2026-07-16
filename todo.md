# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the canonical aggregate identity and store seam

## Just Finished

- Plan Step 2 packet: migrated real aggregate declaration lowering through
  `build_type_decls`. `lower()` now registers each HIR aggregate definition
  before lowering, and `build_type_decls` consumes that definition-owned HIR
  ref to intern the owning LIR module's aggregate store. Repeated declaration
  lowering reuses the store entry; missing refs and incomplete/foreign refs
  fail closed. Legacy owner-key/tag occurrence adapters remain untouched.

## Suggested Next

- Select the next bounded aggregate occurrence producer for migration; retain
  this declaration producer as the registration-before-use precedent.

## Watchouts

- `build_type_decls` is now the only migrated producer. Do not recover refs
  from tags, parser pointers, rendered text, or owner keys when migrating
  later aggregate occurrences.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` passed (6 tests); log: `test_after.log`.
