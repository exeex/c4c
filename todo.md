Status: Active
Source Idea Path: ideas/open/170_route4_block_entry_publication_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Proven-Private Prepared Surface

# Current Packet

## Just Finished

Step 4 evidence pass completed for the prepared block-entry publication helper
surface after the Step 3 MIR migration. No contraction is safe now.

Scanned declarations/definitions and direct consumers:
- `PreparedBlockEntryPublicationStatus` and
  `prepared_block_entry_publication_status_name(...)`: still used by
  `src/backend/prealloc/prepared_printer/value_locations.cpp` and oracle tests.
- `PreparedBlockEntryPublication`: still public through
  `src/backend/prealloc/value_locations.hpp`,
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/mir/x86/prepared/prepared.hpp`, AArch64 dispatch helpers, x86
  publication-plan tests, and Route 4 oracle tests.
- `PreparedBlockEntryPublication` fields `status`, `home`,
  `destination_value_id`, `destination_kind`, `destination_storage_kind`, and
  `destination_register_name`: still read by production AArch64 current-block
  entry publication paths and/or scalar publication planning. `bundle` and
  `move` remain observable through helper tests and are tied to the public
  helper result shape.
- `PreparedCurrentBlockEntryPublicationStatus` and
  `prepared_current_block_entry_publication_status_name(...)`: status remains
  part of the public prepared query result and is still asserted by Route 4
  oracle tests.
- `PreparedCurrentBlockEntryPublicationQueryInputs` fields `names`,
  `regalloc`, `value_locations`, `value_home_lookups`, and `successor_label`:
  still required by `find_prepared_current_block_entry_publication(...)`
  callers in AArch64 current-block entry publication code and oracle tests.
- `PreparedCurrentBlockEntryPublication` fields `status`, `publication`,
  `destination_home`, `destination_value_id`, and `destination_value_name`:
  still consumed by AArch64 publication-register selection and Route 4 oracle
  tests.
- `prepared_block_entry_publication_available(...)`: still used by
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`, and tests.
- `collect_prepared_block_entry_publications(...)`: still used by
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/prepared_printer/value_locations.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
  `src/backend/mir/x86/prepared/prepared.hpp`, and tests.
- `find_prepared_current_block_entry_publication(...)`: still used by
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` and prepared/Route
  4 oracle tests. AST query also confirmed the `bir::Value` overload delegates
  to the `PreparedValueId` overload inside `src/backend/prealloc/prepared_lookups.cpp`.
- `PreparedScalarPublicationPlan::current_block_entry_publication`,
  `PreparedScalarPublicationPlan::current_block_entry_publication_available`,
  and `PreparedScalarPublicationInputs::current_block_entry_publication`: still
  part of scalar publication planning in
  `src/backend/prealloc/publication_plans.cpp`, AArch64 comparison/value-home
  materialization, and x86 publication-plan coverage.

Decision: keep the prepared block-entry publication helper surface public. The
Step 3 MIR migration removed the selected MIR semantic consumer, but remaining
current-block, target/codegen, printer, x86 wrapper, scalar publication-plan,
and oracle consumers are direct evidence against hiding or narrowing the helper
surface in this packet.

## Suggested Next

Proceed to Step 5 validation and handoff for the Route 4 migration slice. No
follow-up contraction packet is recommended from current evidence.

## Watchouts

- Direct prepared helper consumers remain outside the migrated MIR query:
  `dispatch_publication.cpp`, `dispatch_producers.cpp`, prepared printer,
  x86 prepared wrapper, scalar publication planning, and oracle tests.
- No exact contraction-owned files or contraction proof command are recorded
  because no helper or field is proven private.
- Do not treat test-only uses as the only blocker; there are still production
  target/codegen and printer consumers.

## Proof

No build or tests required for this analysis-only packet. Used `rg` plus
`c4c-clang-tool`/`c4c-clang-tool-ccdb` direct symbol queries. `test_after.log`
was not touched.
