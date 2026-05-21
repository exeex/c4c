Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2: Classify Residual Buckets. Classified the six current external
AArch64 backend residuals from existing `test_after.log`, generated assembly
under `build/c_testsuite_aarch64_backend/src/`, and source cases:

- `00174`: runtime mismatch, scalar FP expression/constant publication.
  Source prints float/double arithmetic and assignments. Generated
  `00174.c.s` starts by converting and printing uninitialized `d13`:
  `fcvt s8, d13`, later `fmov d0, d13`, which matches actual leading
  `0.000000` lines while integer FP comparisons later use explicit
  materialized constants.
- `00187`: runtime mismatch, external/libc call-result publication. Source
  checks `fread(...) != 6`. Generated `00187.c.s` calls `fread`, but compares
  `ldr x9, [sp, #96]` against `#6` instead of publishing the `x0` return,
  producing the spurious `couldn't read fred.txt` line while later file reads
  still produce the expected content.
- `00200`: timeout quarantine, shift/type-promotion macro expansion. Source
  is the left-shift type-promotion stress test. Generated `00200.c.s` is a
  very large branch/check expansion with repeated synthetic branch ladders and
  should remain quarantined unless a timeout-specific owner is selected.
- `00205`: runtime mismatch, scalar constant/`sizeof` stack-home publication.
  Source loop bounds are compile-time constants:
  `sizeof(cases)/sizeof(cases[0])` and
  `sizeof(cases->c)/sizeof(cases->c[0])`. Generated `00205.c.s` has correct
  `cases:` global `.xword` data, but the first loop compares `j` against
  stack values loaded from `[sp, #40]` then `[sp, #32]`, and the inner loop
  compares `i` against `[sp, #64]` then `[sp, #56]`; those homes are not
  initialized before the comparisons. The runtime prints no rows, so the first
  bad fact is before selected global element publication.
- `00207`: timeout quarantine, dynamic stack/VLA plus goto/fixed-slot control.
  Source stresses a VLA in `f1`, label/goto flow, and short-circuit
  conditionals. Generated `00207.c.s` mutates `sp` dynamically in `f1` and
  uses fixed stack slots around the dynamic adjustment; keep this as a
  timeout-specific bucket unless Step 3 deliberately splits dynamic-stack
  timeout work.
- `00216`: runtime nonzero/crash, broad aggregate initializer/compound
  relocation/function-pointer-table residual. The closed local aggregate
  address owner has advanced: generated `main` now materializes global and
  local aggregate addresses for the early `print(...)` calls and `foo(&gw,
  &phdr)`. The remaining source stress includes nested aggregate
  initializers, compound literals, local/global function-pointer arrays, and
  range-initialized function pointer tables; keep this below the crisp scalar
  publication owners until the crash point is localized further.

Candidate semantic owner ranking for Step 3:

1. Split a focused scalar constant/`sizeof` stack-home publication owner for
   `00205`. First bad fact evidence is strongest and non-timeout: generated
   code uses uninitialized stack homes for compile-time loop bounds even
   though the `cases` global data is emitted correctly.
2. Split external/libc call-result publication for `00187`. This is a crisp
   result-home issue (`fread` return in `x0` not reaching the compare), but it
   is a single residual and should not reopen closed call-symbol-home work
   without fresh scope wording.
3. Split scalar FP expression/constant materialization for `00174`. Multiple
   first FP outputs collapse to zero from stale `d13`; comparisons show some
   FP constants can materialize, so the owner should be narrowed before repair.
4. Park `00216` as aggregate initializer/compound relocation/function-pointer
   residual until a crash-point localization packet selects a concrete owner.
5. Quarantine `00200` as shift/type-promotion timeout.
6. Quarantine `00207` as dynamic stack/VLA fixed-slot timeout.

## Suggested Next

Execute Step 3: select or create the focused owner for scalar
constant/`sizeof` stack-home publication using `00205` as the lead
representative, unless the supervisor prefers to split the `00187` external
call-result owner first.

## Watchouts

This umbrella remains classification-only; do not implement fixes under idea
295. Do not reopen closed owners from counts alone. The `00205` evidence is
not selected/global aggregate publication yet: its global data exists, and the
first visible failure is uninitialized `sizeof` loop-bound homes before any
row is printed. Keep `00200` and `00207` quarantined as timeout cases unless a
timeout-specific Step 3 split is explicitly chosen.

## Proof

No new test run per packet. Used existing `test_after.log` from:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The log shows `ninja: no work to do.`, 357 selected tests, 351 passing tests,
and 6 failures, all under the `aarch64_backend c_testsuite` labels.
