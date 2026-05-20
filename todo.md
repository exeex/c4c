Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Stack-Homed Scalar Cast Register Source Publication

# Current Packet

## Just Finished

Step 2 corrected the scalar-cast prepared-consumer register-source publication
boundary after reviewer rejection. `make_prepared_consumer_register_source` now
admits stack-slot source homes only for the select-produced stack-home
materialization boundary this packet can prove: a same-block `bir::SelectInst`
defines the cast operand, the before-instruction move bundle contains a
`consumer_stack_to_register` value move from the select result value id to the
cast result value id, and the selected source register aliases the scalar cast
result register by bank and index. Direct stack-source scalar casts continue to
use the assembler load-plus-cast fallback.

`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now has
focused coverage that constructs a same-block select result stack-homed as an
i16 cast operand, adds the before-instruction consumer stack-to-register move
into `w0`, and asserts the lowered cast is a selected
`ScalarInstructionRecord` whose `scalar_cast->source` is a structured
`RegisterOperand` for the select value id/name in the consumer register. The
test also prints the route and verifies it no longer hits the old structured
register-source diagnostic. The direct stack-source fallback fixture keeps
i32-to-i64, i16-to-i32, and i8-to-i32 coverage on the assembler path.

## Suggested Next

Next packet should classify or repair the remaining `00143` runtime-nonzero.
The old scalar-cast structured-register-source diagnostic is gone, and the
delegated proof has no regressions beyond the classified `00143` residual.

## Watchouts

- The select restriction is intentional materialization safety for this slice:
  broadening stack-slot publication to all same-block producers or direct
  frame-slot casts regressed `00086` and `00111` at runtime because those paths
  still need explicit load materialization.
- The current `00143` failure is `[RUNTIME_NONZERO]`, not the old printer
  diagnostic.
- `test_after.log` contains the supervisor-selected proof output. The command
  exits nonzero only because `c_testsuite_aarch64_backend_src_00143_c` fails at
  runtime; `00086`, `00111`, and the four backend tests pass.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, plan files, or ideas.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer|scalar_cast_records|prepared_scalar_cast_records)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`

Build completed. `test_after.log` records 6/7 passing tests:
`backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`,
`backend_aarch64_scalar_cast_records`,
`backend_aarch64_prepared_scalar_cast_records`, `00086`, and `00111` pass.
`00143` fails with `[RUNTIME_NONZERO]` exit 1. The old
`scalar cast node requires a structured register source operand` diagnostic is
absent from `test_after.log`.
