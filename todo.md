Status: Active
Source Idea Path: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Classify Unary-Plus FP Lvalue Publication

# Current Packet

## Just Finished

Step 4 ran the external `00174` representative proof after the scalar FP binary
publication repair. The representative still fails, but the first bad fact has
advanced: rows 1-12 now match expected output, row 13
`printf("%f\n", +a)` prints `0.000000` instead of `12.340000`, and row 14
`-a` plus rows 15-16 still match.

## Suggested Next

Execute Step 5 by localizing why unary plus on an FP lvalue loses the value for
`00174` row 13 while unary minus and nearby FP rows still publish correctly.
Compare the `+a` and `-a` generated/lowered paths before editing, then repair
the shared scalar FP expression publication or classification owner generally.

## Watchouts

Do not special-case `00174`, row 13, unary plus spelling, one FP register, or
one emitted instruction sequence. Keep the residual framed as scalar FP value
publication/classification until evidence proves it belongs to a separate
lifecycle owner.

The runbook has been extended because the original Step 1-4 route is exhausted
but the advanced residual remains inside the scalar FP publication source idea.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00174_c$'; } > test_after.log 2>&1`.
The build completed, then the focused external representative failed with the
advanced residual described above. `test_after.log` is the proof log.
