# BIR Prealloc Missing Call ABI Fallback Boundary Runbook

Status: Active
Source Idea: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md

## Purpose

Constrain or eliminate prealloc paths that reconstruct missing call ABI facts from scalar types after BIR lowering should already have published call, parameter, result, and return ABI metadata.

## Goal

Make the boundary between required BIR ABI carriers and any retained prealloc compatibility fallback explicit, tested, and unable to become silent primary ABI authority.

## Core Rule

Do not add new type-only ABI inference paths in prealloc. Target-sensitive promotion and physical placement may remain in prealloc, but ordinary lowered calls and functions must arrive with the BIR ABI carriers that prealloc consumes.

## Read First

- `ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- Existing backend/prealloc tests for call ABI classification, prepared call plans, frame stack calls, and AArch64 call movement.

## Current Targets

- `CallInst::arg_abi` and `CallInst::result_abi` consumers.
- `Function::params[*].abi` and `Function::return_abi` consumers.
- `legalize.cpp::infer_call_arg_abi_impl`, `infer_call_result_abi`, `infer_function_return_abi`, and missing-ABI fallback sites in `legalize_module`.
- Type-only register-bank fallbacks in `call_plans.cpp`.
- Scalar return fallback behavior in `regalloc/call_moves.cpp`.
- Focused proof that ordinary lowered calls/functions carry ABI metadata before prealloc planning.

## Non-Goals

- Do not move physical register, stack slot, clobber, or move planning into BIR.
- Do not change target ABI classification for correctly populated BIR facts.
- Do not rewrite runtime helper ABI modeling; that belongs to a separate follow-up idea.
- Do not weaken tests, mark supported paths unsupported, or claim progress through expectation-only changes.
- Do not broaden into an unrelated ABI redesign.

## Working Model

Classify each missing-ABI fallback site with one of these labels:

- `bir-carrier-required`
- `prealloc-target-placement-authority`
- `compat-bootstrap-repair`
- `ordinary-path-unreachable`
- `missing-bir-lowering-fix-needed`
- `needs-contract-proof`
- `no-action`

Use `todo.md` for the running inventory, classification evidence, proof commands, and any separate follow-up candidates discovered during execution.

## Execution Rules

- Start with an audit before changing code.
- Preserve target-sensitive legalization and physical placement in prealloc.
- Convert undocumented ordinary-path reconstruction into assertions or earlier BIR lowering fixes when the source idea supports it.
- If a fallback remains, name the compatibility path and prove it cannot silently own ordinary scalar call ABI classification.
- Keep proof broad enough to cover args, params, call results, and function returns where they are in scope.
- Edit the source idea only if durable intent changes or a separate initiative must be recorded.

## Ordered Steps

### Step 1: Inventory Missing ABI Fallback Sites

Goal: identify every prealloc path that reconstructs call, parameter, result, or return ABI facts when BIR metadata is absent.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/calling.cpp`

Actions:

- Inspect fallback helpers and call sites for `CallInst::arg_abi`, `CallInst::result_abi`, `Function::params[*].abi`, and `Function::return_abi`.
- Trace which paths use scalar type shape, register bank, or synthetic defaults when ABI metadata is absent.
- Separate target-sensitive placement decisions from missing-BIR-fact reconstruction.
- Record file/function evidence and classify each site with the working-model labels in `todo.md`.

Completion check:

- `todo.md` contains a complete fallback inventory with provisional status for call args, params, call results, and function returns.

### Step 2: Decide Required Carriers And Compatibility Paths

Goal: decide which missing-ABI fallbacks should become ordinary-path requirements, which need earlier BIR repair, and which may remain documented compatibility/bootstrap repairs.

Primary targets:

- BIR call/function metadata producers.
- Prealloc legalization and planning consumers from Step 1.
- Existing tests that construct BIR directly without full lowering.

Actions:

- For each fallback, decide whether BIR must always publish the carrier for ordinary lowered code.
- Identify direct test-fixture or bootstrap cases that legitimately need a retained fallback.
- Define the smallest assertion/helper/documentation shape needed to prevent silent primary authority in prealloc.
- Record the selected boundary, rejected alternatives, and proof expectations in `todo.md`.

Completion check:

- `todo.md` states the selected contract for every audited fallback and identifies the implementation steps needed before tests.

### Step 3: Repair Or Constrain Missing ABI Fallbacks

Goal: implement the selected boundary without changing ABI classification for correctly populated BIR facts.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- BIR lowering files only if Step 2 proves an ordinary producer is missing metadata.

Actions:

- Replace ordinary-path missing-ABI reconstruction with assertions, explicit compatibility guards, or earlier BIR metadata repair as selected in Step 2.
- Keep target-sensitive promotion and physical placement logic in prealloc.
- Document any retained compatibility fallback at the consumer surface that still needs it.
- Build after code changes before moving to focused tests.

Completion check:

- Build proof is recorded in `todo.md`, and the diff does not introduce new type-only ABI inference authority in prealloc.

### Step 4: Add Focused ABI Carrier Proof

Goal: prove ordinary lowered calls/functions carry ABI metadata into prealloc and that retained fallbacks are not the primary scalar ABI authority.

Primary targets:

- Backend/prealloc tests that cover ordinary source call args and call results.
- Backend/prealloc tests that cover function params and returns.
- Direct-BIR or compatibility tests only where Step 2 retained a fallback.

Actions:

- Add or update focused tests for ordinary call argument ABI metadata.
- Add or update focused tests for ordinary call result and function return ABI metadata.
- Add or update focused tests for parameter ABI metadata where the audited fallback can otherwise reconstruct it.
- Include retained compatibility paths only with explicit naming and narrow expectations.
- Do not weaken expectations or mark supported paths unsupported.

Completion check:

- Narrow proof for the added/updated tests is recorded in `todo.md`, and coverage is not limited to one scalar call shape.

### Step 5: Run Broader Validation And Close Readiness

Goal: prove the boundary is stable enough for supervisor acceptance and close evaluation.

Primary targets:

- `todo.md`
- `test_after.log`
- Backend/prealloc regression subset selected by the supervisor.

Actions:

- Run the supervisor-delegated build/test subset for the final implementation slice.
- Summarize removed, asserted-unreachable, and retained compatibility fallback statuses in `todo.md`.
- Note any separate adjacent initiative only if execution discovers work outside this source idea.

Completion check:

- `todo.md` contains close-readiness notes, proof results, and any follow-up path needed for separate initiatives.
