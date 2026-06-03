# BIR Runtime Intrinsic Placeholder Identity Contract Runbook

Status: Active
Source Idea: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md

## Purpose

Replace or explicitly document raw placeholder callee string matching for BIR runtime/intrinsic calls with a structured identity contract that prealloc can consume without treating display names as semantic authority.

## Goal

Make runtime/intrinsic placeholder identity explicit while preserving ordinary direct-call link-name authority and existing helper behavior.

## Core Rule

Do not make ordinary direct calls fall back to raw callee strings when `callee_link_name_id` is valid. Runtime/intrinsic placeholders may keep invalid link-name ids only if another explicit identity or a deliberately documented compatibility exception is the authority.

## Read First

- `ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/dynamic_stack.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/frame_plan.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prepared_printer/*`

## Current Targets

- BIR `CallInst` runtime/intrinsic placeholder identity.
- `lower_runtime_intrinsic_inst` and related runtime helper lowering.
- Prealloc variadic helper classification currently reached through raw `llvm.va_*` and `llvm.va_arg.*` names.
- Prealloc local aggregate address publication policy currently reached through raw `llvm.` prefix checks.
- Other prealloc raw runtime/intrinsic callee checks that may need to share the same explicit contract or remain documented exceptions.
- Focused contract tests or dump/proof cases for the affected routes.

## Non-Goals

- Do not rework ordinary direct-call link-name resolution.
- Do not change target ABI behavior.
- Do not broaden into AArch64 call cleanup or unrelated call-plan rewrites.
- Do not weaken existing tests, mark supported paths unsupported, or rely on expectation-only changes.
- Do not introduce testcase-shaped shortcuts for one named intrinsic.

## Working Model

Classify each callee-identity use with one of these labels:

- `ordinary-link-name-authority`
- `runtime-placeholder-identity-authority`
- `runtime-placeholder-compat-string-exception`
- `raw-string-semantic-authority`
- `needs-structured-placeholder-identity`
- `needs-contract-proof`
- `no-action`

Use `todo.md` for running inventories, classification notes, proof commands, and any narrow follow-up findings.

## Execution Rules

- Start with an audit before changing code.
- Prefer a named BIR identity field, enum, or helper contract over scattered raw string checks if the audit shows multiple consumers need the same placeholder semantics.
- If a raw string path remains, document why it is compatibility-only and why no structured identity is available or needed.
- Keep changes behavior-preserving unless the source idea explicitly requires a contract repair.
- Keep proof focused on the affected identity routes and include nearby same-feature cases, not only one named testcase.
- Edit the source idea only if durable intent changes; routine implementation findings belong in `todo.md`.

## Ordered Steps

### Step 1: Inventory Placeholder Identity Producers And Consumers

Goal: classify every relevant runtime/intrinsic placeholder identity producer and prealloc consumer before choosing a contract shape.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/dynamic_stack.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/frame_plan.cpp`
- `src/backend/prealloc/regalloc.cpp`

Actions:

- Inspect `CallInst` fields, `callee`, `callee_link_name_id`, and any existing call/intrinsic kind metadata.
- Trace `lower_runtime_intrinsic_inst` and related runtime helper lowering that intentionally leaves `callee_link_name_id` invalid.
- Inventory prealloc checks that infer semantics from raw `llvm.` or helper callee strings.
- Classify each use with the working-model labels and record file/function evidence in `todo.md`.

Completion check:

- `todo.md` contains a producer/consumer inventory that names which raw string checks are semantic authority, compatibility exceptions, or no-action.

### Step 2: Choose The Placeholder Identity Contract

Goal: decide the smallest explicit contract that satisfies the audited consumers without changing ordinary direct-call identity.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/call_plans.cpp`

Actions:

- Decide whether runtime placeholders need a new structured identity on `CallInst`, a shared helper API, or documented string-only exceptions.
- Preserve invalid `callee_link_name_id` for placeholders when ordinary symbol identity is intentionally absent.
- Define how prealloc should query variadic helper identity and local aggregate publication exceptions.
- Record the selected contract, alternatives rejected, and proof expectations in `todo.md`.

Completion check:

- `todo.md` states the chosen contract and confirms ordinary direct calls keep `callee_link_name_id` as their authority.

### Step 3: Implement The Contract And Update Consumers

Goal: replace undocumented raw-name semantic authority with the chosen explicit contract.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/call_plans.cpp`
- Any audited prealloc raw placeholder consumers that must share the contract.

Actions:

- Add or expose the selected runtime/intrinsic placeholder identity contract.
- Update runtime/intrinsic lowering to populate or expose that contract.
- Update prealloc variadic classification and aggregate-address-publication policy to consume the explicit contract where applicable.
- Leave any retained raw string path clearly documented as a compatibility exception, not silent semantic authority.
- Build after the implementation change before moving to tests.

Completion check:

- Build proof is recorded in `todo.md`, and the diff does not broaden ordinary direct calls into raw-name fallback.

### Step 4: Add Focused Contract Proof

Goal: prove both acceptance routes affected by the old raw-name checks and guard nearby same-feature behavior.

Primary targets:

- Existing backend/prealloc test surfaces that cover variadic helper classification.
- Existing backend/prealloc test surfaces that cover local aggregate address publication policy for runtime/intrinsic calls.
- Prepared printer or BIR dump tests if the selected contract changes visible dumps.

Actions:

- Add or update focused tests for at least one variadic helper classification route.
- Add or update focused tests for at least one local aggregate address publication route affected by the old `llvm.` check.
- Include nearby same-feature cases so proof is not limited to one named testcase.
- Do not weaken expectations or mark supported paths unsupported.

Completion check:

- Narrow proof for the added/updated tests is recorded in `todo.md`, with no expectation downgrades.

### Step 5: Run Broader Validation And Close Readiness

Goal: show the identity contract is stable enough for supervisor acceptance.

Primary targets:

- `todo.md`
- `test_after.log`
- Backend/prealloc regression subset selected by the supervisor.

Actions:

- Run the supervisor-delegated build/test subset for the final implementation slice.
- Summarize the final contract, touched consumers, retained compatibility exceptions, and proof results in `todo.md`.
- Note any separate adjacent initiative only if the implementation discovers one that is outside this source idea.

Completion check:

- `todo.md` contains close-readiness notes, proof results, and any follow-up path needed for separate initiatives.
