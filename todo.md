Status: Active
Source Idea Path: ideas/open/363_rv64_object_parameter_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the Parameter-Home Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by defining the RV64 object-route contract for the
audited byval aggregate parameter stack-slot home.

The object admission check now distinguishes:

- supported ordinary formal homes that resolve to RV64 GPR registers
- unsupported byval aggregate formal homes whose prepared parameter home is a
  stack slot
- the existing generic `unsupported_param_home` fallback for other unsupported
  formal-home classes

The selected contract is a precise unsupported diagnostic rather than semantic
lowering because the current object route has no ABI contract that turns the
callee byval home slot into a valid lowered aggregate parameter address.
GPR formal behavior remains admitted through the same `gpr_register_number_for_home`
path.

Focused coverage added a synthetic RV64 prepared module for the `%p.pa`-class
shape: a byval pointer formal with prepared `source_kind=byval_param` stack
object facts and a `PreparedValueHomeKind::StackSlot` home. The test now
expects:

```text
unsupported_byval_param_home: RV64 object route does not yet lower byval aggregate parameter homes in prepared stack slots
```

Files changed:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

## Suggested Next

Run Step 4 representative validation for `src/20030914-2.c` and record whether
it now reports the precise byval stack-slot parameter-home diagnostic or
advances to a later structured boundary.

Suggested representative command:

```sh
build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030914-2.c
```

## Watchouts

- Treat `src/20030914-2.c` as a representative signal, not an implementation
  key.
- Do not treat the new diagnostic as support for all stack-slot formals; sret
  stack-slot formals remain a separate contract.
- Do not conflate callee byval formal homes with caller aggregate-address
  call-argument transport.
- Do not reopen scalar vararg, `va_arg`, `va_copy`, or variadic helper work
  from idea 362 and its follow-ups.
- The next packet should not downgrade allowlists or expectations; it should
  only record the representative outcome against the structured diagnostic.

## Proof

AST-backed lookup used:

- `prepared_param_homes_supported` was defined in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and called only by
  `prepared_function_to_object_function()`.
- Its direct callee chain resolved through `gpr_register_number_for_value()` and
  `gpr_register_number_for_home()`.

Ran delegated proof:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: passed, with `test_after.log` containing 3/3 passing tests.
