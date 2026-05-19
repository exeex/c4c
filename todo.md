Status: Active
Source Idea Path: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair sign-extension structured source publication

# Current Packet

## Just Finished

Step 3 repaired sign-extension structured source publication/materialization for
simple integer scalar casts whose source arrives as an immediate/rematerializable
operand. The scalar cast printer now materializes immediate sources into the
destination GPR, then prints the existing sign/zero/truncate cast form using that
structured register source. The changed machine-printer unit coverage now
asserts semantic source materialization for an immediate `sext i32 -> i64`
source (`mov w0, #1`; `sxtw x0, w0`) instead of treating the immediate as a
missing source-register fact.

The old `scalar cast node requires a structured register source operand`
diagnostic is gone for both Step 3 sign-extension cases:

- `00134`: passes the delegated focused CTest subset.
- `00135`: passes the delegated focused CTest subset.

Step 2 zero-extension improvements are preserved in the same proof subset:

- `00105`: passes.
- `00126`: passes.

The remaining focused failures are residual runtime modes outside this Step 3
source-materialization packet:

- `00035`: `RUNTIME_NONZERO exit=1`.
- `00151`: `RUNTIME_NONZERO exit=1`.
- `00208`: `RUNTIME_NONZERO exit=Segmentation fault`.

## Suggested Next

Select the next packet around the residual runtime cases (`00035`, `00151`,
`00208`) without folding them into the completed Step 3 sign-extension
source-materialization slice.

## Watchouts

- Keep the owner limited to scalar-cast machine-printer forms unless the
  supervisor explicitly moves to a runtime residual packet.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- `00035`, `00151`, and `00208` are no longer Step 2 printer-admission failures,
  and they still have runtime residuals that should not be conflated with the
  completed sign-extension Step 3 source-operand work.
- The Step 3 implementation intentionally materializes immediate cast sources in
  the scalar cast printer using the destination register as the temporary source
  register; unsupported non-immediate non-register sources still fail closed with
  the structured-source diagnostic.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j7 -R '^c_testsuite_aarch64_backend_src_(00035|00105|00126|00134|00135|00151|00208)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; CTest failed 3/7 on residual runtime modes only.
`00105`, `00126`, `00134`, and `00135` passed. `00035` and `00151` still fail
with `RUNTIME_NONZERO exit=1`; `00208` still fails with `RUNTIME_NONZERO
exit=Segmentation fault`. Proof log: `test_after.log`.

Additional changed-unit check:

```bash
ctest --test-dir build -j7 -R '^backend_aarch64_machine_printer$' --output-on-failure
```

Result: passed.
