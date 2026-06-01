# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Relocate one narrow owner-local publication family

## Just Finished

Completed Step 2 from `plan.md`: relocated
`value_power_of_two_shift` and
`value_publication_may_write_scratch_register` from the private
`dispatch_value_materialization.cpp` helper surface into the AArch64 ALU
owner. The helpers are now declared in `alu.hpp`, defined in `alu.cpp`, and
`emit_value_publication_to_register` passes dispatcher-owned prepared lookup
callbacks into the ALU-owned scratch classifier.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `todo.md`

The dispatcher still owns `prepared_same_block_integer_constant` and
`prepared_same_block_scalar_producer`; ALU consumes those facts through typed
callbacks instead of re-deriving prepared producer authority.

## Suggested Next

Execute the next Step 2 relocation packet against another narrow owner-local
publication branch that can consume existing prepared facts without moving the
shared prepared lookup helpers; comparison-owned or memory-owned publication
branches are likely candidates for supervisor review.

## Watchouts

- Keep `emit_value_publication_to_register` in place until more owner-local
  helper branches are extracted.
- Do not move the prepared producer lookup helpers unless the packet creates a
  shared prepared-authority surface; they are still consumed across scalar,
  globals, select, comparison, and memory paths.
- The ALU scratch classifier intentionally takes dispatcher-owned lookup
  callbacks. Moving `prepared_same_block_integer_constant` or
  `prepared_same_block_scalar_producer` into ALU would widen ALU into shared
  prepared producer authority.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
