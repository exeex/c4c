Status: Active
Source Idea Path: ideas/open/345_aarch64_scalar_select_result_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Completed `plan.md` Step 3 residual scalar select publication repair for the
`%t9.store*` stack-homed initialization family.

`lower_scalar_select_publication` now materializes same-block scalar cast
producers when they feed select compare operands or select arm values. This
covers the `%t8 = sext i32 ... to i64` compare input and `%t10 = trunc i32 ...
to i16` true arm used by the first `00143` initialization loop, while still
falling back to prepared homes when direct producer rematerialization is not
available.

Added focused dispatch coverage for a stack-homed `i16` select whose compare
input is an unpublished same-block `sext` cast. The test asserts that the route
prints the cast materialization, emits `csel`, publishes the select stack home,
then lets the store consumer reload that home.

Generated AArch64 for the first `%t9.store0` path now emits `cmp x13, #0`,
`csel w9, w10, w9, eq`, `strh w9, [sp, #240]`, then reloads
`ldrh w9, [sp, #240]` before storing to `[sp]`. The later `%t13.store*`
publication pattern remains intact. The delegated proof now passes, including
`c_testsuite_aarch64_backend_src_00143_c`.

## Suggested Next

Supervisor should review whether Step 3 is complete and decide whether to close
or broaden validation for `ideas/open/345_aarch64_scalar_select_result_publication.md`.

## Watchouts

- The dispatch test allows the intentionally unlowered standalone compare-cast
  producer diagnostic in the focused synthetic case; the selected route still
  proves select publication before the store-local consumer reload.
- This packet did not edit expectations, unsupported classifications, runner
  behavior, timeout policy, proof-log policy, CTest registration, `plan.md`, or
  the source idea.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_|c_testsuite_aarch64_backend_src_00143_c)' > test_after.log 2>&1`

Result: build completed, CTest ran 142 selected tests, and all 142 passed.
`test_after.log` is the canonical proof log.
