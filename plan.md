# AArch64 Fused Compare-Branch Operand Forms

Status: Active
Source Idea: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Supersedes: umbrella inventory runbook from ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the AArch64 backend path that fails to publish or print valid operands
for fused compare-branch forms.

## Goal

Make the 22 fused compare-branch compile-stage failures advance through
machine printing by fixing semantic operand-form authority for the underlying
compare-branch lowering/printing path.

## Core Rule

Fix compare-branch operand semantics. Do not match c-testsuite filenames,
rewrite expectations, downgrade supported tests, change runners, or alter CTest
registration to improve counts.

## Read First

- `ideas/open/296_aarch64_fused_compare_branch_operand_forms.md`
- `todo.md`
- The current failing cases:
  `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`, `00057`,
  `00059`, `00076`, `00077`, `00085`, `00092`, `00093`, `00101`, `00127`,
  `00200`, `00203`, `00207`, `00212`, `00214`, `00215`

## Current Targets

- AArch64 compare-branch lowering and machine-printer operand-form handling.
- The fused compare-branch instruction family, including zero/nonzero and
  compare-result branch forms reached by the failing c-testsuite cases.
- Shared operand publication rules that let the printer consume valid operands
  without testcase-shaped bypasses.

## Non-Goals

- Do not change test expectations, allowlists, unsupported classifications,
  runner behavior, timeout policy, or CTest registration.
- Do not add filename, test-number, or exact emitted-instruction matching.
- Do not reopen closed runtime owners 285 through 294 from these compile-stage
  failures.
- Do not absorb the unrelated scalar machine-printer, `lir_to_bir` admission,
  runtime, or timeout buckets from umbrella idea 295.

## Working Model

The umbrella inventory found 38 machine-printer failures. The largest coherent
subset is 22 fused compare-branch cases whose failure shape points at a shared
operand-form contract between lowering and printing. The repair should make the
lowering path publish the operands the AArch64 machine printer expects, or make
the printer consume the semantic operands that lowering is supposed to expose,
without special-casing individual tests.

## Execution Rules

- Start with one or two representative failing cases, but prove the repaired
  rule against the full 22-case focused subset before claiming capability
  progress.
- Inspect the generated machine/LIR/BIR path enough to identify the shared
  compare-branch operand contract.
- Prefer a small semantic lowering or printer contract fix over broad backend
  rewrites.
- Keep generated proof in `test_after.log` or another supervisor-approved
  artifact; do not create extra root-level logs.
- Escalate back to lifecycle planning if evidence shows the 22 cases split
  into separate semantic owners.

## Steps

### Step 1: Confirm The Shared Failure Contract

Inspect representative failing cases from the 22-case set and identify the
specific fused compare-branch operand form that reaches the machine-printer
failure.

Completion check: `todo.md` records the representative cases inspected, the
shared operand contract, and any cases that do not fit the fused
compare-branch family.

### Step 2: Locate Operand Authority

Trace where compare-branch operands are created, normalized, and consumed by
the AArch64 machine printer.

Completion check: `todo.md` names the lowering/printer functions or data
structures that own the operand-form contract, plus the smallest viable repair
site.

### Step 3: Repair Semantic Operand Publication Or Printing

Implement the narrow semantic fix so fused compare-branch lowering publishes
printer-consumable operands, or the printer consumes the canonical semantic
operands produced by lowering.

Completion check: the representative compile-stage failures no longer fail at
the old machine-printer operand-form point, with no expectation, runner,
allowlist, unsupported-classification, timeout, or CTest-registration changes.

### Step 4: Prove The Focused 22-Case Family

Run the supervisor-delegated proof for the full 22-case focused subset and
compare the old failure mode against the new result.

Completion check: `todo.md` records the exact proof command, result, and
remaining failures. Any residual failures must be classified as either outside
the fused compare-branch operand-form owner or a blocker for this idea.

### Step 5: Broader Backend Sanity

Run the supervisor-selected broader backend sanity check after the focused
subset is repaired or clearly blocked.

Completion check: broader proof is recorded without new backend regressions
attributable to the compare-branch operand-form change.
