Status: Active
Source Idea Path: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Contract Coverage

# Current Packet

## Just Finished

Step 3 identified the focused RV64 object-emission coverage surface for the
Step 2 RV64-owned boundary without leaving a failing tracked test.

Existing focused coverage is adjacent but not exact:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  `builds_prepared_pointer_value_scalar_local_object`,
  `builds_prepared_pointer_value_scalar_local_store_with_t1_base_object`,
  `builds_prepared_pointer_value_i8_local_store_object`, and
  `builds_prepared_pointer_value_f64_local_object` cover prepared
  pointer-value base-plus-offset local-memory consumers when the pointer base
  already has a register home.
- `builds_prepared_sret_stack_pointer_store_object` covers a stack-homed
  pointer-value base for an sret parameter store, but that fixture is sret-home
  specific and is not the prepared local pointer-value stack-home boundary from
  the `src/20000519-1.c` local-memory failure.

Temporary red probe evidence:

- Added then removed a local-only probe in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`:
  `make_prepared_pointer_value_scalar_stack_home_local_module()` cloned
  `make_prepared_pointer_value_scalar_local_module()`, changed only the
  pointer base `%p` home from `Register` to a coherent `StackSlot`, added the
  matching prepared stack object/frame slot/frame plan, and kept the existing
  pointer-value base-plus-offset store/load accesses.
- Added then removed
  `builds_prepared_pointer_value_scalar_stack_home_local_object()`, expecting
  `rv64::build_rv64_prepared_text_object_module(prepared)` to succeed.
- The probe failed before repair with exit status `1` and stderr:
  `expected prepared pointer-value stack-home local RV64 object module to build`.
- Evidence files are under
  `build/agent_state/561_step3_pointer_value_stack_home_boundary/`.

Smallest future tracked test surface after the repair starts: add the same
stack-home variant beside `make_prepared_pointer_value_scalar_local_module()`
and register it in `main()` next to the existing pointer-value local object
tests. Its fixture should remain independent of `src/20000519-1.c` and should
assert the emitted sequence materializes the pointer base from the prepared
stack slot before using the existing pointer-value base-plus-offset load/store
path.

## Suggested Next

Execute Step 4 by teaching the RV64 prepared local-memory consumer to
materialize a pointer-value base from a coherent prepared stack home, then add
the focused stack-home pointer-value local object test from this packet as the
first tracked proof.

## Watchouts

- Keep the repair semantic: do not special-case `src/20000519-1.c`; the route
  should handle prepared pointer-value base-plus-offset accesses whose pointer
  base has a coherent stack-slot home.
- Do not reconstruct local-memory facts from RV64 target-specific instruction
  shapes.
- Do not combine this route with direct-call metadata repair.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not use named-case shortcuts for retained torture representatives.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.
- The existing sret stack-home pointer-value object test is not enough by
  itself for this route because it depends on sret-home publication and does not
  exercise a normal prepared local pointer-value stack home.
- The temporary Step 3 probe intentionally failed before repair and was removed
  from tracked tests; re-add it only with the Step 4 consumer repair.

## Proof

Step 3 validation:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && git diff --check -- todo.md
```

Result: passed. The backend subset completed successfully and
`git diff --check -- todo.md` passed. Red probe evidence was captured before
the tracked test was removed:
`build/agent_state/561_step3_pointer_value_stack_home_boundary/red_backend_riscv_object_emission.status`
contains `1`, and
`build/agent_state/561_step3_pointer_value_stack_home_boundary/red_backend_riscv_object_emission.stderr`
contains the expected stack-home local object build failure.
