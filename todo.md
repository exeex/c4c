Status: Active
Source Idea Path: ideas/open/111_store_source_publication_dump_visibility.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Bounded Store-Source Publication Dump Output

# Current Packet

## Just Finished

Step 2 added bounded prepared dump output for retained store-source
publication facts.

Output surface: `prepare::print(const PreparedBirModule&)` now emits
`--- prepared-store-source-publications ---` between call plans and select-chain
materializations. Rows use
`store_source function=<name> block=<label> inst=<index> source=<value>` plus
`status`, `intent`, `source_producer`, optional `source_producer_block`,
optional `source_producer_inst`, `source_load_local`, `source_load_global`,
`source_cast`, `source_binary`, `source_select`,
`direct_global_select_chain`, `direct_global_root_is_select`, and optional
`direct_global_root_inst`. The row does not print raw producer pointers or
destination/publication internals.

Access path: `PreparedBirModule` now retains
`PreparedStoreSourcePublicationPlans store_source_publications`; contract
publication calls `populate_store_source_publication_plans(prepared_)`.
Population walks existing prepared BIR stores and builds each row's
`PreparedStoreSourcePublicationPlan` through `plan_prepared_store_source_publication`
using existing prepared addressing, value-home, source-producer, recovered
source, byval, and direct-global select-chain helpers. The printer consumes only
the retained plan fields.

## Suggested Next

Add the focused Step 3 contract-test packet in
`tests/backend/bir/backend_prepared_printer_test.cpp`: assert the new
`prepared-store-source-publications` header and concrete row fields for one
source-producer case and one direct-global select-chain case, reusing the
existing prepared-printer fixture style.

## Watchouts

- The printer row lives in `prepared_printer/select_chains.cpp` because some
  BIR tests link a fixed prepared-printer helper list; adding a new helper
  translation unit caused a link gap.
- Contract tests should assert concrete producer/direct-global fields, not just
  the section header.
- Preserve missing-source and missing-destination-access fail-closed behavior;
  do not move semantic planning into the printer.

## Proof

Ran focused proof:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepared_printer|backend_store_source_publication_plan)$"' > test_after.log 2>&1`;
passed.

Supervisor broader backend proof:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`;
passed 179 of 179 backend tests. Proof log: `test_after.log`.
