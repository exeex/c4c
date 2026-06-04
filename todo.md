Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Characterize 00204

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: reproduced and characterized the current
AArch64 backend `00204.c` runtime mismatch without implementation changes.

Exact reproduction/proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Observed `00204.c` signature in `test_after.log`: build succeeds, `00032.c`
passes, `00182.c` passes, and `00204.c` fails with `[RUNTIME_MISMATCH]`.
The non-stdarg `MOVI:` block matches expected output through the end of the
log, which keeps the failure focused on argument/return/variadic/HFA ABI
traffic rather than generic runtime output.

Failure split from the current artifacts:

- Fixed argument and return aggregate/string cases are already corrupt before
  the explicit `stdarg:` block. The `Arguments:` output starts with repeated
  low control-byte payloads instead of `0`, `12`, `345`, ...; the `Return
  values:` string aggregates print repeated `!` payloads. The scalar/HFA
  direct argument and return values print mostly `0.0`, so ordinary aggregate
  argument/return movement is also suspect, not only `va_arg`.
- String/integer stdarg corruption is visible in the two string `myprintf`
  calls: expected `ABCDE...`/`lmnopqr` strings are replaced by short control
  bytes and garbage pointer-looking bytes. The prepared `myprintf` artifact
  shows `variadic_entry=yes` and GPR `va_arg` paths for `%7s` and `%9s` using
  `%lv.ap.24` offsets and `%lv.ap.8`/`%lv.ap.0` register-vs-stack source
  selection.
- HFA payload corruption is separate and affects all three payload families.
  `HFA long double:` prints all `0.0,0.0`; `HFA double:` prints mostly
  `0.0,0.0` with some `-nan`; `HFA float:` prints mostly `0.0,0.0` with a few
  stray values such as `-337.3` and one very large negative float. The prepared
  `myprintf` artifact lowers HFA varargs through `llvm.va_arg.aggregate(...)`
  blocks after the string cases, so Step 2 should trace both the variadic entry
  save-area setup and aggregate/HFA `va_arg` expansion before choosing a
  repair layer.

Relevant current artifacts observed:

- `build/c_testsuite_aarch64_backend/src/00204.c.s`
- `build/c_testsuite_aarch64_backend/src/00204.c.bin`
- `build/tmp_00204_prepared_myprintf.txt`
- `build/tmp_00204_prepared_myprintf_join.txt`

## Suggested Next

Proceed to Step 2: trace the first wrong ABI fact across BIR call ABI,
prealloc call plans, variadic entry plans, call moves, and AArch64 lowering.
Start with the direct aggregate argument/return corruption, then compare it
against `myprintf` variadic GPR save-area behavior and HFA
`llvm.va_arg.aggregate` payload behavior.

## Watchouts

- Do not downgrade `00204.c` expectations or mark it unsupported.
- Do not special-case `00204.c` or its literal output shape.
- Keep `00032.c` and `00182.c` visible as AArch64 guard cases.
- Treat narrow probes as ABI-fact probes, not testcase-shaped shortcuts.
- The mismatch is broader than the final `stdarg:` section: direct aggregate
  argument and return paths are visibly wrong too, while `MOVI:` remains green.
- The current evidence separates string/integer varargs from HFA varargs, but
  it does not yet identify the first wrong layer; do not jump directly to a
  named `00204.c` output fix.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8, expected for this characterization packet. Guard status:
`c_testsuite_aarch64_backend_src_00032_c` passed and
`c_testsuite_aarch64_backend_src_00182_c` passed;
`c_testsuite_aarch64_backend_src_00204_c` failed with runtime mismatch.
Canonical executor proof log: `test_after.log`.
