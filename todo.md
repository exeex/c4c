Status: Active
Source Idea Path: ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Register-Source Fact Publication

# Current Packet

## Just Finished

Step 2 repaired scalar cast register-source publication in
`lower_scalar_cast_instruction`.

The selected scalar cast handoff now recognizes prepared
`BeforeInstruction` value moves from the named cast operand into the cast
result register for `sign_extend` and `zero_extend`. When the prepared cast
record initially contains a frame-slot source, the handoff republishes the
consumer register as the structured source operand in both
`scalar_cast.source` and `inputs[0]` before `make_scalar_instruction`.

Focused dispatch coverage now constructs stack-sourced `SExt` and `ZExt`
casts with prepared consumer-register moves and verifies both selected-node
metadata and machine printing use the prepared consumer register. The source
register keeps the original source value identity while using the destination
physical register with the operand-width view, e.g. `x0` consumer authority
published as a `w0` source for `sxtw`.

## Suggested Next

Delegate Step 3 to prove/classify the remaining 9-case scalar cast bucket.
Start from the fact that `00143` now passes and `00216` has advanced past the
old scalar-cast structured-register-source printer diagnostic to a runtime
output mismatch.

## Watchouts

- Do not reopen closed owners 334 through 337 from counts alone.
- Keep the scope on scalar cast register-source operand facts unless the
  supervisor chooses the Step 3 residual-classification packet.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- The old scalar-cast structured register-source diagnostic is absent from
  `test_after.log`.
- `00216` is no longer a scalar-cast publication failure in this proof; it now
  compiles and runs but reports a broad runtime output mismatch. Treat that as
  residual first-bad-fact work, not as evidence that the source-publication
  repair failed.

## Proof

Delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(scalar_cast_records|prepared_scalar_cast_records|machine_printer|instruction_dispatch)|c_testsuite_aarch64_backend_src_(00143|00216)_c' > test_after.log 2>&1`

Result: build passed; CTest subset was 5/6. Passing tests:
`backend_aarch64_scalar_cast_records`,
`backend_aarch64_prepared_scalar_cast_records`,
`backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`c_testsuite_aarch64_backend_src_00143_c`.

Residual: `c_testsuite_aarch64_backend_src_00216_c` failed with
`RUNTIME_MISMATCH`, not the old scalar-cast structured-register-source
diagnostic. `test_after.log` is the preserved proof log. `git diff --check`
also passed.
