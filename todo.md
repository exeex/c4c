Status: Active
Source Idea Path: ideas/open/351_aarch64_nested_select_false_value_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Nested Select Value Boundary

# Current Packet

## Just Finished

Step 1: Localize Nested Select Value Boundary completed. The first bad
boundary is scratch register reuse inside AArch64 scalar select false-operand
materialization, specifically `lower_scalar_select_publication` in
`src/backend/mir/aarch64/codegen/alu.cpp`.

Representative prepared evidence from `/tmp/c4c_00182_print_led.prepared_bir.txt`:
`print_led` keeps the digit lookup as a nested select chain where
`%t36.outer0` selects `elt0` for `i == 0` and otherwise carries accumulated
false value `%t36.outer0.sel1`; `%t36.outer0.sel1` is stack-homed at offset
396, and final `%t36.outer0` is register-homed in `x21`.

Representative generated evidence from `/tmp/c4c_00182.s`: the `sel1` result
is computed and published with `csel w9, w10, w9, eq` followed by
`str w9, [sp, #396]`. The final select then evaluates `cmp x13, #0`, reloads
the true operand into the same scratch with `mov w9, w20`, copies that clobbered
scratch into the false/result register with `mov w21, w9`, and emits
`csel w21, w9, w21, eq`. Thus the final `csel` sees `d[0]` for both true and
false operands.

This does not look like idea 345's closed stack-home publication failure:
the prior select home at `[sp, #396]` is published, but the final false operand
is materialized from the already-clobbered emitted scratch instead of preserving
or reloading the accumulated false value. Condition-code handling is not the
first bad boundary because the intervening `mov` instructions do not clobber
NZCV.

## Suggested Next

Execute Step 2/3 as a focused implementation packet: add a backend scalar
select test where a nested select's final condition is false and the false
operand is the accumulated previous select, then repair
`lower_scalar_select_publication` so the false operand is preserved or
rematerialized before the true operand scratch can overwrite it.

## Watchouts

- Do not special-case `00182`, `print_led`, LED line renderers, the digit
  array, `d[0]`, one temporary, one register, or one emitted instruction
  sequence.
- The fix should be semantic to scalar select operand materialization. A likely
  shape is to detect when the false operand source aliases the true scratch or
  result register, then materialize in an order or from a home that preserves
  the accumulated false value across final true-value materialization.
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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00182_c)$' | tee test_after.log`

Result: build was up to date; backend guard tests passed 6/6, and
`c_testsuite_aarch64_backend_src_00182_c` still fails with the known runtime
mismatch where output renders seven `7` digits. This is sufficient for the
localization packet because the red external representative matches the
recorded first bad boundary. Proof log: `test_after.log`.
