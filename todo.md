# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Focused 22-Case Family

# Current Packet

## Just Finished

Step 4 repaired the remaining register-vs-immediate fused compare branch
printer forms whose immediates do not fit AArch64 `cmp #imm`. The printer now
materializes a non-encodable integer immediate into a reserved MIR scratch GP
register and emits a register-register `cmp`, including swapped immediate-left
forms. The machine-printer unit coverage now includes an
`slt i64 %lhs, -2147483648` branch that materializes the RHS through `x9` before
`cmp x12, x9`.

The delegated proof improved the focused scope from the accepted 21 passed / 6
failed baseline in `test_before.log` to 23 passed / 4 failed in
`test_after.log`. `00041` and `00203` now pass and no longer fail at
`opcode=compare_branch: fused compare branch operands are not printable`.

## Suggested Next

Hand back to the supervisor for the next packet choice. The compare-branch
printer blockers in this focused family are repaired; the remaining focused
failures are outside this packet's fused compare-branch operand-form scope.

## Watchouts

- `00200` remains a runtime mismatch after this repair.
- `00207`, `00214`, and `00215` still fail later in scalar immediate printing
  (`add`/`xor` immediate outside the plain `#imm` range). That is outside this
  packet's compare-branch repair.
- The fused compare-branch materialization path deliberately uses reserved MIR
  scratch GP registers and avoids the compare source register if it already
  occupies the first scratch register.
- Do not match c-testsuite filenames, test numbers, or exact emitted
  instruction strings.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.
- Keep this owner limited to fused compare-branch operand publication/printing;
  do not absorb the remaining scalar machine-printer, `lir_to_bir` admission,
  runtime, or timeout buckets from umbrella idea 295.
- Proving only one known failing case is not enough; the full 22-case focused
  family must be checked before claiming capability progress.

## Proof

Ran the delegated proof exactly:
`cmake --build build --target c4cll backend_aarch64_branch_control_lowering_test backend_aarch64_branch_compare_records_test backend_aarch64_compare_branch_candidate_records_test backend_aarch64_prepared_branch_records_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_(branch_control_lowering|branch_compare_records|compare_branch_candidate_records|prepared_branch_records|machine_printer)|c_testsuite_aarch64_backend_src_(00030|00034|00037|00038|00041|00054|00055|00057|00059|00076|00077|00085|00092|00093|00101|00127|00200|00203|00207|00212|00214|00215)_c)$' --output-on-failure | tee test_after.log`

Result in `test_after.log`: build succeeded; backend unit tests passed; focused
CTest result was 23 passed / 4 failed. `00041` and `00203` passed. Residual
failures are `00200` at runtime, and `00207`, `00214`, `00215` at scalar
immediate printer limits.
