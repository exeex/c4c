Status: Active
Source Idea Path: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Function Pointer Coverage

# Current Packet

## Just Finished

Step 2 focused function-pointer coverage for idea 315.

Added expected-repair backend cases:

- `riscv64_function_pointer_local_storage.c`: stores a function address through
  a local struct function-pointer field, reloads it through a scalar local
  function pointer, and performs a runtime indirect call.
- `riscv64_function_pointer_return_chain.c`: returns a function pointer from a
  selector helper, stores/reloads the returned pointer in `main`, and performs
  a chained runtime indirect call.

Registered focused coverage:

- `backend_dump_riscv64_function_pointer_local_storage_expected_repair`
- `backend_codegen_route_riscv64_function_pointer_local_storage_expected_repair`
- `backend_dump_riscv64_function_pointer_return_chain_expected_repair`
- `backend_codegen_route_riscv64_function_pointer_return_chain_expected_repair`

The dump tests assert the prepared semantic shape, including direct-global
function-address metadata and indirect call plans. The codegen-route tests
assert the current RV64 truncation/gap and forbid direct-call substitution or
completed `jalr` emission, keeping the suite green before the repair.

## Suggested Next

Proceed to Step 3 by repairing RV64 prepared function-address materialization
for pointer-typed direct function symbols when they are stored, returned, or
used as values before indirect calls.

## Watchouts

- Do not special-case filenames, symbol names, field offsets, or local
  aggregate shapes.
- Do not replace indirect calls with direct calls unless the function-pointer
  semantics are still genuinely preserved.
- The new expected-repair route tests intentionally forbid `call add_three`,
  `call sub`, and `jalr` in the current truncated output. Step 3 should flip
  these toward successful semantic runtime coverage only after function-pointer
  value materialization and indirect-call emission are implemented.
- `src/00124.c` has multiple gaps. Keep the next repair boundary on direct
  function-address materialization first, then re-evaluate indirect `jalr` and
  null pointer return handling with focused evidence.
- Keep external empty-stub policy, broad aggregate repair, and unrelated global
  data object work out of scope.

## Proof

Ran focused coverage:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_function_pointer_(local_storage|return_chain)_expected_repair'`

Focused result:

- Build passed.
- Focused Step 2 tests passed: 4/4.

Ran delegated proof:

- `cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Delegated backend proof wrote `test_after.log`.
- Backend proof failed with `backend_riscv_prepared_edge_publication` as the
  only failing backend test; the new Step 2 function-pointer tests passed.
