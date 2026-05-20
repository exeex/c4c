Status: Active
Source Idea Path: ideas/open/351_aarch64_nested_select_false_value_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Nested Select Coverage and Repair Select Operand Preservation

# Current Packet

## Just Finished

Step 2/3: Add Focused Nested Select Coverage and Repair Select Operand
Preservation completed. Added instruction-dispatch coverage for a nested scalar
select whose final false operand is the previously accumulated stack-published
select result, and repaired `lower_scalar_select_publication` so the false
operand is materialized into the final result before the true scratch is filled.

The repair is semantic to scalar select publication: the final `csel` still
selects between the true scratch and result register, but the result register
now already holds the preserved false value. Direct frame-slot loads no longer
require an unused address scratch when the `[sp, #imm]` form is encodable,
which keeps the stricter occupied-register set usable for direct stack homes.

## Suggested Next

Execute Step 3/3 as the supervisor chooses: review the now-green representative
and decide whether the active idea can close after broader guard validation or
needs a small follow-up around select publication invariants.

## Watchouts

- Do not special-case `00182`, `print_led`, LED line renderers, the digit
  array, `d[0]`, one temporary, one register, or one emitted instruction
  sequence.
- The new focused test asserts order around final false preservation before
  true scratch materialization, not a named C testcase shape.
- Do not reopen frame-slot/frame-size layout consistency from idea 316 unless
  fresh evidence shows a current frame allocation or stack-slot divergence.
- Do not reopen unsigned div/rem producer publication, direct-call lowering,
  prepared call/address materialization, runner behavior, timeout policy,
  expectation changes, unsupported-classification changes, or CTest
  registration.
- Do not treat this as idea 345 unless fresh evidence shows an unpublished
  stack home; current evidence shows a published stack home but stale register
  materialization for the false operand.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00182_c)$' | tee test_after.log`

Result: build completed and all selected tests passed, including
`c_testsuite_aarch64_backend_src_00182_c`. Proof log: `test_after.log`.

Supervisor broader guard after the scalar select publication repair:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed 141/141.
