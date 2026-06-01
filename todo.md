# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Relocate one narrow owner-local publication family

## Just Finished

Completed Step 2 from `plan.md`: relocated
`emit_prepared_global_load_to_register` from the private
`dispatch_value_materialization.cpp` helper surface into the AArch64 globals
owner. The helper is now declared in `globals.hpp`, defined in `globals.cpp`,
and `emit_value_publication_to_register` continues to call it through the
globals-owned declaration.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `todo.md`

The caller still obtains the prepared same-block global-load access from the
existing prepared producer facts and passes the prepared `LoadGlobalInst` plus
`PreparedMemoryAccess` into the relocated helper; no prepared authority was
re-derived in globals.

## Suggested Next

Execute the next Step 2 relocation packet by moving the scalar/ALU arithmetic
publication helper family (`value_power_of_two_shift` and the private
`value_publication_may_write_scratch_register`) toward the scalar/ALU owner,
with the packet scoped to declarations and call-site wiring needed by
`emit_value_publication_to_register`.

## Watchouts

- Keep `emit_value_publication_to_register` in place until more owner-local
  helper branches are extracted.
- Do not move the prepared producer lookup helpers unless the packet creates a
  shared prepared-authority surface; they are still consumed across scalar,
  globals, select, comparison, and memory paths.
- The globals helper still depends on scalar register spelling and memory load
  mnemonic/address formatting helpers; that dependency existed before the move
  and was not widened.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
