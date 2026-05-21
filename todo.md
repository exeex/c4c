Status: Active
Source Idea Path: ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Constant Loop-Bound Publication

# Current Packet

## Just Finished

Step 1 localization complete for `00205` scalar constant/`sizeof` loop-bound
publication. Source loop bounds are compile-time `sizeof` divisions:
`sizeof(cases) / sizeof(cases[0])` and
`sizeof(cases->c) / sizeof(cases->c[0])`.

Concrete first bad fact: semantic and prepared BIR keep the bounds as named
constant-binary values, `%t1 = bir.udiv i64 504, 56` and
`%t9 = bir.udiv i64 32, 8`, but prepared value homes publish them as stack
slots, not immediates or live registers:
`home %t1 kind=stack_slot slot_id=5 offset=32` and
`home %t9 kind=stack_slot slot_id=8 offset=56`. Their compare-side casted
indices `%t2` and `%t10` also receive stack slots at offsets 40 and 64.

The selected branch conditions are fused compares:
`for.cond.1 compare=ult i64 %t2, %t1` and
`for.cond.3 compare=ult i64 %t10, %t9`. Therefore the selected compare
currently expects stack homes for the named RHS loop-bound values, not direct
immediates. Generated assembly confirms the consumer reads those homes before
any visible publisher stores them:
`ldr x13, [sp, #40]`, `ldr x13, [sp, #32]`, then `cmp x9, x13` for the outer
loop, and `ldr x13, [sp, #64]`, `ldr x13, [sp, #56]`, then `cmp x9, x13` for
the inner loop.

Publication is lost or missing at the AArch64 fused-compare operand
publication boundary: the prepared data has enough expression information to
evaluate `%t1` and `%t9` as constants, but the selected compare path still
falls back to stack-home loads for those named constant-binary operands. The
smallest likely repair boundary is AArch64 fused compare/selected operand
lowering in `src/backend/mir/aarch64/codegen/dispatch.cpp`, with the adjacent
prealloc value-home classification gap that only rematerializes a subset of
computed immediates and leaves these `i64 udiv` constants stack-backed.

## Suggested Next

Execute Step 2: repair the fused-compare operand publication path so named
compile-time scalar loop bounds such as `%t1` and `%t9` are materialized as
compare immediates/registers or are stored to any selected stack home before
the first compare. Keep the fix semantic for constant-binary loop bounds, not
specific to `00205`, one offset, or one `sizeof` expression.

## Watchouts

Do not implement under idea 295. Keep this owner narrow to scalar constant or
`sizeof` loop-bound publication. The current focused proof still fails with
empty actual output/no rows printed, and the generated loop headers still show
the unpublished stack-home compare pattern, so remove that first before
reclassifying any later `00205` value mismatch.

Avoid solving this by hard-coding `00205`, `cases`, offsets 32/40/56/64, or
one compare neighborhood. Nearby aggregate/global value mismatches may remain
after this first bad fact is removed and should be reclassified separately.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00205_c$'; } > test_after.log 2>&1`

Result: failed. Build reported `ninja: no work to do`; the focused CTest ran
`c_testsuite_aarch64_backend_src_00205_c` and failed with
`[RUNTIME_MISMATCH]`. The canonical proof log is `test_after.log`.
