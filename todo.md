# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Shared Failure Contract

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella inventory idea 295 to focused fused
compare-branch operand-form idea 296. No implementation has started under this
focused plan.

## Suggested Next

Delegate Step 1 to an executor: inspect representative cases from the 22-case
set and identify the shared fused compare-branch operand-form contract that
causes the machine-printer failure.

## Watchouts

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

Lifecycle-only switch. No build or CTest proof was required or run.
