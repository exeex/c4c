Status: Active
Source Idea Path: ideas/open/301_aarch64_memory_store_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize symbol/global store value operands

# Current Packet

## Just Finished

Step 3 materialized prepared frame-slot source operands for symbol/global
stores in `print_symbol_memory`. Symbol stores now keep the symbol address in
the first reserved scratch and load the frame-slot source through a distinct,
width-aware value scratch before emitting the selected store. Added a
machine-printer guard for a symbol store whose source value is a prepared
frame-slot memory operand.

Focused proof moved the Step 3 symbol/global store-value cases past
`symbol store value is not a register or immediate operand`: `00213` now
passes; `00176`, `00181`, and `00214` reach runtime failures; `00182` reaches
a different backend assembler failure on a non-encodable immediate move.
Step 2 source-scratch behavior remains covered by the existing 1-byte
stack-source printer guard and `00194` still passes.

## Suggested Next

Pick the next coherent packet from the remaining focused modes: runtime
incorrectness/segfaults in `00176`, `00181`, and `00214`, or the non-encodable
immediate move assembler failure in `00182`.

## Watchouts

- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 300 from failing counts alone.
- Keep runtime nonzero, runtime mismatch/crash, timeout, scalar operand-shape,
  call-boundary, unsigned reduction, invalid scalar cast spelling, and
  `lir_to_bir` residuals outside this owner unless generated-code or
  diagnostic evidence proves they share the memory-store operand failure mode.
- `00173` and `00187` are no longer blocked by stack-source printer
  materialization but now expose runtime modes. Keep those runtime modes outside
  the symbol/global packet unless new generated-code evidence ties them back to
  store source materialization.
- Do not special-case c-testsuite names or diagnostic strings. The Step 3
  semantic target for symbol/global store-value materialization is complete for
  prepared frame-slot operands; remaining failures need fresh ownership.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00173|00176|00181|00182|00187|00194|00213|00214)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; focused CTest subset failed 6/8. `00194` and `00213`
passed. `00173`, `00181`, and `00214` failed with runtime segmentation faults,
`00176` failed with a runtime mismatch, `00182` failed in backend assembly
with `mov x0, #1234567` rejected as an incompatible immediate, and `00187`
timed out. None of the Step 3 symbol/global cases failed with
`symbol store value is not a register or immediate operand`. `test_after.log`
is the proof log for this packet. Also ran
`./build/tests/backend/mir/backend_aarch64_machine_printer_test`, which passed.
