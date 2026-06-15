Status: Active
Source Idea Path: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Fixture Binding

# Current Packet

## Just Finished

Step 1 - Locate The Fixture Binding completed as a read-only inspection.
Inspected `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`,
`src/backend/mir/riscv/codegen/emit.cpp`,
`src/backend/mir/riscv/codegen/emit.hpp`,
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/prealloc/addressing.hpp`, and supporting generic helper coverage in
`tests/backend/mir/backend_store_source_publication_plan_test.cpp` and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Current binding point: `make_load_local_dynamic_stack_source(...)` installs the
compatible dynamic `LoadLocal` BIR instruction and pushes the supplied
`PreparedMemoryAccess` into `prepared.addressing.functions.front().accesses`;
`make_lookups(...)` then calls `prepare::make_prepared_function_lookups(...)`,
which builds both the public `PreparedFunctionLookups::memory_accesses` row and
the edge-publication source-memory pointer from the same prepared addressing
row.

Compatible public row: `load_local_source_access(ids)` for source value id `1`
and value name `%src`, at block `left`, instruction index `0`, pointer base
`%base`, offset `12`, size `4`, align `4`, base-plus-offset enabled. The
compatible assertion is that
`find_unique_indexed_prepared_memory_access_by_result_value_id(&lookups.memory_accesses, 1)`
returns the exact same row pointer as `publication->source_memory_access`, and
RISC-V emits `lw a1, 12(s2)`.

Current Route 3 / Route 5 authority row: `make_route5_edge_identity_function(ids,
true)` constructs the route-side `LoadLocal` with pointer base `%base`, offset
`12`, size `4`; `bir::route3_find_memory_access_record(...)` names the Route 3
`load_local` / `pointer_value` memory row, and
`bir::route5_cfg_edge_publication_record(...)` produces the Route 5
`memory_source` edge whose `source_memory_access` must agree with the prepared
publication before the RISC-V consumer accepts the dynamic source.

Intended supported extension point for Step 2: add a narrow fixture option at
the supported prepared-construction surface around
`make_load_local_dynamic_stack_source(...)` / `load_local_source_access(...)`
that publishes a stale public `PreparedFunctionLookups::memory_accesses` row
through normal `PreparedAddressingFunction::accesses` and
`make_prepared_function_lookups(...)`. The extension should not clear or edit
lookup maps after construction; it should create distinguishable prepared
addressing rows while preserving the compatible default fixture and allowing
the route-side `make_route5_edge_identity_function(ids, true)` authority row to
remain current.

## Suggested Next

Start Step 2 - Add Supported Stale Fixture Construction by adding the minimal
fixture option/helper at the RISC-V prepared edge-publication fixture surface
identified above.

## Watchouts

- Do not accept synthetic post-construction mutation as fixture support.
- Preserve the compatible `LoadLocal` fixture and exact `lw a1, 12(s2)` output.
- Keep final stale-publication fail-closed proof out of scope unless the
  supervisor explicitly folds it into this runbook.
- The current negative in the fixture clears
  `lookups.memory_accesses.accesses_by_result_value_id`; that remains useful as
  a fail-closed check but is not an acceptable stale-fixture construction
  mechanism.
- The RISC-V consumer currently rejects dynamic `LoadLocal` Route 5 evidence
  when `route5_edge_source_agrees_with_prepared_publication(...)` fails, so Step
  2 should make the stale/current relationship observable without relying on
  post-construction route or lookup mutation.

## Proof

No build/test proof required for this read-only inspection packet. No
`test_after.log` update was made.
