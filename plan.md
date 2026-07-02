# RV64 Object Call Variadic Prologue And Return Fragment Cleanup Runbook

Status: Active
Source Idea: ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md

## Purpose

Split RV64 object-route call, variadic, prologue/epilogue, and return fragments only after the lower-level frame, scalar, memory, select, and publication helper APIs are stable enough to keep the movement behavior-preserving.

## Goal

Move late call-family object helpers behind narrow prepared helper APIs without changing call boundaries, variadic admission, saved-register behavior, sret/byval behavior, before-return moves, object fixups, or emitted object behavior.

## Core Rule

This is cleanup, not capability repair. Do not weaken tests, unsupported markers, runtime expectations, diagnostics, ABI behavior, or object bytes to make a slice pass.

## Read First

- `ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`

## Current Targets

- Object-route call fragments.
- Variadic helper/resource fragments.
- Prologue and epilogue fragments.
- Return and before-return movement fragments.
- Any new compiled RV64 object helper file needed to avoid reviving legacy references without build ownership.

## Non-Goals

- Do not implement new call, variadic, prologue, return, or ABI capability.
- Do not change variadic admission, preserved-register behavior, sret/byval behavior, byval lowering, before-return semantics, diagnostics, or runtime expectations.
- Do not move final object module assembly, public ELF entrypoints, or `prepared_function_to_object_function`.
- Do not revive legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, or `returns.cpp` as live destinations unless the slice creates and proves real CMake/build ownership.
- Do not combine this work with gcc_torture expectation changes, unsupported marker changes, or target-side inference.

## Working Model

- Treat legacy call-family files as references only until a slice explicitly creates compiled ownership.
- Keep object encoder, fixup, final module assembly, and public object entrypoints object-side.
- Prefer small extractions that expose dependencies in function signatures over broad carrier structs that recreate object-emission coupling.
- Park helpers when their dependencies would require changing semantics, expanding ownership too broadly, or hiding call/prologue/return coupling behind a catch-all API.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Update `todo.md` after each packet with the exact helper group moved, helpers deliberately parked, and proof command/result.
- Use the validation command below for code-changing proof unless the supervisor delegates a narrower or broader exact command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'
```

- Escalate to reviewer or plan-owner review before moving final object assembly, public entrypoints, or traversal facade responsibilities.

## Steps

### Step 1: Map Call-Family Ownership

Goal: Identify the narrow call, variadic, prologue/epilogue, and return helper groups that can move without semantic changes.

Actions:

- Inspect the current helper clusters in `object_emission.cpp`.
- Map which helpers already belong in `prepared_call_emit.*` or `prepared_frame_emit.*`.
- Identify any helper group that would need a new compiled owner instead of legacy reference files.
- Record parked helpers and the reason they are not safe to move yet in `todo.md`.
- Record the exact proof command the next executor should run for code-changing packets.

Completion check:

- `todo.md` names the first extraction target, retained object-side dependencies, and the exact validation command.
- No implementation files are changed.

### Step 2: Extract Narrow Call Fragment Helpers

Goal: Move the first behavior-preserving call helper group into `prepared_call_emit.*`.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`

Actions:

- Move only helpers whose dependencies are already explicit enough for the prepared call API.
- Keep object encoder, symbol/fixup, module assembly, and traversal-heavy helpers object-side.
- Preserve sret/byval behavior, call-boundary effects, and diagnostics.
- Build and run the delegated proof command.

Completion check:

- The moved helpers compile through `prepared_call_emit.*`.
- The selected backend/object call subset passes.
- `todo.md` records moved helpers, parked helpers, and proof result.

### Step 3: Extract Variadic Resource Helpers

Goal: Move behavior-preserving variadic helper/resource logic only where ownership can be made explicit without admission changes.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_call_emit.*`
- A new compiled helper file only if the slice adds real build ownership.

Actions:

- Isolate variadic predicates or resource helpers that do not alter variadic admission.
- Keep call ABI side effects, object encoder work, and diagnostic-heavy object context object-side unless their dependencies become explicit.
- Do not revive legacy `variadic.cpp` as a live owner without CMake proof.
- Build and run the delegated proof command.

Completion check:

- Variadic behavior and helper-resource authority are unchanged.
- The selected backend/object call and variadic subset passes.
- `todo.md` records any admission-sensitive helpers intentionally parked.

### Step 4: Extract Prologue And Epilogue Frame Helpers

Goal: Move safe prologue/epilogue frame fragments into `prepared_frame_emit.*`.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`

Actions:

- Move only helpers that can expose saved-register and frame-layout dependencies clearly.
- Preserve preserved-register behavior, stack frame setup, and byval/sret interactions.
- Keep object-side helpers parked when they still require encoder or final traversal authority.
- Build and run the delegated proof command.

Completion check:

- Frame helper movement reduces object-side call-family coupling without changing emitted frame behavior.
- The selected backend/object byval, callee-saved, and call subset passes.
- `todo.md` records moved and retained frame helpers.

### Step 5: Extract Return And Before-Return Helpers

Goal: Move return and before-return move helpers only where call/frame dependencies are explicit and behavior remains identical.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_call_emit.*`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.*`
- A new compiled return helper owner only if the slice adds real build ownership.

Actions:

- Move narrow return or before-return helpers after call and frame extraction boundaries are stable.
- Preserve before-return moves, sret behavior, byval behavior, and diagnostics.
- Keep any helper tied to final object traversal or module assembly object-side.
- Build and run the delegated proof command.

Completion check:

- Return helper ownership is narrower and does not create a new catch-all call-family module.
- The selected backend/object call, byval, and runtime subset passes.
- `todo.md` records remaining parked helpers and proof result.

### Step 6: Review And Close Readiness

Goal: Decide whether the source idea is complete or whether remaining call-family work needs a new plan checkpoint.

Actions:

- Review the final diff against the source idea's in-scope and out-of-scope sections.
- Confirm no legacy owner file was revived without build ownership.
- Confirm no admission, ABI, expectation, unsupported marker, or object byte contract changed.
- Record closure readiness or remaining work in `todo.md`.

Completion check:

- `todo.md` states whether the runbook is ready for plan-owner closure evaluation.
- The final proof command and result are recorded.
