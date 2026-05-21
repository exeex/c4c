Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 close-blocker regression repair is implemented and proofed. The
regressions introduced after `047790dac` had two independent causes:

- `00130`: the new stack-home fused-compare branch path was allowed to consume
  a stack home for a same-block derived value. In `00130`, `%t17 = sext i8
  %t16 to i32` had a stack-slot home, but that slot depended on a same-block
  publication that had not run; the branch shortcut loaded `[sp, #32]` and
  compared stale/uninitialized data instead of the loaded byte. The repair
  restricts stack-home fused compare branching to values whose same-block
  producer is an actual memory load, preserving the intended byte-home path
  used by `00204` while forcing cast-derived compare operands through normal
  value publication.
- `00123`: the close guard exposed a pre-existing gap in register publication
  for floating scalar compare results. `double x = 100.0; return x < 1;`
  lowered the load but never materialized the `sitofp` operand or the
  floating compare result, so the return moved raw FP bits through the GPR
  return register. The repair teaches the existing value-publication helper to
  materialize FP-producing casts (`sitofp`/`uitofp`, `fpext`/`fptrunc`) into FP
  scratch registers and to publish `f32`/`f64` compare results with
  `fcmp`/`cset` into the prepared GPR result home.

The `00204` format-byte repair is preserved. Generated
`build/c_testsuite_aarch64_backend/src/00204.c.s` still contains the intended
byte-width loop terminator publication:
`ldrb w9, [x13]`; `strb w9, [sp, #880]`; `ldrb w9, [sp, #880]`;
`cmp w9, #0`.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`:

- `fp_scalar_compare_result_publication_materializes_fcmp_cset` covers the
  `00123`-style FP compare result path and asserts the route emits FP operand
  materialization plus `fcmp`/`cset` before any GPR return move.
- `stack_home_fused_compare_rejects_same_block_i8_extension_home` covers the
  `00130`-style same-block i8 load plus sign extension branch path and asserts
  the branch route uses `ldrb`/`sxtb`/`cmp`, not the stale stack-home shortcut
  load from the derived value's stack slot.

## Watchouts

This packet did not touch expectations, unsupported classifications, runners,
HFA register-save-area code, byval aggregate lane code, `plan.md`, or
`ideas/open/*`. The `00123` repair is limited to general FP compare value
publication and the `00130` repair is limited to rejecting stack-home fused
branching for same-block derived homes.

## Proof

Ran the delegated close-blocker proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00123_c|c_testsuite_aarch64_backend_src_00130_c|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded; `backend_.*`,
`c_testsuite_aarch64_backend_src_00123_c`,
`c_testsuite_aarch64_backend_src_00130_c`,
`c_testsuite_aarch64_backend_src_00140_c`, and
`c_testsuite_aarch64_backend_src_00204_c` all passed after adding focused
backend coverage. The selected test set ran
147 tests with 0 failures (`100% tests passed, 0 tests failed out of 147`).
`test_after.log` is the preserved proof log for this packet.
