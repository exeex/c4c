Status: Active
Source Idea Path: ideas/open/335_aarch64_uninitialized_local_slot_runtime_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Initialization Or Publication Owner

# Current Packet

## Just Finished

Step 2 repaired the AArch64 scalar-consumer publication path for
`bir.load_local` values whose prepared result homes are not already live in an
emitted scalar register. Scalar consumers now retarget unpublished load-local
operands to the prepared source frame-slot access when there is no intervening
store to the same local byte range, covering stack result homes and GP register
homes that may otherwise contain stale call or scratch values. The direct scalar
call-argument materializer no longer records a load-local prepared register as
emitted without generating the load first.

Focused coverage was added for a prepared block where scalar operands consume
one load-local result assigned to a stack home and another assigned to a GP
register home; the lowered scalar record now reads the original local frame
slots instead of the unpublished result homes. The machine-printer guard also
covers a stack-result `mul` with two frame-slot operands so load-local source
materialization can use the result scratch register for the first operand and
still materialize the second operand.

`c_testsuite_aarch64_backend_src_00164_c` now passes. The old scalar
operand-printer diagnostics did not return in the delegated proof.

## Suggested Next

Supervisor should review the Step 2 diff and commit the completed repair slice
with the updated focused tests and `todo.md`. If more validation is desired,
the next coherent packet is broader AArch64 backend regression only, not another
`00164.c` localization packet.

## Watchouts

The repair intentionally refuses to retarget across an intervening store that
may alias the local-load byte range. That preserves load snapshot semantics and
keeps the fix semantic rather than offset- or testcase-shaped. The slice touches
the scalar consumer and dispatch materialization path, not c-testsuite
expectations or unsupported classifications.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll backend_aarch64_memory_operand_records_test backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_machine_printer_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(memory_operand_records|scalar_alu_records|prepared_scalar_alu_records|machine_printer)|c_testsuite_aarch64_backend_src_00164_c' > test_after.log 2>&1`.
The build succeeded, all 5 selected tests passed, and `test_after.log` is the
canonical proof log.
