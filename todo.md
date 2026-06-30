Status: Active
Source Idea Path: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Reconcile Tied Scalar GPR Operand Homes

# Current Packet

## Just Finished

Step 2 verified the scalar GPR register input/output operand-home publication
packet. The supported untied scalar GPR `r` and `=r` carrier path is already
implemented and covered by focused prepared-carrier tests, so no implementation
files were changed.

Completed work:

- `src/backend/prealloc/inline_asm.cpp::make_prepared_inline_asm_operand`
  already copies each register input operand value from `call.args[arg_index]`,
  resolves its prepared value id, and publishes `operand.home` from the
  prepared value-location table.
- `src/backend/prealloc/inline_asm.cpp::build_inline_asm_carrier` already
  publishes `result_home` for register outputs from the call result through the
  same prepared value-location authority, then attaches target register
  identities to operand/result homes before validation.
- `validate_inline_asm_carrier` already accepts scalar GPR register inputs and
  outputs only when the corresponding homes are present, register-backed, and
  compatible with the register constraint; missing or incompatible homes remain
  fail-closed.
- Focused coverage in
  `tests/backend/bir/backend_prepared_printer_test.cpp` proves complete RV64
  scalar `r` input carrier homes, complete RV64 scalar `=r` output result
  homes, complete `+r` read/write scalar carrier homes, and fail-closed
  unsupported scalar/vector constraint cases.
- Existing focused coverage also preserves fail-closed behavior for missing
  register homes, unsupported memory/address authority or selection, named
  operands, template modifiers, unsupported clobber operands, vector/F128
  carrier boundaries, and other out-of-scope forms.
- The old representative blocker facts are not solved by this packet because
  they are tied-operand reconciliation forms: `missing_operand0_home` for tied
  `=r,0` source-home authority and `tied_input_output_home_mismatch` for
  incoherent tied input/output homes. Those remain the Step 3 owner.

## Suggested Next

Execute Step 3: reconcile tied scalar GPR inline-asm operand homes in
`src/backend/prealloc/inline_asm.cpp` and add focused prepared-carrier tests in
`tests/backend/bir/backend_prepared_printer_test.cpp`.

## Watchouts

- Step 2 did not add code; review should treat the existing producer/carrier
  logic and focused prepared-carrier tests as proof for untied scalar GPR input
  and output home publication.
- Do not fold tied `=r,0` reconciliation into the Step 2 completion claim;
  `missing_operand0_home` and `tied_input_output_home_mismatch` remain Step 3.
- Do not implement RV64 object-lowering inference for missing inline-asm homes.
- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Keep `=*m` memory constraints and clobber-only `~{memory}` carriers outside
  this scalar GPR producer/carrier plan unless the audit proves a shared
  invariant requires them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
