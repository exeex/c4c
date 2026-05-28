Status: Active
Source Idea Path: ideas/open/62_aarch64_shared_edge_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Migrate Prepared Edge-Publication Consumers

# Current Packet

## Just Finished

Step 3 - Migrate Prepared Edge-Publication Consumers completed.

Migrated the AArch64 prepared edge-publication consumers in
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` to obtain source
identity through shared prepared edge-copy source facts:

- block-entry move clobber scanning now revalidates publication source facts
  with `prepare_edge_copy_source_facts(...)` before producer/materialization
  checks.
- copied machine-instruction move suppression now uses
  `prepare_edge_copy_source_facts(...)` by predecessor, successor, and
  destination value id, avoiding local publication recovery while preserving
  target-local redundant/source-move checks.
- predecessor select parallel-copy source materialization now uses
  `prepare_block_entry_parallel_copy_edge_source_facts(...)` for publication,
  source home, destination home, and producer facts before calling the existing
  AArch64 emission helper.

AArch64 still owns scratch selection, clobber-sensitive ordering, register
spelling, va-list carrier handling, and machine instruction emission. Missing
or mismatched shared facts now fail closed by skipping the prepared source
publication route or leaving ordinary target-local move emission in place; no
predecessor-depth or local producer recovery was added.

## Suggested Next

Step 4 - Remove Or Fail-Close Null-Publication Recovery: retire or hard
fail-close the remaining null-publication/local producer fallback paths once
the supervisor confirms this Step 3 consumer migration is accepted.

## Watchouts

`prepare_block_entry_parallel_copy_edge_source_facts(...)` requires the exact
prepared move object and is suitable when iterating the prepared move bundle.
The machine-instruction move suppression path receives a copied move record, so
it uses `prepare_edge_copy_source_facts(...)` plus the existing source-move
checks instead of the pointer-identity wrapper. Step 4 should focus on the
remaining null-publication/local producer fallback helpers, not on weakening
prepared fact requirements.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof passed: 169/169 backend tests passed. Log path: `test_after.log`.
