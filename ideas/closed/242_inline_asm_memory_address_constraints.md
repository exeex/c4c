# Inline Assembly Memory And Address Constraints

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/closed/240_aarch64_inline_asm_machine_nodes.md

## Goal

Support inline-asm memory and address constraints only when prepared homes expose
target-valid memory or address authority, and keep missing-home or
allocator-dependent forms fail-closed.

## Why This Exists

The AArch64 inline-asm machine-node route intentionally avoided inventing a
local scratch allocator or spill planner. Memory and address constraints still
need an explicit prepared-home contract before dispatch and printing can accept
them as real supported operands.

## In Scope

- Define the prepared-home authority required for inline-asm memory and address
  operands.
- Preserve enough structured BIR and prepared facts for AArch64 selection to
  distinguish supported homes from unsupported forms.
- Select and print memory/address operands only when the existing prepared home
  is complete and target-valid.
- Diagnose missing homes, unsupported constraints, and target-invalid address
  forms explicitly.
- Prove at least one supported representative if the current prepared-home
  model can express it; otherwise leave a precise blocker in lifecycle state.

## Out Of Scope

- Do not build an inline-asm-specific register allocator, spill planner, or
  scratch-register convention.
- Do not parse address semantics from rendered AArch64 assembly text.
- Do not weaken unsupported-path tests to turn diagnostic-only forms into
  apparent support.
- Do not solve clobber ingress or alias-aware tied-home coallocation in this
  route.

## Acceptance Criteria

- The prepared inline-asm contract names the memory/address home facts required
  for support.
- AArch64 selection accepts only complete, target-valid structured
  memory/address operands.
- Backend tests prove supported memory/address behavior when expressible and
  fail-closed diagnostics for missing or invalid homes.
- Existing supported inline-asm operand, name, immediate, modifier, side-effect,
  output, and tied-home tests continue to pass.
- A regression guard over the supervisor-selected scope passes.

## Reviewer Reject Signals

- Reject testcase-shaped matching for a single memory or address fixture instead
  of a structured prepared-home rule.
- Reject any implementation that infers memory/address facts from rendered
  assembly text or post-selection formatting.
- Reject unsupported expectation downgrades, skipped tests, or weaker
  diagnostics used to claim constraint progress.
- Reject scratch-register, spill, or allocator policy added locally in dispatch
  or printing without a prepared-home contract.
- Reject helper renames, classification-only changes, or expectation rewrites
  claimed as memory/address capability.
- Reject broad unrelated backend rewrites that do not establish target-valid
  inline-asm memory/address homes.

## Closure Notes

Closed after Step 5 hardened supported and fail-closed backend coverage for the
structured prepared-home rule. The close-time backend regression guard passed in
non-decreasing mode over canonical logs: both `test_before.log` and
`test_after.log` reported 139/139 passing backend tests with no new failures.
