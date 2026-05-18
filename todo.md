# AArch64 Stack Frame SP Alignment Todo

Status: Active
Source Idea Path: ideas/open/288_aarch64_stack_frame_sp_alignment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Minimal SP Alignment Failure

# Current Packet

## Just Finished

Completed Step 1, "Establish Minimal SP Alignment Failure", for
`tests/c/external/c-testsuite/src/00004.c`.

Observed source shape:
- `main` declares `int x; int *p;`, stores `x = 4`, stores `p = &x`, writes
  through `*p`, and returns `*p`.

Generated AArch64 assembly at
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00004.c.s`:
- Prologue emits `sub sp, sp, #24`.
- First stack access after that is `str x20, [sp, #16]`.
- Local `x` lives at `[sp, #0]` with 4-byte stores/loads.
- Local pointer `p` lives at `[sp, #8]` with 8-byte stores/loads.
- `p = &x` materializes `add x9, sp, #0` and stores that pointer at
  `[sp, #8]`.
- Epilogue restores `x20` from `[sp, #16]` and emits `add sp, sp, #24`.

Prepared layout evidence from `/tmp/c4c_00004_aarch64_prepared_bir.txt`:
- `prepared-stack-layout`: local frame size is 16 bytes, frame alignment is
  8 bytes, slot #0 is `x` at offset 0 size 4 align 4, slot #1 is `p` at
  offset 8 size 8 align 8.
- `prepared-frame-plan`: base frame size is 16 bytes, saved callee register
  `x20` is placed at stack offset 16 size 8 align 8.
- The AArch64 boundary calculation extends the emitted frame to 24 bytes but
  does not round the final SP adjustment to 16 bytes for this leaf function.

Failure shape:
- The runtime exits with `Bus error`.
- The minimal misalignment is the 24-byte stack adjustment: assuming an
  initially 16-byte-aligned `sp`, `sub sp, sp, #24` leaves `sp % 16 == 8`.
- The next emitted SP-relative memory operation uses misaligned `sp` as the
  base register, which is invalid for AArch64 SP-relative accesses under the
  16-byte stack-pointer alignment rule.

## Suggested Next

Delegate Step 2 to repair the general AArch64 frame-size alignment path so the
final prologue/epilogue SP adjustment is rounded to a 16-byte multiple while
preserving prepared local offsets.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact frame-size shortcuts, local-variable
  spelling shortcuts, and emitted-instruction-text matching.
- Preserve local-slot load/store correctness when adding frame padding:
  `x` should remain at offset 0 and `p` at offset 8 for this case unless a
  broader semantic layout change intentionally moves all references together.
- Likely Step 2 implementation surfaces are
  `src/backend/mir/aarch64/codegen/prologue.cpp` frame-boundary fact
  calculation, `src/backend/mir/aarch64/codegen/machine_printer.cpp` frame
  emission consumers, and possibly `src/backend/prealloc/frame_plan.cpp` if
  the durable prepared frame plan should carry the rounded size instead of
  only the AArch64 boundary node.
- `src/backend/prealloc/stack_layout/slot_assignment.cpp` already rounds the
  local-object layout to its max local alignment, producing 16 bytes here;
  the missing rounding appears after saved callee-register slots are included
  in the AArch64 frame boundary size.
- Separate remaining bus-error cases that prove to be broader pointer, array,
  struct, function-pointer, or local-state owners.
- Keep timeout-sensitive cases `00132.c`, `00173.c`, and `00220.c` out of this
  route unless a separate timeout-specific idea is created.
- Keep compare/branch printer/lowering and other compile-stage backend gaps
  out of this route.

## Proof

Ran the delegated baseline proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00004_c$'; } > test_before.log 2>&1
```

Result: build was up to date, the selected CTest failed as expected with
`[RUNTIME_NONZERO] .../00004.c exit=Bus error`.

Proof log: `test_before.log`.
