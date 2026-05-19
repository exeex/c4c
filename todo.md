# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Focused 22-Case Family

# Current Packet

## Just Finished

Step 4 repaired the constant-vs-constant fused compare branch printer path.
Both-immediate compare operands are now evaluated from the structured compare
facts and printed as a direct `b` to the selected true/false target instead of
being sent through AArch64 `cmp` operand spelling. The machine-printer unit test
now covers `ult i64 4, 2` folding to the false target without emitting `cmp`.

The delegated proof improved the focused scope from the accepted 6 passed / 21
failed baseline in `test_before.log` to 21 passed / 6 failed in
`test_after.log`. The old
`opcode=compare_branch: fused compare branch operands are not printable` failure
is gone for the 19 constant-vs-constant cases from this packet. Most of that
family now passes; `00200` reaches runtime and fails with output mismatches
instead of stopping at compare-branch printing.

## Suggested Next

Repair the remaining non-encodable register-vs-immediate fused compare branch
forms next. The focused failures still at the old compare-branch printer point
are `00041` (`slt i32 %t0, 5000`) and `00203`
(`slt i64 %t1, -2147483648` / later immediate-left large compare). Handle these
as an operand-form rule, either by materializing the non-encodable immediate or
by routing the compare branch through a supported non-fused form.

## Watchouts

- Constant-vs-constant folding is implemented in the target printer because the
  current conditional terminator lowering interface returns only an instruction;
  folding earlier would also need successor metadata changes to avoid silently
  dropping or misclassifying CFG edges.
- `00200` is no longer a compare-branch printer blocker, but it remains a
  runtime failure after this repair. Treat it separately from the
  non-encodable-immediate printer failures.
- `00207`, `00214`, and `00215` now get past their constant-vs-constant branch
  blockers and fail later in scalar immediate printing (`add`/`xor` immediate
  outside the plain `#imm` range). That is outside this packet's compare-branch
  repair.
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
CTest result was 21 passed / 6 failed. Residual failures are `00041` and
`00203` at non-encodable fused compare-branch operands, `00200` at runtime, and
`00207`, `00214`, `00215` at scalar immediate printer limits.
