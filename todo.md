Status: Active
Source Idea Path: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair zero-extension supported-width publication

# Current Packet

## Just Finished

Step 2 repaired the zero-extension supported-width printer path for promoted
truth-value casts by allowing `ZExt` records whose promoted source/result
integer widths are equal to print as an identity GPR move. The changed
machine-printer unit coverage now asserts the semantic `zext i32 -> i32`
promoted truth-value form prints as `mov`, while the existing unsupported-facts
test still rejects missing source registers and unsupported types.

The old `scalar cast node requires a supported integer source/result width`
diagnostic is gone for all five Step 2 zero-extension cases:

- `00105`: passes the delegated focused CTest subset.
- `00126`: passes the delegated focused CTest subset.
- `00035`: now reaches runtime and fails with `RUNTIME_NONZERO exit=1`;
  inspection shows the remaining visible mode is later scalar arithmetic/compare
  behavior, not the zero-extension printer diagnostic.
- `00151`: now reaches runtime and fails with `RUNTIME_NONZERO exit=1`;
  inspection shows the visible mode is return-register publication after the
  scalar cast, not the old zero-extension width rejection.
- `00208`: now reaches runtime and fails with `RUNTIME_NONZERO exit=Segmentation
  fault`; the old zero-extension width rejection is no longer the blocking mode.

The two Step 3 sign-extension cases remain at their prior diagnostic:
`00134` and `00135` still fail with `opcode=sign_extend: scalar cast node
requires a structured register source operand`.

## Suggested Next

Execute `plan.md` Step 3 for `00134` and `00135`: repair sign-extension source
operand publication/materialization so rematerializable immediate sources become
printable structured register sources before the scalar cast printer.

## Watchouts

- Keep the owner limited to scalar-cast machine-printer forms.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- `00035`, `00151`, and `00208` are no longer Step 2 printer-admission failures,
  but they still have runtime residuals that should not be conflated with the
  sign-extension Step 3 source-operand work.
- The sign-extension cases (`00134`, `00135`) still point at scalar cast source
  materialization for immediate/rematerializable values. The earlier surface was
  `make_prepared_scalar_operand` in `src/backend/mir/aarch64/codegen/alu.cpp`
  feeding `print_scalar_cast_instruction`.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j7 -R '^c_testsuite_aarch64_backend_src_(00035|00105|00126|00134|00135|00151|00208)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; CTest failed 5/7. `00105` and `00126` passed. The
failed cases are `00035` runtime nonzero, `00151` runtime nonzero, `00208`
runtime segfault, and the expected Step 3 `00134`/`00135` sign-extension
structured-source diagnostics. Proof log: `test_after.log`.

Additional changed-unit check:

```bash
ctest --test-dir build -j7 -R '^backend_aarch64_machine_printer$' --output-on-failure
```

Result: passed.
