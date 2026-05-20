Status: Active
Source Idea Path: ideas/open/345_aarch64_scalar_select_result_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Completed `plan.md` Step 2 scalar select publication work in
`src/backend/mir/aarch64/codegen/alu.cpp`.

`lower_scalar_select_publication` now carries the prepared
`result_stack_offset_bytes` through the `csel` publication boundary and appends
stack-home publication lines for prepared stack-homed scalar select results.
The shared scalar stack-publication helper now uses scalar-width stores for
`i1`/`i8`/`i16` homes (`strb`/`strh`) instead of always publishing narrow
integer homes with a 32-bit `str`.

Added focused dispatch coverage in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for an `i16`
scalar select whose result has a prepared stack home and is then consumed by a
prepared frame-slot store. The test asserts that the printed route emits
`csel`, publishes the select result home, then allows the store consumer to
reload that home and store to the destination.

The old `00143` unpublished-select-home symptom is fixed in generated AArch64:
the representative route now emits `csel`, then `strh w9, [sp, #402]`, then
the consumer `ldrh w9, [sp, #402]` before storing to `[sp, #78]`.

## Suggested Next

Investigate the remaining `c_testsuite_aarch64_backend_src_00143_c` runtime
nonzero after scalar select result homes are published. The current failure no
longer matches the old unpublished-select-home reload; the next packet should
localize the next bad value or control-flow fact in the generated Duff-device
route.

## Watchouts

- `test_after.log` is red only because
  `c_testsuite_aarch64_backend_src_00143_c` still exits with code 1; all
  `backend_*` tests in the delegated subset passed.
- The next `00143` investigation should not back out the scalar select
  publication repair or the typed narrow stack-home stores.
- `clang-format` was not available on `PATH` in this environment; formatting
  was kept manual.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_|c_testsuite_aarch64_backend_src_00143_c)' > test_after.log 2>&1`

Result: build completed with `ninja: no work to do`; CTest ran 142 tests,
141 passed and `c_testsuite_aarch64_backend_src_00143_c` failed with
`[RUNTIME_NONZERO] exit=1`. `test_after.log` is the canonical proof log.
