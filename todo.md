Status: Active
Source Idea Path: ideas/open/367_rv64_helper_free_variadic_entry_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Helper-Free Variadic Entry Facts

# Current Packet

## Just Finished

Activated `ideas/open/367_rv64_helper_free_variadic_entry_contract.md` as the
active runbook.

## Suggested Next

Execute Step 1 from `plan.md`: audit the RV64 helper-free variadic entry
admission facts around the current `src/20030914-2.c` boundary.

Executor packet:

- Owned files: `todo.md`; inspect `src/backend/mir/riscv/codegen/object_emission.cpp`, focused backend tests, and representative logs/dumps as needed.
- Do not edit implementation or tests in this audit packet.
- Record the exact admission gate, available prepared facts, and the Step 2 proof command.
- Validate with `git diff --check -- todo.md`.

## Watchouts

- Do not weaken the explicit variadic runtime-contract gate.
- Do not route `va_arg`, `va_copy`, or byval stack-slot parameter-home work into
  this idea unless the audit proves it is required by the helper-free entry
  admission contract.
- Do not use testcase-name matching for `src/20030914-2.c`.

## Proof

Activation validation:

- `git diff --check -- plan.md todo.md` passed.
- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Audit Helper-Free Variadic Entry Facts'` completed.
