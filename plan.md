# Inline Assembly Tied-Home Allocation Policy Runbook

Status: Active
Source Idea: ideas/open/243_inline_asm_tied_home_allocation_policy.md

## Purpose

Activate the alias-aware tied-home route from the inline-asm machine-node work
without hiding allocator or prepared-home policy inside template substitution,
dispatch, or machine printing.

## Goal

Support inline-asm tied output/input operands only when structured prepared-home
facts prove a shared target-valid home before AArch64 selection accepts the tie.

## Core Rule

Do not infer tied-home agreement from rendered assembly, equal textual operand
names, testcase spelling, or printer-local allocation choices. AArch64 selection
may accept a tied operand pair only when prepared-home policy proves both sides
coallocate to the same valid target home.

## Read First

- `ideas/open/243_inline_asm_tied_home_allocation_policy.md`
- Inline-asm prepared-home records and numeric-tie plumbing.
- AArch64 inline-asm selection and machine operand construction paths.
- Existing inline-asm tests for operands, names, immediates, modifiers, side
  effects, outputs, memory/address constraints, and concrete numeric ties.

## Current Scope

- Define how prepared homes express tied output/input coallocation guarantees.
- Decide how aliases, register classes, and target-valid homes affect tied
  inline-asm operands.
- Diagnose mismatched, missing, allocator-dependent, or target-invalid tied
  homes explicitly.
- Extend AArch64 selection only after prepared-home policy proves the shared
  home.
- Prove supported alias-aware tied-home behavior and nearby fail-closed cases.

## Non-Goals

- Do not accept tied operands based only on equal textual names or rendered
  assembly.
- Do not implement an inline-asm-specific allocator in the printer, dispatch,
  or selection path.
- Do not weaken current numeric-tie diagnostics or supported-path tests.
- Do not solve clobber ingress in this route.
- Do not solve memory/address constraints in this route.

## Working Model

- Tied output/input pairs are allocator-sensitive operands.
- Prepared-home data is the trust boundary for proving coallocation before
  AArch64 selection accepts a tie.
- Alias-aware support must normalize through structured target-home facts, not
  through final text.
- Missing, mismatched, target-invalid, register-class-invalid, spill-dependent,
  scratch-dependent, or otherwise allocator-dependent homes remain unsupported.
- Diagnostics are part of the contract and should identify why a tie cannot be
  accepted.

## Execution Rules

- Keep each implementation step semantic and structured; reject fixture-shaped
  matching.
- Prefer extending existing prepared-home and inline-asm numeric-tie data paths
  over adding a parallel tied-home protocol.
- Preserve existing supported inline-asm behavior while adding fail-closed
  coverage near the new route.
- Keep allocation decisions in prepared-home or allocator policy surfaces, not
  in AArch64 printing or template rendering.
- Record blockers in `todo.md` when the current prepared-home model cannot
  express an alias-aware coallocation proof; do not mask that gap with
  expectation rewrites.
- For code-changing steps, run at least a fresh build or compile proof plus the
  supervisor-selected narrow inline-asm test subset. Escalate to broader
  regression guard when touched surfaces cross backend contracts.

## Ordered Steps

### Step 1: Inspect Numeric-Tie And Prepared-Home Surfaces

Goal: identify the existing structured facts available for inline-asm tied
output/input operands.

Primary targets:

- Inline-asm BIR representation and prepared-home records.
- Numeric-tie validation and diagnostic plumbing.
- AArch64 inline-asm selection and machine operand construction.
- Existing supported and unsupported tied-operand tests.

Actions:

- Trace how current numeric ties carry prepared-home authority into AArch64
  selection.
- Locate where alias information, register classes, target-valid homes, or
  coallocation facts are flattened, absent, or rejected.
- Separate prepared-home policy gaps from pure diagnostic or test gaps.
- Decide whether an alias-aware supported representative is expressible with
  current prepared-home data.

Completion check:

- `todo.md` records the concrete surfaces to change, the first supported or
  blocked alias-aware representative, and the narrow proof command selected by
  the supervisor.

### Step 2: Define Tied-Home Coallocation Authority

Goal: make the tied output/input support contract explicit in structured
prepared-home facts and diagnostics.

Primary targets:

- Prepared-home or inline-asm operand data structures.
- Numeric-tie validation and unsupported-reason plumbing.
- Register-class and target-home compatibility helpers.

Actions:

- Add or clarify structured fields needed to prove that tied operands share a
  target-valid home.
- Normalize aliases through target-aware home identity instead of equal printed
  names.
- Preserve enough information for selection to distinguish complete
  coallocation proof, missing homes, mismatched homes, incompatible register
  classes, target-invalid homes, and allocator-dependent cases.
- Keep spill-dependent or scratch-dependent ties fail-closed with explicit
  reasons.

Completion check:

- Selection-facing data can answer whether a tied output/input pair has proven
  shared target-home authority without consulting rendered assembly text.
- Existing supported non-tied and concrete numeric-tie tests continue to pass
  under the delegated narrow proof.

### Step 3: Harden Fail-Closed Tied-Home Boundaries

Goal: make unsupported tied-home forms fail closed before selection or printing
can infer a home.

Primary targets:

- Inline-asm preparation diagnostics.
- AArch64 inline-asm selection.
- AArch64 machine operand construction and printing assertions.

Actions:

- Reject tied pairs with missing, mismatched, target-invalid, class-invalid, or
  allocator-dependent homes with explicit diagnostics.
- Reject selected tied records that lack structured shared-home proof before
  printing.
- Preserve current concrete numeric-tie diagnostics while making alias-aware
  gaps more precise.
- Record whether the missing prepared-home policy prevents any supported
  alias-aware representative from being expressible.

Completion check:

- Nearby fail-closed tests prove unsupported tied-home records are rejected for
  structured reasons instead of being accepted through operand spelling or final
  assembly text.
- `todo.md` records any remaining prepared-home blocker if no supported
  alias-aware representative is currently expressible.

### Step 4: Accept Proven Alias-Aware Tied Homes In AArch64 Selection

Goal: bridge proven prepared-home coallocation into AArch64 selected
inline-asm operands without local allocation policy.

Primary targets:

- AArch64 inline-asm selection and dispatch bridge.
- Selected inline-asm machine operand records adjacent to tied operands.
- Machine printer behavior for selected tied operands.

Actions:

- Convert tied output/input pairs into selected operands only when their
  prepared homes prove the same target-valid home.
- Reuse existing target home or selected operand representations where possible
  instead of introducing an unrelated tie-only carrier.
- Print accepted tied operands from structured selected payloads, never from
  rendered template text, final assembly text, or testcase-shaped spelling.
- Keep missing, partial, mismatched, target-invalid, allocator-dependent,
  scratch-dependent, and spill-dependent forms on the explicit unsupported
  diagnostic path.

Completion check:

- At least one alias-aware tied-home representative passes through preparation,
  AArch64 selection, and printing from structured authority.
- Existing fail-closed diagnostics remain covered for tied records without
  proven shared-home authority.

### Step 5: Harden Tests And Regression Scope

Goal: prove the route as a prepared-home capability rather than a narrow
fixture.

Primary targets:

- Backend inline-asm tests covering tied-home support and failure modes.
- Existing supported inline-asm regression subset.

Actions:

- Add supported alias-aware tied-home coverage only for structured expressible
  homes.
- Add fail-closed cases for mismatched homes, missing homes, incompatible
  register classes, target-invalid homes, and allocator-dependent ties.
- Re-run existing supported inline-asm operand, name, immediate, modifier,
  side-effect, output, memory/address, and numeric-tie coverage selected by the
  supervisor.
- Run the supervisor-selected regression guard before closure.

Completion check:

- Supported and fail-closed tests demonstrate a general prepared-home
  coallocation rule.
- The regression guard passes, or `todo.md` records a precise blocker that
  requires a route rewrite instead of expectation downgrades.
