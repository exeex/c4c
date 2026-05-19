# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Semantic Operand Publication Or Printing

# Current Packet

## Just Finished

Step 3 completed the immediate-left fused compare-branch normalization slice at
the printer boundary. `print_fused_compare_branch` now derives local printable
compare operands by first trying the published order and then trying the swapped
order with the swapped predicate. This preserves `BranchConditionRecord` and
`compare_branch_candidate` semantic facts unchanged while emitting legal AArch64
`cmp reg, #imm` order for forms such as `slt i32 1000, %t8`.

Added focused machine-printer coverage for an immediate-left `slt` fused
compare branch. The test keeps the semantic compare facts as `1000 < %rhs`,
publishes printable operands in the old illegal immediate/register order, and
expects `cmp w13, #1000` with `b.gt`.

The representative c-testsuite case `00030` now passes in the delegated proof
and no longer fails at the old fused compare-branch operand-form printer point.

## Suggested Next

Delegate Step 4 to decide whether the remaining 21 focused c-testsuite failures
belong to this idea as additional operand-form work or should be split. The next
coherent packet should classify and repair constant-vs-constant fused compares
or non-encodable immediate compare operands without weakening tests.

## Watchouts

- Residual focused failures still report the generic
  `fused compare branch operands are not printable` diagnostic, but sampled
  prepared BIR shows different shapes from the repaired immediate-left register
  case: constant-vs-constant compares such as `ne i32 1, 0`, `ult i64 4, 2`,
  and `eq i64 2, 2`; out-of-range immediate RHS forms such as
  `slt i32 %t0, 5000`; and negative immediate RHS forms such as
  `slt i64 %t1, -2147483648`.
- The current repair intentionally does not constant-fold branch direction,
  materialize compare immediates, add scratch-register compare lowering, or
  mutate semantic compare records.
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

Focused pre-proof:
`cmake --build build --target backend_aarch64_machine_printer_test && ctest --test-dir build -R '^backend_aarch64_machine_printer$' --output-on-failure`
passed.

Delegated proof command run exactly:
`cmake --build build --target c4cll backend_aarch64_branch_control_lowering_test backend_aarch64_branch_compare_records_test backend_aarch64_compare_branch_candidate_records_test backend_aarch64_prepared_branch_records_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_(branch_control_lowering|branch_compare_records|compare_branch_candidate_records|prepared_branch_records|machine_printer)|c_testsuite_aarch64_backend_src_(00030|00034|00037|00038|00041|00054|00055|00057|00059|00076|00077|00085|00092|00093|00101|00127|00200|00203|00207|00212|00214|00215)_c)$' --output-on-failure | tee test_after.log`

Result recorded in `test_after.log`: build succeeded; the five focused AArch64
unit tests passed; `c_testsuite_aarch64_backend_src_00030_c` passed; the
remaining 21 c-testsuite focused cases failed at the generic fused
compare-branch operand printer diagnostic and are classified above for the next
packet.
