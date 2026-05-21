Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Executed idea 295 Step 2 classification for the 7 remaining external AArch64
backend residuals from the current backend-regex surface. No implementation or
test files were edited; this packet used existing `test_after.log`, generated
AArch64 under `build/c_testsuite_aarch64_backend/src/`, c-testsuite sources,
and targeted prepared-BIR dumps where useful.

Classified residual buckets:

1. Local aggregate address publication for address-of local aggregate values
   passed to scalar pointer call consumers: `00218`, `00216`.
   - `00218`: source checks an unsigned enum bit-field stored in local
     `union tree_node convs`. Prepared BIR for `convert_like_real` has the
     expected load/mask/compare against `152`, and generated callee code reads
     `[x0,#16]` and masks with `255`. The first bad fact is in `main`: the call
     passes stale `x21` in `x0` instead of materializing `&convs` from the
     local aggregate home. The bit-field unsigned mask is adjacent, not the
     first owner.
   - `00216`: runtime segfaults before buffered stdout appears. In `foo`, the
     first local `print(ls)` call sets `x1` from stale `x13` instead of
     materializing the address of local aggregate `ls`, so `print_` dereferences
     an invalid pointer. Later generated code also has likely compound
     initializer/function-pointer selected-table residuals, but the first crash
     is local aggregate address publication.

2. Scalar integer constant/`sizeof` stack-home publication for loop bounds:
   `00205`.
   - Expected output iterates the global `cases` initializer; actual output is
     empty. Global `.data cases` is emitted with plausible initialized values,
     but `main` tests the outer loop bound by reloading uninitialized stack
     slots (`[sp,#40]` then `[sp,#32]`) instead of publishing the constant
     `sizeof(cases) / sizeof(cases[0]) == 9`. The first bad fact is the missing
     scalar constant loop-bound home before the branch, not global initializer
     data or selected field loads.

3. External call result publication into a prepared scalar home before compare:
   `00187`.
   - The first mismatch is `couldn't read fred.txt` after a successful file
     write. Generated code calls `fread`, then compares `ldr x9, [sp,#96]` to
     `6` without storing the `x0` return value into that prepared home. Later
     `fgets`/file-flow issues may remain, but the first bad fact is after-call
     scalar result publication for a branch compare.

4. Scalar FP constant and FP binary arithmetic materialization/publication:
   `00174`.
   - Floating arithmetic/constant prints are `0.000000`, while integer compare
     results and `sin(2)` are correct. Generated code for `float a = 12.34 +
     56.78` and direct `printf("%f", 12.34 +/-/*// 56.78)` paths moves or
     converts from stale `d13` instead of materializing FP constants and FP
     binary results. FP compare lowering is not the first owner because
     constants are materialized correctly for `fcmp`/`cset` paths.

5. Dynamic stack/VLA fixed-slot base publication and restoration: `00207`
   timeout.
   - `f1` moves `sp` for a VLA, but later reloads and stores the fixed `argc`
     slot through `[sp,#8]` after `sp` now points at the dynamic allocation.
     Prepared frame state says `has_dynamic_stack=yes`,
     `fixed_slots_use_fp=yes`, and `stable_base=fp`. The first timeout owner is
     fixed-slot addressing after dynamic stack movement, not the later
     short-circuit code in `f3`.

6. Shift/type-promotion timeout bucket: `00200` timeout.
   - The source expands many left-shift promotion checks. Generated code is a
     very large straight-line check sequence with many branch/call sites; no
     exact first bad fact was proven from artifacts alone. Park this as a
     timeout/type-promotion/shift-width localization bucket before repair.

## Suggested Next

Recommended Step 3 owner ranking:

1. Local aggregate address publication for address-of local aggregates passed
   to calls, with `00218` as the lead crisp representative and `00216` as a
   crash guard. This bucket covers two residuals and has a clear first bad
   boundary: publish the local aggregate address value into the call argument
   instead of using stale scratch/callee-saved carriers.
2. Scalar constant/`sizeof` stack-home publication for loop bounds, led by
   `00205`. This is a narrow, high-confidence single-representative repair
   candidate: materialize the constant loop bound before the branch consumer.
3. External call result publication before scalar compare, led by `00187`.
4. Scalar FP constant/binary arithmetic publication, led by `00174`.
5. Dynamic stack fixed-slot base repair, led by timeout `00207`.
6. Shift promotion/width timeout localization, led by `00200`.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. `00216` may advance to later initializer or
function-pointer selected-table facts after the local-address crash is fixed;
keep that as residual classification instead of broadening the first repair.
`00200` and `00207` remain timeout buckets and need focused timeout
localization before implementation.

## Proof

No new CTest command was required for Step 2. Used the existing Step 1
backend-regex proof in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build completed; CTest returned nonzero for the expected residual
surface. `test_after.log` reports 356 selected, 349 passed, 7 failed, with
timeouts in `00200` and `00207`. Failed list:
`00174`, `00187`, `00200` timeout, `00205`, `00207` timeout, `00216`, `00218`.

Additional read-only classification commands/artifacts used:
`build/c4cll --dump-prepared-bir --mir-focus-function main` for `00205`,
`build/c4cll --dump-prepared-bir --mir-focus-function f1` for `00207`, and
`build/c4cll --dump-prepared-bir --mir-focus-function convert_like_real` for
`00218`, plus generated assembly/source inspection for all 7 residuals.
