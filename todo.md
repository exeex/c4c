Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General AArch64 Byte/NUL Publication

# Current Packet

## Just Finished

Step 2 added focused AArch64 backend coverage for stack-homed narrow scalar
control reloads in `backend_aarch64_instruction_dispatch`: materialized branch
conditions with prepared i8 and i16 stack homes now require `ldrb` / `ldrh`
before `cbnz`, even when the prepared memory-access fact is wider than the
value home.

Step 3 repaired the `alu.cpp` scalar frame-slot helper path so prepared
frame-slot scalar loads select `ldrb` / `ldrh` / `ldr` from the prepared
operand width and force W-register spelling for byte/halfword/word loads.
`make_prepared_scalar_load_source` now prefers the prepared value-home
size/alignment over a wider frame-slot access width, and unpublished same-block
local loads preserve the BIR load result width before control publication.

Step 3 also repaired the dispatch-side residual. `dispatch.cpp`
`emit_prepared_value_home_to_register` now accepts the prepared stack layout
and selects stack-slot reload width from the authoritative prepared home/slot
width, capped by the supplied scalar type width. Sub-64-bit stack-home reloads
therefore publish through W-register spellings with `ldrb` / `ldrh` instead of
falling back to a stale full-width `ldr`.

The remaining format-byte control path was a fused-compare support instruction:
dispatch emitted a `BeforeInstruction` stack-source move for the compare input,
then skipped the compare as branch-fused. A new dispatch-owned
stack-home fused-compare branch path reloads the prepared lhs home with
`emit_prepared_value_home_to_register` before `cmp`, and the redundant
stack-source move is suppressed when that path owns the branch. The
`myprintf` loop now lowers the format-byte test as:

```asm
strb w9, [sp, #880]
ldrb w9, [sp, #880]
cmp w9, #0
```

## Watchouts

Do not route this packet to HFA, byval aggregate, or aggregate `va_arg`
owners. This packet stayed limited to general byte/halfword stack-home reload
publication for scalar control paths.

Avoid a testcase-shaped `.str50` or `myprintf` special case. The defect is a
general narrow stack-home reload in AArch64 control publication. The focused
backend coverage exercises i8/i16 stack-home control reloads and the narrower
prepared-home case where an i64 transport/control value is backed by an i8
stack home.

## Proof

Ran the delegated Step 2/3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded; `backend_.*` passed; `c_testsuite_aarch64_backend_src_00140_c`
passed; `c_testsuite_aarch64_backend_src_00204_c` passed. The selected test
set ran 145 tests with 0 failures. `test_after.log` is the preserved proof log.
