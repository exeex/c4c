# RV64 Object Function Traversal Facade Cleanup Runbook

Status: Active
Source Idea: ideas/open/542_rv64_object_function_traversal_facade_cleanup.md

## Purpose

Create a small RV64 object-function traversal facade around `prepared_function_to_object_function` after the lower-level helper-family APIs have been peeled out.

## Goal

Reduce central coupling in RV64 object function emission without changing admission semantics, diagnostics, traversal order, prepared lookup construction, function name matching, fixups, or emitted object behavior.

## Core Rule

This is cleanup, not capability repair. Do not weaken tests, unsupported markers, runtime expectations, diagnostics, traversal behavior, or object bytes to make a slice pass.

## Read First

- `ideas/open/542_rv64_object_function_traversal_facade_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.hpp`
- The prepared family helper APIs already extracted for frame, memory, scalar/select, edge publication, call, variadic, and return fragments.

## Current Targets

- `prepared_function_to_object_function` in `object_emission.cpp`.
- The traversal/event loop around `prepare::make_prepared_object_function_traversal`.
- A narrow prepared-function facade in `prepared_function_emit.*` or a new compiled object-route facade file if the first mapping step proves that is cleaner.
- Explicit dependency interfaces for prepared lookups, control-flow facts, stack frame facts, select-edge placements, carrier alias authority, and dependency operand authority.

## Non-Goals

- Do not move `fragment_for_prepared_instruction` before its family helper fanout has an explicit API boundary.
- Do not move final object module assembly, public ELF entrypoints, data object emission, symbol/fixup ownership, relocation mapping, section emission, or module layout.
- Do not change admission semantics, diagnostics, block traversal order, function name matching, prepared lookup construction, or object bytes.
- Do not add RV64 capability repair, target-side inference, gcc_torture expectation changes, unsupported marker changes, or runtime expectation changes.
- Do not hide `fragment_for_prepared_instruction` fanout behind a new monolithic dispatcher.

## Working Model

- Treat `object_emission.cpp` as the owner of object encoder, final module assembly, public entrypoints, and late symbol/fixup/module boundaries.
- Treat `prepared_function_emit.*` as a possible facade owner only when dependencies are visible in signatures and the file does not become a second all-purpose object emission module.
- Prefer small interfaces that expose dependency sets over carrier structs that silently recreate the old central coupling.
- Keep diagnostics and traversal order byte-for-byte behavior-preserving unless the source idea is explicitly revised by the plan owner.
- Park helpers when moving them would require hiding object-side authority or combining this idea with the later data/symbol/fixup cleanup.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Update `todo.md` after each packet with the exact group moved, dependencies intentionally exposed, helpers deliberately parked, and proof command/result.
- Use the validation command below for code-changing proof unless the supervisor delegates a narrower or broader exact command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(riscv_object_emission|prepared_object_consumer_contract|object_model_records|obj_runtime_rv64_|codegen_route_riscv64_)'
```

- Escalate to broader `^backend_` proof before accepting a slice that touches traversal order, diagnostic aggregation, object module layout, relocation attachment, section emission, or ELF writing.
- Escalate to reviewer or plan-owner review before moving final module assembly, public entrypoints, `fragment_for_prepared_instruction`, or data/symbol/fixup responsibilities.

## Steps

### Step 1: Map Function Traversal Facade Ownership

Goal: Identify the smallest behavior-preserving facade boundary around `prepared_function_to_object_function`.

Actions:

- Inspect the current setup, admission, lookup, prologue, traversal, fallback traversal, terminator, and return paths inside `prepared_function_to_object_function`.
- Map which dependencies can be made explicit through `prepared_function_emit.*` or a new compiled facade file.
- Identify which pieces must remain parked in `object_emission.cpp`, especially `fragment_for_prepared_instruction`, object encoder utilities, symbol/fixup/module work, and diagnostic-heavy object context.
- Confirm that prior helper-family APIs are available for the facade boundary and record any missing dependency as parked work rather than expanding this source idea.
- Record the exact proof command the next executor should run for code-changing packets.

Completion check:

- `todo.md` names the first extraction target, retained object-side dependencies, and the exact validation command.
- No implementation files are changed.

### Step 2: Extract Facade Result And Admission Shell

Goal: Move only the narrow result/admission shell needed by the facade while keeping behavior and diagnostics identical.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Move or expose the minimal prepared-function result/rejection helpers only if their dependencies stay explicit.
- Keep public object module/image result types and public entrypoints in `object_emission.hpp` / `object_emission.cpp`.
- Preserve empty-name, missing-body, variadic admission, atomic-operation, stack-frame, parameter-home, and variadic-helper diagnostics.
- Do not move traversal loops or instruction fragment fanout in this step.
- Build and run the delegated proof command.

Completion check:

- The result/admission shell compiles through the selected facade owner without changing diagnostic strings or admission outcomes.
- The selected backend/object facade subset passes.
- `todo.md` records moved helpers, parked helpers, and proof result.

### Step 3: Extract Traversal Context Assembly

Goal: Move the dependency-gathering and context assembly around prepared-function traversal without hiding dependencies.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_function_emit.*`
- A new compiled facade file only if Step 1 selected it and build ownership is added in the same slice.

Actions:

- Move narrow setup for prepared lookups, dependency operand authorities, carrier alias authorities, select-edge source producer placements, frame/storage/inline-asm plan lookup, and stack frame sizing only when signatures expose those inputs clearly.
- Keep object-function fragment appending, object encoder work, and final module assembly object-side unless Step 1 proved a narrower facade boundary.
- Preserve prepared lookup construction, function name matching, and diagnostic order.
- Build and run the delegated proof command.

Completion check:

- Traversal context construction is narrower and no new catch-all context hides dependencies.
- The selected backend/object facade subset passes.
- `todo.md` records exposed dependencies and any retained object-side setup.

### Step 4: Extract Object Function Traversal Loop Facade

Goal: Move the smallest traversal/event loop surface that reduces central coupling while preserving event order and fragment behavior.

Primary target:

- `src/backend/mir/riscv/codegen/prepared_function_emit.*`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Extract only the loop/facade boundary that delegates to already-extracted family helper APIs.
- Preserve `prepare::make_prepared_object_function_traversal` event order, fallback block traversal behavior, label emission order, move-bundle classification, select-publication admission, terminator handling, and prepared-consumer diagnostics.
- Keep `fragment_for_prepared_instruction` fanout parked unless an explicit API boundary prevents the new facade from becoming another monolith.
- Keep final object module assembly and symbol/fixup/module work parked.
- Build and run the delegated proof command.

Completion check:

- Function traversal is owned by a small facade with visible dependencies.
- Diagnostics, traversal order, and object bytes are unchanged under the selected proof subset.
- `todo.md` records any remaining parked traversal or fragment helpers.

### Step 5: Review And Close Readiness

Goal: Decide whether the source idea is complete or whether remaining facade work needs a new plan checkpoint.

Actions:

- Review the final diff against the source idea's in-scope and out-of-scope sections.
- Confirm the facade reduced coupling and did not create a second all-purpose object file.
- Confirm `fragment_for_prepared_instruction` fanout, data/symbol/fixup/module assembly, public entrypoints, diagnostics, traversal order, and object byte contracts were not changed outside the source idea.
- Confirm no tests, expectations, unsupported markers, or runtime contracts were weakened.
- Record closure readiness or remaining work in `todo.md`.

Completion check:

- `todo.md` states whether the runbook is ready for plan-owner closure evaluation.
- The final proof command and result are recorded.
