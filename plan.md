# AArch64 Scalar Expression Control-Value Authority

Status: Active
Source Idea: ideas/open/292_aarch64_scalar_expression_control_value_authority.md

## Purpose

Repair the AArch64 backend scalar value authority path selected by the
post-inventory split.

## Goal

Make AArch64 scalar expression results, branch predicates, return values, and
scalar call arguments consume the authoritative semantic value instead of stale
scratch registers or placeholder locations.

## Core Rule

Do not claim progress by fixing one c-testsuite filename, changing expected
outputs, weakening test contracts, altering allowlists, changing unsupported
classifications, or folding pointer/aggregate, timeout, or printer-admission
work into this scalar owner without fresh proof that it is the same authority
defect.

## Read First

- `ideas/open/292_aarch64_scalar_expression_control_value_authority.md`
- `todo.md`
- Closed AArch64 owner notes for boundary checks:
  - `ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`
  - `ideas/closed/286_aarch64_scalar_call_value_semantics.md`
  - `ideas/closed/287_aarch64_string_global_address_external_call_lowering.md`
  - `ideas/closed/288_aarch64_stack_frame_sp_alignment.md`
  - `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`
  - `ideas/closed/290_aarch64_scalar_parameter_alu_authority.md`
  - `ideas/closed/291_aarch64_call_argument_register_authority.md`

## Current Targets

- AArch64 backend scalar value emission in `src/backend/prealloc/` and
  `src/backend/mir/aarch64/codegen/`.
- Starter c-testsuite representatives:
  - `tests/c/external/c-testsuite/src/00009.c`
  - `tests/c/external/c-testsuite/src/00012.c`
  - `tests/c/external/c-testsuite/src/00056.c`
  - `tests/c/external/c-testsuite/src/00156.c`
  - `tests/c/external/c-testsuite/src/00161.c`
  - `tests/c/external/c-testsuite/src/00211.c`
- Focus areas: scalar expression publication, local scalar load/store
  authority, branch predicates, loop-control values, return values, and scalar
  call arguments.

## Non-Goals

- Do not repair pointer/aggregate address failures such as `src/00019.c`.
- Do not take timeout/hang cases such as `src/00132.c`, `src/00173.c`, or
  `src/00220.c` as ordinary proof for this owner.
- Do not handle compile-stage printer/admission gaps for scalar `sub`, `or`,
  `xor`, or fused compare-branch operands.
- Do not reopen closed AArch64 owners without contradictory generated-code
  evidence.
- Do not edit tests, expectations, allowlists, unsupported classifications,
  timeout policy, CTest registration, runner behavior, parser/sema, or broad
  aggregate ABI code.

## Working Model

The inventory found remaining runtime failures where semantic scalar facts are
available or computable, but later AArch64 consumers read stale fallback
registers, stale callee-saved state, unset argument registers, or placeholder
stack slots. The repair should find the narrowest backend authority layer that
connects prepared scalar values to all scalar consumers, then prove the route
across representatives instead of one filename.

## Execution Rules

- Start with generated-code inspection before editing implementation files.
- Prefer authority plumbing or semantic lowering fixes over local emitted-text
  patches.
- Keep each packet tied to one consumer class or one shared authority primitive.
- After each code-changing packet, run a fresh build or compile proof plus the
  delegated narrow representative subset.
- Escalate to a broader AArch64 backend subset once more than one scalar
  consumer class has changed.
- Keep root `test_before.log` and `test_after.log` use under supervisor
  direction.

## Steps

### Step 1: Reproduce And Locate Scalar Authority Breaks

Goal: Identify the shared backend/prealloc authority layer behind the starter
representatives.

Primary target: generated AArch64 assembly and relevant scalar value authority
records.

Actions:

- Reproduce the starter subset with an explicit timeout.
- Inspect generated assembly for `src/00009.c`, `src/00012.c`, `src/00056.c`,
  `src/00156.c`, `src/00161.c`, and `src/00211.c`.
- Map each stale consumer to its producer: arithmetic result, constant/local
  value, predicate, return, or call argument.
- Locate the backend/prealloc code path that chooses the stale fallback
  register or omits the materialization.
- Record any representatives that belong to pointer/aggregate, timeout, or
  printer-admission scope instead of this scalar owner.

Completion check: `todo.md` names the first semantic repair target, the exact
representative proof command, and the generated-code evidence that ties it to
scalar value authority rather than a closed owner.

### Step 2: Repair The First Scalar Publication Primitive

Goal: Make the first shared scalar consumer class use the authoritative
semantic value.

Primary target: the smallest backend/prealloc or AArch64 codegen surface found
in Step 1.

Actions:

- Implement the narrow authority repair without testcase-shaped matching.
- Add or update focused backend tests only if the repo already has a matching
  local test pattern for the repaired primitive.
- Build the affected target.
- Run the delegated representative subset that proves the repaired primitive.
- Inspect generated assembly enough to show the old stale-register read is
  gone.

Completion check: at least one starter representative passes or advances from
the old stale scalar publication mode, and generated assembly demonstrates the
semantic value now reaches the consumer.

### Step 3: Extend Across Control And Boundary Consumers

Goal: Apply the same scalar authority model to branch predicates, returns, and
scalar call arguments where Step 1 showed the same defect.

Primary target: scalar branch/control and call-boundary consumers in the
AArch64 route.

Actions:

- Reuse the Step 2 authority primitive where possible instead of creating
  consumer-specific shortcuts.
- Repair branch predicate or loop-control consumers for `src/00156.c` and
  `src/00161.c` only when they share the scalar authority defect.
- Repair scalar vararg/call-argument consumers for `src/00056.c` and
  `src/00211.c` only when the format pointer is already valid and the missing
  value is scalar.
- Keep pointer/aggregate arguments and timeout-sensitive cases out of this
  step.
- Run the starter subset and inspect generated assembly for each changed
  consumer class.

Completion check: the starter subset no longer contains stale scalar
expression/control-value failures for the repaired consumer classes, and no
closed-owner or out-of-scope case was absorbed silently.

### Step 4: Owner-Boundary Validation And Close Readiness

Goal: Prove the focused scalar owner is complete enough for lifecycle review.

Primary target: starter representatives plus nearby same-family scalar cases
from the refreshed inventory.

Actions:

- Run the full starter subset with timeout protection.
- Sample nearby scalar expression/control runtime failures from the refreshed
  inventory to reject filename overfit.
- Check that pointer/aggregate, timeout, and compile-stage printer buckets are
  still separated or explicitly documented as follow-on ideas.
- Compare against closed AArch64 owners and record why none need reopening.
- Prepare matching before/after proof logs only if the supervisor requests a
  close or regression-guard pass.

Completion check: `todo.md` records proof, nearby sampling, owner-boundary
evidence, and either a close-ready statement for this source idea or the next
bounded scalar substep.
