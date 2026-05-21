Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Executed idea 295 Step 2 by classifying the 8 external AArch64 backend
residuals from the current backend-regex surface. No implementation files,
tests, plan files, source ideas, or logs were edited.

Classified buckets:

- `00182`: dynamic static/global indexed array publication. Output renders all
  LED digits as zeroes. `print_led` computes/stores remainders into
  `__static_local_print_led_3`, then later uses a large selected-load chain for
  `d[i]` before calling `topline`/`midline`/`botline`. Likely owner is AArch64
  selected global/static aggregate snapshot publication before scalar call
  arguments; this is adjacent to idea 371 but for static/global selected loads
  feeding calls.
- `00205`: dynamic global aggregate selected load plus scalar loop-bound
  publication. Actual output is empty. Prepared BIR has constant loop bounds
  `%t1 = udiv i64 504, 56` and `%t9 = udiv i64 32, 8`, but generated
  `main` reloads uninitialized stack slots (`[sp,#40]`, `[sp,#32]`,
  `[sp,#64]`, `[sp,#56]`) for those bounds, so the outer loop exits before
  printing. The body also contains large selected chains for `cases[j].c[i]`.
  Likely same selected/global aggregate family, with an additional scalar
  constant-binary stack publication first bad fact.
- `00174`: scalar FP arithmetic/materialization. Expected floating arithmetic
  prints nonzero values; actual first five FP prints are `0.000000` while FP
  comparisons are correct. Generated `main` starts with `fcvt s8, d13` and
  later `fmov d0, d13` for direct FP arithmetic values without materializing
  the double constants/results. Owner is AArch64 scalar FP ALU/immediate
  materialization and FP call-argument publication, not variadic ABI layout.
- `00187`: external call-result stack publication. The first mismatch is the
  spurious `couldn't read fred.txt`; generated code calls `fread`, then
  compares `ldr x9, [sp,#96]` against `6` instead of using/publishing the
  `fread` return in `x0`. Later file loops continue correctly. Owner is
  fixed-arity call-result publication to a stack home before compare/control.
- `00200` timeout: macro-expanded constant-control/loop surface. Generated
  `main` is a very large ladder of `do { ... } while (0)` style blocks with
  many constant false backedges (`mov w9,#0; cmp w9,#0; b.ne ...`) and nested
  unconditional jumps. Park as control-flow/constant-loop lowering until a
  focused timeout localization proves the exact infinite/slow path.
- `00207` timeout: dynamic stack/VLA plus goto/short-circuit surface. `f1`
  allocates a VLA, stores dynamic-stack metadata through `x29` even though the
  frame is not clearly established, restores `sp` around the goto loop, and
  `f3` contains short-circuit joins with skipped RHS call results. Park as
  dynamic-stack/control-flow until localized; do not fold into idea 370 without
  proof, because this timeout also has VLA stack adjustment.
- `00216`: broad aggregate initializer/relocation/indirect-call surface.
  Runtime segfaults. Generated `test_multi_relocs` loads `table[0..2]` but
  calls `blr x21` unconditionally, selecting the middle function pointer
  instead of `table[i]`; `test_compound_with_relocs` also exercises local and
  global function-pointer compound literals. Park as selected global function
  pointer load feeding indirect call plus aggregate initializer/relocation
  stress.
- `00218`: unsigned enum bit-field/local aggregate address publication.
  `convert_like_real` itself masks the 8-bit field and compares against `152`
  correctly, but generated `main` passes `mov x0, x21` to
  `convert_like_real` while `%lv.convs`/the local union address in `x21` was
  never materialized. Park as local aggregate address publication for
  bit-field/union locals, adjacent to but not the same as idea 372's
  pointer-local assignment repair.

Owner ranking for the next focused plan:

1. Selected static/global aggregate or pointer-table value publication before
   consumers: covers the most surface (`00182`, body of `00205`, and likely the
   indirect call symptom in `00216`). Start with a smaller representative than
   `00216`, preferably `00182` or a focused static-array selected load feeding
   a call argument.
2. Scalar constant-binary stack publication in loop bounds: `00205` has a very
   crisp first bad fact and may be an easier high-confidence repair before the
   larger selected-load body.
3. External call-result stack publication before compare/control: `00187` is a
   narrow first bad fact (`fread` return).
4. Scalar FP arithmetic/immediate materialization: `00174`.
5. Parked broader buckets needing separate localization: `00200` timeout,
   `00207` VLA/control timeout, `00218` bit-field local-address publication,
   and the aggregate-initializer parts of `00216`.

## Suggested Next

Open or activate a focused idea for the ranked owner: AArch64 selected
static/global aggregate value publication before scalar consumers, with `00182`
as the lead representative and `00205`/`00216` used only as adjacency checks
after the first bad fact is repaired.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. `00163` is absent from the current
backend-regex failure list after idea 372 closure. The suggested next owner is
not a testcase-shaped `00182` fix; it should prove static/global selected
values are materialized for general consumers.

## Proof

No new test command was required for Step 2. Classification used existing
`test_after.log`, generated AArch64 under
`build/c_testsuite_aarch64_backend/src/`, C sources under
`tests/c/external/c-testsuite/src/`, and focused prepared-BIR dumps for
representative functions.

Reference Step 1 backend-regex proof:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result remains: build completed; CTest selected 356 backend-regex tests, 348
passed, 8 failed. Proof log remains `test_after.log`.
