Status: Active
Source Idea Path: ideas/open/275_riscv_memory_accesses_stale_publication_fixture_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Supported Stale Fixture Construction

# Current Packet

## Just Finished

Step 2 - Add Supported Stale Fixture Construction completed.
Changed `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` and
`todo.md`. Added a vector-backed
`make_load_local_dynamic_stack_source(...)` fixture path so multiple
`PreparedMemoryAccess` rows can be supplied through normal
`PreparedAddressingFunction::accesses` construction before
`make_prepared_function_lookups(...)`. Added
`stale_public_load_local_source_access(...)` and replaced the old
post-construction `accesses_by_result_value_id.clear()` negative with a
supported stale-public fixture that exposes both the current position-selected
row and a separately identifiable stale public row by source value id. The
compatible default fixture still asserts the unique public row and `lw a1,
12(s2)` output, and the Route 3 / Route 5 authority-row test remains separate.

## Suggested Next

Start the next Step 2 or Step 3 packet by deciding whether to add a dedicated
Route 3 / Route 5 stale-public fail-closed proof that uses the supported
fixture, or move on if the supervisor considers the construction proof enough.

## Watchouts

- The supported stale fixture currently makes the value-id public lookup
  non-unique, so `consume_edge_publication_move_intent(...)` rejects it before
  rendering. That is construction-backed and no longer depends on clearing
  lookup maps after construction.
- The stale row uses the same source value id but a different block-position
  key and offset `16`; the current publication remains tied to the real
  `LoadLocal` position at `left`, instruction `0`, offset `12`.
- No backend/prealloc implementation files were changed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' | tee test_after.log`
passed. Proof log: `test_after.log`.
