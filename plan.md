# tiny-c2ll Execution Plan

Last updated: 2026-03-08

## Objective

Raise full-suite stability by fixing real root causes in parser/typing/IR lowering, with strict regression control.

## Current Baseline

- Full run command of record: `ctest -j`
- Latest user-reported status: `1661 passed, 74 failed out of 1735` (`~96% pass`)
- Previous checkpoint: `1613 passed, 122 failed out of 1735` (`~93% pass`)
- Net improvement from previous checkpoint: `+48 passed`, `-48 failed`
- Recent successful high-value fix pattern:
  - Reproduce fail
  - Compare against clang `.ll`
  - Patch smallest ABI/IR mismatch
  - Re-verify targeted + full suite

## Remaining Failures (74) - Classified

### By label (latest run)

- `llvm_gcc_c_torture`: 74 / 1458 failed
- `c_testsuite`: 0 / 220 failed
- `ccc_review`: 0 / 8 failed
- `negative_tests`: 0 / 43 failed

### By feature cluster (from current fail list)

- `misc_runtime/date_pr`: 48 (largest bucket; many `pr*` / `YYYYMMDD_*`)
- `varargs_printf`: 7 (`fprintf_chk_1`, `printf_chk_1`, `user_printf`, `vfprintf_1`, `vfprintf_chk_1`, `vprintf_1`, `vprintf_chk_1`)
- `bitfield_layout_sign`: 4 (`bf64_1`, `bf_layout_1`, `bitfld_3`, `bitfld_5`)
- `control_flow_addr`: 3 (`comp_goto_1`, `const_addr_expr_1`, `return_addr`)
- `simd_vector`: 3 (`scal_to_vec2`, `simd_1`, `simd_2`)
- `string_lib`: 2 (`strlen_3`, `strlen_4`)
- `alignment_abi`: 2 (`align_3`, `stkalign`)
- `widechar`: 2 (`wchar_t_1`, `widechar_3`)
- `ieee_fp`: 2 (`ieee_copysign2`, `ieee_rbug`)
- `other_named`: 1 (`conversion`)

## Priority Order

1. P0: `negative_tests` compile-fail contracts (must not regress)
2. P1: `c_testsuite` runtime/ABI correctness
3. P2: `llvm_gcc_c_torture` conformance expansion

## Mandatory Loop (each repair slice)

1. Select one failing test and classify: parser / semantic / ABI / runtime.
2. Reproduce with exact `ctest -R` command.
3. For ABI/runtime/codegen failures, generate both IRs:
   - ours: `tiny-c2ll-stage1 case.c -o /tmp/ours.ll`
   - clang: `clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 case.c -o /tmp/clang.ll`
4. Identify one concrete mismatch and patch only that slice.
5. Run:
   - the failing test
   - sibling tests in same bucket
   - `ctest -j` (acceptance gate)
6. Log result in this file (short entry with date, case, root cause, status).

## Hard Constraints

1. No edits to vendored upstream tests under:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
2. No "fix" that only masks failures (blanket skips/allowlists) without documented rollback plan.
3. No merge if new regressions appear in previously passing tests.

## Evidence Template (append per slice)

Use this exact format:

```md
### YYYY-MM-DD: <test_name>
- Symptom:
- Root cause:
- Clang IR delta used: yes/no
- Files changed:
- Targeted re-run:
- Full `ctest -j` delta:
- Residual risk:
```

## Next Slice Queue

1. `P0-bitfield-pack-readwrite`: stabilize bitfield cluster first.
   - Target set: `bf64_1`, `bf_layout_1`, `bitfld_3`, `bitfld_5`.
   - Focus: storage-unit width, bit offset packing, signed/unsigned extension parity with clang.
2. `P1-varargs-abi`: close `varargs_printf` cluster (7 cases) in one ABI-focused slice.
   - Validate default promotions, va_list layout, indirect variadic calls, chk remap path.
3. `P2-control-flow-addressing`: close `comp_goto_1`, `const_addr_expr_1`, `return_addr`.
   - Verify `blockaddress`, label map materialization, and pointer constant lowering.
4. `P3-widechar-locale`: close `wchar_t_1`, `widechar_3`.
   - Verify wchar width assumptions and literal/load lowering.
5. `P4-simd-and-alignment`: close `simd_*`, `scal_to_vec2`, `align_3`, `stkalign`.
   - Verify vector scalar splat, vector load/store ABI, stack/object alignment guarantees.
6. `P5-long-tail`: process `misc_runtime/date_pr` by root-cause fan-out (not by test ID order).
   - Always pick one representative failing case per inferred root cause, then sweep siblings.

## Recent Evidence

### 2026-03-08: c_testsuite_tests_single_exec_00218_c
- Symptom: runtime output printed `unsigned enum bit-fields broken`.
- Root cause: enum bitfield read path used promoted result type for signedness decision, skipping enum nonnegative rule.
- Clang IR delta used: no (direct targeted root-cause fix from IRBuilder path inspection + repro).
- Files changed: `src/frontend_c/ir_builder.cpp`.
- Targeted re-run: `ctest --output-on-failure -R c_testsuite_tests_single_exec_00218_c` -> pass.
- Full `ctest -j` delta: reflected in new baseline (`74` fails total, `c_testsuite` now 0 fail).
- Residual risk: bitfield packing/sign extension interactions still present in `bf_*`/`bitfld_*` torture cases.

## Exit Criteria

1. Full-suite pass rate increases without new P0 regressions.
2. High-risk ABI path has clang-aligned behavior on representative cases.
3. Handoff notes include exact commands and proof points, not just conclusions.
4. `Segmentation fault/Bus error` class reaches zero before long-tail conformance polish.
