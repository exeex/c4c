Status: Active
Source Idea Path: ideas/open/301_aarch64_memory_store_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize store source scratch operands

# Current Packet

## Just Finished

Step 2 materialized stack-slot store source operands for local/frame stores in
`machine_printer.cpp`. The printer now uses a width-aware source scratch and
source load mnemonic, so 1-byte frame-slot store values print as `ldrb w9,
[sp, #source]` followed by the selected `strb` to the destination. Added a
machine-printer unit guard for this 1-byte stack-source materialization.

Focused proof moved the source-scratch cases past the printer diagnostic:
`00194` now passes, `00173` reaches runtime segfault, and `00187` reaches
runtime timeout. The residual focused printer failures are the Step 3
symbol/global store-value cases: `00176`, `00181`, `00182`, `00213`, and
`00214` still fail with `symbol store value is not a register or immediate
operand`.

## Suggested Next

Implement Step 3: materialize stack-slot source values for symbol/global stores
in `print_symbol_memory`, reusing the same semantic scratch model without
weakening printer checks.

## Watchouts

- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 300 from failing counts alone.
- Keep runtime nonzero, runtime mismatch/crash, timeout, scalar operand-shape,
  call-boundary, unsigned reduction, invalid scalar cast spelling, and
  `lir_to_bir` residuals outside this owner unless generated-code or
  diagnostic evidence proves they share the memory-store operand failure mode.
- Step 2 deliberately did not change symbol/global store printing; those cases
  are still Step 3 residuals.
- `00173` and `00187` are no longer blocked by stack-source printer
  materialization but now expose runtime modes. Keep those runtime modes outside
  the symbol/global packet unless new generated-code evidence ties them back to
  store source materialization.
- Do not special-case c-testsuite names or diagnostic strings. The semantic
  target for the next packet is symbol/global store-value materialization for
  prepared frame-slot operands.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00173|00176|00181|00182|00187|00194|00213|00214)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; focused CTest subset failed 7/8. `00194` passed.
`00173` failed at runtime with a segmentation fault, and `00187` timed out.
`00176`, `00181`, `00182`, `00213`, and `00214` remain Step 3 symbol/global
printer failures with `symbol store value is not a register or immediate
operand`. `test_after.log` is the proof log for this packet. Also ran
`./build/tests/backend/mir/backend_aarch64_machine_printer_test`, which passed.
