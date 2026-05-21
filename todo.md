Status: Active
Source Idea Path: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Classify Unary-Plus FP Lvalue Publication

# Current Packet

## Just Finished

Step 5 localized and repaired the unary-plus scalar FP publication residual at
the AArch64 call-boundary immediate argument path. In this checkout, `00174`
row 13 is `printf("%f\n", +12.34)`: semantic/prepared BIR already classify it
as an F64 variadic FPR argument, but call-boundary immediate binding only
materialized integer immediates into GPR ABI registers. The repair extends the
shared immediate call-argument binding and printer path so F32/F64 immediates
materialize into scalar FPR ABI registers generally.

## Suggested Next

Have the supervisor decide whether Step 5 completes the active scalar FP
publication idea or whether another residual needs a new packet.

## Watchouts

The delegated prompt referred to `+a`, but the live `00174` source in this
checkout uses `+12.34`. The failure boundary was therefore scalar FP immediate
publication into a variadic FPR call argument, not lvalue load classification.
The repair is general for F32/F64 immediate call arguments and does not
special-case `00174` or unary plus spelling.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00174_c$)'; } > test_after.log 2>&1`.
The build completed and all 146 selected tests passed, including
`backend_aarch64_return_lowering` and
`c_testsuite_aarch64_backend_src_00174_c`. `test_after.log` is the proof log.
