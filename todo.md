Status: Active
Source Idea Path: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representative Rows And Decide Close Readiness

# Current Packet

## Just Finished

Step 3 implemented tied scalar GPR inline-asm operand-home reconciliation for
supported `=r,0`-style prepared carriers.

Completed work:

- `src/backend/prealloc/inline_asm.cpp` now reconciles supported tied scalar
  GPR operands after prepared result/operand homes have target register
  identities. A tied input is accepted only when its value is scalar GPR-shaped,
  its tied output metadata is a single GPR register output, and the prepared
  result home is a concrete compatible target register.
- Supported tied inputs publish `operand.home` from the authoritative prepared
  `result_home` and publish `tied_home_authority` with the shared target
  register identity, so coherent tied carriers no longer report
  `missing_operand0_home` or `tied_input_output_home_mismatch`.
- Focused prepared-printer coverage proves the AArch64 tied
  output/input/immediate carrier and RV64 tied scalar GPR carriers are complete
  with shared home authority, including an immediate tied input that previously
  had no operand home to publish.
- Existing fail-closed behavior remains in place for unsupported memory/address
  forms, clobber-only carriers, non-register homes, non-GPR/vector/F128
  boundaries, and target-register identity incompatibilities.

## Suggested Next

Execute Step 4 close-readiness validation for the prepared inline-asm operand
home carrier plan. Re-probe representative tied scalar GPR rows/classes and
classify any remaining unsupported rows as out-of-scope follow-up instead of
adding RV64 object-lowering inference.

## Watchouts

- Step 3 uses prepared carrier metadata and result homes as authority. It does
  not infer missing homes in RV64 object lowering.
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
