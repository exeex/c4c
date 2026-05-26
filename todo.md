Status: Active
Source Idea Path: ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete Obsolete Store-Source Translation Unit

# Current Packet

## Just Finished

Steps 2-4 mechanically folded `memory_store_sources.*` into the AArch64 memory
owner. All definitions formerly in `memory_store_sources.cpp` now live in
`memory.cpp`; no-external-caller helper groups are namespace-local there, and
the still-public store-source publication surface is declared from
`memory.hpp`.

Updated external include sites to use the memory owner header: `dispatch.cpp`
and `dispatch_publication.cpp` no longer need a separate include,
`dispatch_value_materialization.cpp` includes `memory.hpp`, and
`backend_aarch64_prepared_memory_operand_records_test.cpp` includes
`memory.hpp`.

Deleted obsolete `memory_store_sources.cpp` and `memory_store_sources.hpp`.
Removed `memory_store_sources.cpp` from `src/backend/CMakeLists.txt`.
`rg -n "memory_store_sources" src/backend/mir/aarch64/codegen tests/backend/mir src/backend/CMakeLists.txt`
now reports no live source, test, or build metadata references.

## Suggested Next

Supervisor/plan-owner should decide whether idea 39 is complete or whether a
small review/broader guard is needed before lifecycle closure.

## Watchouts

This packet intentionally did not change prepared store-source behavior,
pointer-base value-home handling, memory diagnostics, or test expectations.
The only compile fallout was a local overload visibility issue after moving
`emit_scalar_conversion_cast_to_register` into `memory.cpp`; the fix adds the
same local two-argument `scalar_fp_register_view` view adapter already used by
the moved helper.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_memory_operand_contract)$' | tee test_after.log`

Result: passed, 5/5 tests. Proof log: `test_after.log`.

Additional checks:
`rg -n "memory_store_sources" src/backend/mir/aarch64/codegen tests/backend/mir src/backend/CMakeLists.txt || true`
returned no matches, and `git diff --check` passed.
