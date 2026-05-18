# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Scalar Publication Primitive

# Current Packet

## Just Finished

Step 2 repaired the prepared scalar frame-slot source to AArch64
call-argument ABI-register primitive.

`src/backend/mir/aarch64/codegen/calls.cpp` now lowers a prepared GPR
call-argument move whose source value home is `stack_slot` and whose call plan
source encoding is `frame_slot` by building a prepared frame-slot memory source
from the value's authoritative load access. The lowering deliberately resolves
the current stack-layout slot from prepared addressing instead of blindly using
the logical call-plan `source_stack_offset_bytes`.

`src/backend/mir/aarch64/codegen/instruction.hpp` now lets
`CallBoundaryMoveInstructionRecord` carry that source memory operand, and the
call-boundary printer emits a load into the destination ABI register.

For `src/00056.c`, the third `printf` call now materializes the second scalar
variadic value before the call:

```asm
mov x0, x21
mov x1, x13
ldr w2, [sp, #12]
bl printf
```

This uses the resolved `%t8` load access for local `d` at frame slot `#3`
offset `12`; it does not load the call-plan logical `source_stack_offset=24`,
which aliases the saved `x21` slot in the current frame layout.

The focused subset now has `00012.c`, `00056.c`, and `00211.c` passing.
`00009.c`, `00156.c`, and `00161.c` remain failing in other scalar
publication/control-value subfamilies.

## Suggested Next

Continue Step 2 by locating the next smallest shared owner among the remaining
representatives `src/00009.c`, `src/00156.c`, and `src/00161.c`. Keep the
scope on scalar expression/control-value publication; do not widen into
pointer/aggregate address or timeout owners.

## Watchouts

- Do not solve this by filename checks, expectation changes, allowlist changes,
  unsupported downgrades, or CTest edits.
- Keep pointer/aggregate address failures, timeout/hang cases, and
  compile-stage printer gaps out of this owner.
- `src/00056.c` confirms the third `printf` format pointer is correct; only the
  scalar second variadic value is missing. Treat it as scalar ABI argument
  materialization, not the closed string/global address owner.
- `src/00211.c` now passes after the immediate GPR call-argument placement fix;
  do not spend the next packet on that representative unless it regresses.
- `src/00056.c` now passes because the call-boundary load uses the prepared
  memory access for `%t8` (`frame_slot=#3`, current offset `12`) rather than
  the logical storage-plan spill slot `#6+stack24`.
- Prepared storage plans can name logical spill slots that are not current
  printable frame slots. For future frame-slot publication work, prefer
  prepared addressing plus stack-layout resolution when the value is produced by
  a load access.
- `clang-format` is not installed in this environment.

## Proof

Ran exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall with 3/6 passing. `00012`, `00056`, and `00211` passed;
`00009` failed with runtime nonzero, while `00156` and `00161` failed with
runtime mismatch. Proof log: `test_after.log`.

Supporting local assembly check for `00056`:
`/tmp/c4c_aarch64_scalar_step2_00056.after.s`.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
