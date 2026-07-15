# AMD64 `va_arg` Unselected Alloca Compatibility Regression Runbook

Status: Active
Source Idea: ideas/open/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md
Activated from: 753 Step 3 full-baseline regression blocker

## Purpose

Restore the verifier-compatible typed-result construction for the unselected
AMD64 overflow `va_arg` compatibility route. This is a regression follow-up to
closed 799, not a reopening or expansion of its selected carrier authority.

## Core Rule

Keep selected AMD64 aggregate overflow carrier authority exactly as accepted in
`c4e820a48`. Repair only the unselected route's typed `LirAllocaOp` result
representation; do not weaken the verifier or derive semantic facts from raw
text.

## Read First

- `ideas/open/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- `src/codegen/lir/ir.hpp` and the `LirAllocaOp` verifier admission path
- `ideas/open/753_lir_memory_va_pointer_authority_convergence.md` (durable
  return record)

## Non-Goals

- Reopening closed 799, changing selected carrier fields or authority,
  aggregate/vector generalization, ABI redesign, Raw-BIR/MIR/emission work,
  or work on 753's receiver handoff.

## Ordered Steps

### Step 1 - Restore typed alloca-result compatibility on unselected overflow routes

Goal: make the unselected compatibility temporary satisfy `LirAllocaOp`'s
typed result contract without changing selected-route behavior.

Actions:

- inspect the `selected` branch in
  `emit_amd64_va_arg_from_overflow` and the `LirAllocaOp` verifier contract;
- replace only the unselected raw-text temporary construction with the
  appropriate typed operand/result representation accepted by that contract;
- retain the selected `fresh_value` path, native carrier construction, and
  `requires_native_memory_va_authority` selection boundary unchanged;
- add or adjust nearby focused coverage only if needed to demonstrate both
  compatible unselected construction and preserved selected authority.

Completion check: fresh `cmake --build --preset default` and
`./build/tests/frontend/frontend_lir_call_type_ref_test` pass, with no
verifier relaxation or selected-carrier contract change.

### Step 2 - Re-establish the parent baseline return gate

Goal: show the correction restores the interrupted full-baseline gate before
753 resumes.

Actions:

- have the supervisor run the exact matching full baseline and compare it with
the accepted 3037/3037 baseline; diagnose any remaining loss before return;
- record the accepted proof and return 753 to Step 3 at its receiver-handoff
action, without performing that handoff under this blocker.

Completion check: matching full baseline is 100% passing and the supervisor
can switch back to 753 using its durable return record.
