Status: Active
Source Idea Path: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Scalar GPR Operand Homes

# Current Packet

## Just Finished

Step 1 audited the prepared inline-asm operand home producer/carrier path and
selected the first implementation packet. No implementation files were changed.

Producer/carrier surface:

- LIR-to-BIR metadata is parsed in
  `src/backend/bir/lir_to_bir/calling.cpp::make_inline_asm_metadata`. It
  classifies `r`, `=r`, `+r`, and decimal tied operands and records unsupported
  constraints, but it does not assign prepared homes.
- Prepared carrier construction is in
  `src/backend/prealloc/inline_asm.cpp::populate_inline_asm_carriers`, which
  calls `build_inline_asm_carrier`, `make_prepared_inline_asm_operand`, and
  `validate_inline_asm_carrier`.
- Operand homes come from
  `prepared_inline_asm_home_for_named_value` over the prepared value-location
  table. Result homes come from the call result through the same table.
- The exact old facts are emitted by `validate_inline_asm_carrier`:
  `missing_operandN_home` for register or tied operands with no operand home,
  and `tied_input_output_home_mismatch` when the tied input and output homes
  both have concrete target identities but differ.
- `src/backend/prealloc/prepared_printer/inline_asm.cpp` is printer-only
  evidence: it prints complete carriers and `missing fact=` rows already
  produced by the carrier validator.

Classification:

- `missing_operand0_home` on the `pr38533`-class rows is a tied scalar GPR
  source-home gap. The tied input value has no prepared source home, while the
  carrier already has output/result-home authority for the `=r,0` contract.
  Treat it as tied reconciliation work, not RV64 object inference.
- `tied_input_output_home_mismatch` on the `pr45695` / `pr49279` classes is a
  tied scalar GPR reconciliation gap. Both sides have homes, but the producer
  did not publish one coherent shared target-register contract.
- Ordinary scalar register input/output home publication is already represented
  in existing focused carrier tests when prepared value homes are present.
- `unsupported_constraint0:=*m` / `unsupported_operand_constraint0` is
  unsupported memory-constraint work and remains out of scope.
- Clobber-only `~{memory}` carriers are complete printer/carrier facts but not
  scalar GPR input/output work.
- No missing fact in the active blocker set is printer-only; the printer only
  exposes facts emitted by `src/backend/prealloc/inline_asm.cpp`.

First implementation target:

- Packet: repair tied scalar GPR inline-asm operand home reconciliation in
  `src/backend/prealloc/inline_asm.cpp`, using prepared carrier result homes
  and prepared operand metadata as authority.
- Primary owned files for the implementation packet:
  `src/backend/prealloc/inline_asm.cpp`,
  `tests/backend/bir/backend_prepared_printer_test.cpp`, `todo.md`, and
  `test_after.log`.
- Exact old facts to eliminate for supported scalar GPR forms:
  `missing_operand0_home` where a tied `=r,0` operand can be represented by the
  output/result register authority, and `tied_input_output_home_mismatch` where
  scalar GPR tied input/output homes can be coherently reconciled.
- Keep `=*m` memory constraints, address operands, clobber-only carriers,
  aggregate, vector, F128, RV64 object-lowering inference, expectations,
  allowlists, unsupported markers, runtime comparison, and pass/fail
  accounting out of the packet.
- Later implementation proof command:
  `{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

## Suggested Next

Execute the first implementation packet: repair prepared tied scalar GPR
inline-asm carrier home reconciliation in `src/backend/prealloc/inline_asm.cpp`
and add focused prepared-carrier tests in
`tests/backend/bir/backend_prepared_printer_test.cpp`.

## Watchouts

- Do not treat this as a simple printer fix; the old facts are produced by the
  carrier validator before printing.
- Do not implement RV64 object-lowering inference for missing inline-asm homes.
- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Keep `=*m` memory constraints and clobber-only `~{memory}` carriers outside
  this scalar GPR producer/carrier plan unless the audit proves a shared
  invariant requires them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 1 todo-only audit. Required local check:

```sh
git diff --check
```
