# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Shared Failure Contract

# Current Packet

## Just Finished

Step 1 completed for focused idea 296. I inspected the existing broad capture
in `test_after.log`, representative source/artifact examples `00030`, `00038`,
`00077`, `00200`, and `00215`, plus the current AArch64 compare-branch
lowering/printer contract.

All 22 focused cases fit the same old failure shape:
`printing failed: target instruction spelling failed ... instruction N: cannot
print AArch64 machine node family=branch; opcode=compare_branch: fused compare
branch operands are not printable`. The failing focused set is `00030`,
`00034`, `00037`, `00038`, `00041`, `00054`, `00055`, `00057`, `00059`,
`00076`, `00077`, `00085`, `00092`, `00093`, `00101`, `00127`, `00200`,
`00203`, `00207`, `00212`, `00214`, and `00215`; no outlier in that set failed
outside the fused compare-branch family in the captured log.

Shared operand-form contract: a selected `MachineOpcode::CompareBranch` for a
`BranchConditionForm::FusedCompare` must carry the true target, false target,
condition/prepared value, and printable compare operands in the target
instruction operand vector. The machine printer consumes compare operands from
`instruction.operands[3]` and `instruction.operands[4]`, spelling them as the
`cmp` lhs and rhs. Those slots must therefore be register/immediate operands
resolved from `condition.compare_operands.{lhs,rhs}` with the compare type's
scalar register view. The old failure means the branch record had enough
structured compare facts to select `opcode=compare_branch`, but the printable
operand slots were absent or not printable when the printer reached them.

## Suggested Next

Delegate Step 2 to locate the authority/repair site for installing or
preserving fused compare-branch printable operands. Start around
`install_fused_compare_print_operands`, `make_fused_compare_print_operand`,
`lower_prepared_conditional_branch_terminator`, and
`print_fused_compare_branch`; the likely contract boundary is between
structured compare facts on `BranchConditionRecord` and the target instruction
operand vector consumed by the printer.

## Watchouts

- Do not treat this as a target-label or predicate-spelling problem first: the
  captured failures reached selected `compare_branch` printing and died on lhs
  or rhs operand spelling.
- Existing unit coverage describes the expected printable shape as five
  operands: true target, false target, prepared/condition value, lhs register,
  and rhs immediate/register. Step 2 should verify whether the public
  c-testsuite route is bypassing that installation, losing operands after
  installation, or producing operand records the printer rejects.
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

No broad proof command was run and `test_after.log` was not modified, per the
packet. Evidence used: existing `/workspaces/c4c/test_after.log` broad capture,
representative source/artifact inspection, and AST-backed symbol inventory for
the AArch64 compare-branch lowering/printer functions.
