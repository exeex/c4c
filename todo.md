Status: Active
Source Idea Path: ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove The 9-Case Bucket

# Current Packet

## Just Finished

Step 3 proved/classified the full 9-case AArch64 scalar cast bucket after the
register-source publication repair, without changing code, expectations, CTest
registration, runner scripts, or lifecycle files outside this `todo.md`
packet.

The old scalar-cast structured-register-source diagnostic is absent from
`test_after.log`. The internal scalar-cast, prepared scalar-cast, machine
printer, and instruction-dispatch tests all pass.

9-case c-testsuite result:

- `00143`: pass.
- `00173`: timeout after 5.02 seconds; first bad fact is nontermination in the
  Tower of Hanoi run, not a scalar-cast printer/source diagnostic.
- `00175`: `RUNTIME_MISMATCH`; first bad fact is scalar conversion/call result
  corruption, with `charfunc(99.0)` printing an empty char, `intfunc(99.0)`
  printing a large negative value, and float conversions printing zeroes.
- `00176`: `RUNTIME_MISMATCH`; first bad fact is quicksort corrupting the
  sorted global integer array into stack/pointer-looking values after the
  initial unsorted print is correct.
- `00181`: `RUNTIME_NONZERO`; first bad fact is a segmentation fault in the
  pointer/string copy test.
- `00185`: pass.
- `00204`: `RUNTIME_MISMATCH`; first bad fact is later AArch64 ABI/stdarg/HFA
  output corruption after the initial argument and return-value sections match.
- `00205`: `RUNTIME_MISMATCH`; first bad fact is global aggregate
  initialization corruption, with every `PT cases[]` element printing the same
  repeated values instead of the expected constants.
- `00216`: `RUNTIME_MISMATCH`; first bad fact is broad struct/union/compound
  initializer corruption beginning at the first printed global object `ce`.

## Suggested Next

Choose the next implementation packet from the residual first-bad-fact
families above. A narrow next packet could target the real scalar-conversion
residual in `00175`; the larger aggregate/ABI failures in `00204`, `00205`,
and `00216` appear to be separate capability work rather than evidence that
the scalar-cast register-source publication repair failed.

## Watchouts

- Do not reopen closed owners 334 through 337 from counts alone.
- Keep the scalar-cast publication repair classified as proven for the old
  diagnostic: the structured register-source failure string does not appear in
  `test_after.log`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Passing cases are `00143` and `00185`; failing residuals are runtime,
  timeout, or segfault classifications only.
- `00216` remains advanced past the old scalar-cast publication failure and is
  now a broad initializer/runtime-output mismatch.

## Proof

Delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(scalar_cast_records|prepared_scalar_cast_records|machine_printer|instruction_dispatch)|c_testsuite_aarch64_backend_src_(00143|00173|00175|00176|00181|00185|00204|00205|00216)_c' > test_after.log 2>&1`

Result: build passed; CTest subset was 6/13. Passing tests:
`backend_aarch64_scalar_cast_records`,
`backend_aarch64_prepared_scalar_cast_records`,
`backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00143_c`, and
`c_testsuite_aarch64_backend_src_00185_c`.

Residual failures: `00173` timed out; `00181` failed with
`RUNTIME_NONZERO`/segmentation fault; `00175`, `00176`, `00204`, `00205`, and
`00216` failed with `RUNTIME_MISMATCH`. `test_after.log` is the preserved proof
log.
