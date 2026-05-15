# Inline Assembly Tied-Home Allocation Policy

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/closed/240_aarch64_inline_asm_machine_nodes.md

## Goal

Define allocator and prepared-home policy for alias-aware inline-asm tied
outputs and inputs so coallocation is proven before AArch64 selection accepts a
tied operand pair.

## Why This Exists

The completed inline-asm machine-node route supports numeric ties only when both
sides already have concrete prepared-home agreement. Alias-aware coallocation,
scratch allocation, and spill policy remain allocator/prepared-home decisions
and should not be hidden in template substitution or machine printing.

## In Scope

- Define how prepared homes express tied output/input coallocation guarantees.
- Decide how aliases, register classes, and target-valid homes affect
  inline-asm tied operands.
- Diagnose mismatched, missing, allocator-dependent, or target-invalid tied
  homes explicitly.
- Extend AArch64 selection only after prepared-home policy proves the shared
  home.
- Prove supported alias-aware tied-home behavior and nearby fail-closed cases.

## Out Of Scope

- Do not accept tied operands based only on equal textual names or rendered
  assembly.
- Do not implement a standalone inline-asm allocator in the printer or dispatch
  path.
- Do not weaken current numeric-tie diagnostics or supported-path tests.
- Do not solve clobber ingress or memory/address constraints in this route.

## Acceptance Criteria

- Prepared-home records can prove when tied inline-asm operands share a valid
  target home.
- AArch64 selection accepts tied operands only when that proof is present.
- Tests cover alias-aware supported behavior and mismatched, missing, or
  allocator-dependent fail-closed cases.
- Existing supported inline-asm operand, name, immediate, modifier, side-effect,
  output, and concrete numeric-tie tests continue to pass.
- A regression guard over the supervisor-selected scope passes.

## Completion Notes

- Alias-aware tied output/input operands are accepted only through structured
  prepared-home coallocation authority.
- Missing, mismatched, target-invalid, class-invalid, allocator-dependent, and
  incomplete selected tied records fail closed with explicit backend coverage.
- Close proof passed with the supervisor-selected backend regression guard:
  `test_before.log` and `test_after.log` both report 139/139 passing tests, and
  the monotonic regression checker reported no new failures.

## Reviewer Reject Signals

- Reject testcase-shaped matching that accepts one tied fixture without a
  general prepared-home/coallocation rule.
- Reject implementations that infer ties from final assembly, rendered operand
  spelling, or equal text instead of structured prepared-home facts.
- Reject unsupported expectation downgrades, skipped tests, or weaker
  diagnostics used to claim tied-home progress.
- Reject printer-only or dispatch-only allocation decisions that bypass the
  allocator/prepared-home contract.
- Reject helper renames, expectation rewrites, or classification-only changes
  claimed as allocator policy.
- Reject broad allocator rewrites that do not specifically prove inline-asm
  tied output/input coallocation.
