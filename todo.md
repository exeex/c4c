Status: Active
Source Idea Path: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize First Scalar FP Bad Fact

# Current Packet

## Just Finished

Step 1 localized the first scalar FP bad fact in `00174`. The earliest
incorrect runtime row is source row 1:
`float a = 12.34 + 56.78; printf("%f\n", a);`, expected `69.120003`, actual
`0.000000`.

Semantic and prepared BIR still contain the intended producer chain:
`%t0 = bir.add double 0x4028AE147AE147AE, 0x404C63D70A3D70A4`, then
`%t1 = bir.fptrunc double %t0 to float`, store/load `%lv.a`, `%t4 =
bir.fpext float %t3 to double`, and `printf(ptr @.str0, double %t4)`.
Prepared locations assign `%t0` to FPR `d13` and the first call argument path
to an FPR call argument.

Generated AArch64 consumes the value at `build/c_testsuite_aarch64_backend/src/00174.c.s:10`-`20`
by converting stale `d13` through `s8`/stack/`d8` and passing `d8` as `d0` to
`printf`, but no preceding instruction materializes the `%t0` double add result
or either FP literal into `d13`. The direct expression rows 2-5 and assignment
operator rows 9-12 fail the same way; prefix literal rows 13-14 also consume
unmaterialized scalar FP values. The adjacent comparison rows 6-8 still print
the expected integer results, and the final `a = 2`/`sin(2)` rows 15-16 still
work, keeping the boundary on scalar FP literal/arithmetic publication rather
than comparisons, integer-to-FP conversion, fixed-arity `sin`, or final
variadic handoff.

## Suggested Next

Execute Step 2 by adding focused backend coverage for AArch64 scalar FP
literal/arithmetic result publication into the FPR consumed by a later
conversion, store/load, or variadic call argument. The smallest useful case
should prove that a non-call scalar FP producer such as `bir.add double
<literal>, <literal>` is materialized before its assigned FPR is consumed,
without naming `00174`.

## Watchouts

Do not special-case `00174`, one output row, one FP register, or one emitted
instruction sequence. Keep `00216`, `00200`, and `00207` parked unless fresh
evidence proves they share this scalar FP owner.

Suspected Step 2/3 backend boundary: AArch64 scalar FP expression/literal
materialization for non-call producers assigned to FPR storage. The call
planner sees the FPR argument and comparisons can still fold/publish integer
results, but the machine emission for scalar FP `BinaryInst`/literal producers
does not publish the value before consumers read `d13`/`d8`.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00174_c$'; } > test_after.log 2>&1`.
The build was up to date and the selected test failed with the expected
`[RUNTIME_MISMATCH]`; `test_after.log` is the proof log.
