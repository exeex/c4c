# AArch64 Stack Frame SP Alignment Todo

Status: Active
Source Idea Path: ideas/open/288_aarch64_stack_frame_sp_alignment.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General AArch64 Frame Alignment

# Current Packet

## Just Finished

Completed Step 2, "Repair General AArch64 Frame Alignment".

Changed `src/backend/mir/aarch64/codegen/prologue.cpp` so fixed-frame AArch64
frame-boundary facts always use at least 16-byte stack-pointer alignment and
round the final emitted prologue/epilogue frame adjustment to that boundary.
Prepared local-slot and saved-register offsets are preserved; the alignment
padding is added only to the final frame size carried by the AArch64 frame
boundary node.

Added focused backend coverage in
`tests/backend/mir/backend_aarch64_return_lowering_test.cpp` for a leaf fixed
frame with a 16-byte prepared local area and one 8-byte saved register at
prepared offset 16. The test verifies the emitted frame fact rounds to 32
bytes, reports 16-byte alignment, does not preserve LR for the leaf case, and
keeps the saved-register slot offset at 16.

Regenerated `tests/c/external/c-testsuite/src/00004.c` AArch64 assembly now
emits `sub sp, sp, #32` and `add sp, sp, #32`; local `x` remains at `[sp]`,
local pointer `p` remains at `[sp, #8]`, and the saved register remains at
`[sp, #16]`.

## Suggested Next

The Step 3 nearby subset is already green. Supervisor should decide whether to
move directly to Step 4 wider bus-error sampling or close/split after review.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact frame-size shortcuts, local-variable
  spelling shortcuts, and emitted-instruction-text matching.
- Separate remaining bus-error cases that prove to be broader pointer, array,
  struct, function-pointer, or local-state owners.
- Keep timeout-sensitive cases `00132.c`, `00173.c`, and `00220.c` out of this
  route unless a separate timeout-specific idea is created.
- The AArch64 fix intentionally rounds only the frame boundary facts consumed
  by prologue/epilogue emission; it does not rewrite prepared stack-layout or
  frame-plan slot offsets.

## Proof

Ran the focused backend regression:

```sh
cmake --build build-aarch64-scan --target backend_aarch64_return_lowering_test
./build-aarch64-scan/tests/backend/mir/backend_aarch64_return_lowering_test
```

Result: passed.

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00004_c$'; } > test_after.log 2>&1
```

Result: passed, `1/1` selected test green.

Proof log: `test_after.log`.

Supervisor acceptance proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed, resolving `c_testsuite_aarch64_backend_src_00004_c`.

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed, `139/139`.

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_000(04|05|13|14|15|16)_c$'; } > /tmp/c4c_sp_alignment_nearby_subset.log 2>&1
```

Result: passed, `6/6`.

```sh
cmake --build --preset default
```

Result: passed.
