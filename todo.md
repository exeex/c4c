Status: Active
Source Idea Path: ideas/open/332_aarch64_movi_zero_extension_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Owner

# Current Packet

## Just Finished

Completed plan Step 2, "Repair The Classified Owner". The BIR scalar immediate
cast fast path in `src/backend/bir/lir_to_bir/scalar.cpp` now routes
`LirCastKind::SExt`, `ZExt`, and `Trunc` immediates through the existing
`fold_integer_cast` helper instead of copying the source immediate into the
target width.

Focused coverage was added in `tests/backend/bir/CMakeLists.txt` as
`backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`. It focuses the
`00204.c` BIR dump to `movi()` and requires the repaired zero-extended calls:
`bir.call void pll(i64 2882338816)` for source `0xabcd0000` and
`bir.call void pll(i64 2863311530)` for source `0xaaaaaaaa`. It also forbids
the old sign-extended forms `-1412628480` and `-1431655766`.

The MOVI first bad fact has advanced: the representative AArch64 runtime output
now prints the MOVI high-bit values as zero-extended, including `abcd0000`,
`aaaaaaaa`, and `f8f8f8f8`. A follow-up residual remains after `movi()` in
`opi()`: the first OPI call at `tests/c/external/c-testsuite/src/00204.c:476`,
`pll(addip0(x))`, expects `3e8` but the appended representative run printed
`c220ecc6`.

## Suggested Next

Execute plan Step 4 classification for the advanced `00204.c` residual. Start
from `opi()` and localize why `addip0(x)` observes `c220ecc6` instead of
`3e8` after MOVI zero-extension is fixed.

## Watchouts

The delegated CTest regex selected the focused dump/guardrail subset but did
not select the representative because the registered test is named
`c_testsuite_aarch64_backend_src_00204_c`, not `backend_aarch64_backend_src_00204_c`.
That representative was run separately and appended to `test_after.log`; it
still fails, but after the repaired MOVI section.

Do not reopen the MOVI BIR immediate cast fold unless new evidence shows a
remaining MOVI mismatch. Leave HFA/byval/stdarg/fixed-formal/local-value
guardrails and `review/326_stdarg_byval_route_review.md` untouched.

## Proof

Ran the delegated proof command:

`cmake --build build --target c4cll backend_prepared_printer_test backend_aarch64_scalar_cast_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_(cli_dump_(bir|prepared_bir)_00204_stdarg|prepared_printer|aarch64_scalar_cast_records|aarch64_backend_src_00204_c)' > test_after.log 2>&1`

Result: passed, 6/6 selected tests green. The selected tests included the new
`backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold` focused guard.

Also ran `git diff --check`: passed.

Additional representative check appended to `test_after.log`:

`ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00204_c$' >> test_after.log 2>&1`

Result: failed after MOVI, with the first observed residual in `opi()` as
recorded above. Proof log path: `test_after.log`.

Supervisor broader validation passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed 141/141.
