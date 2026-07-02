# RV64 Object Prepared Module Admission Shell Cleanup Runbook

Status: Active
Source Idea: ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md

## Purpose

Reduce RV64 object-route entrypoint coupling by factoring prepared module admission and diagnostic construction into a thin behavior-preserving shell.

## Goal

Separate admission/diagnostic orchestration from function conversion, text module assembly, data-object assembly, relocation handling, and ELF writing without changing observable behavior.

## Core Rule

Preserve prepared-object rejection behavior, diagnostic category/text meaning, public wrapper behavior, and pass/fail accounting. This plan is cleanup only, not RV64 capability repair.

## Read First

- `ideas/open/536_rv64_object_prepared_module_admission_shell_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.hpp`

## Current Targets

- Keep `build_rv64_prepared_text_object_module_with_diagnostics` and `write_rv64_prepared_relocatable_elf_object_with_diagnostics` behavior stable.
- Isolate only prepared module admission and diagnostic construction.
- Keep parallel-copy diagnostics, prepared object consumer categories, and public wrappers unchanged.

## Non-Goals

- Do not move `prepared_function_to_object_function`, `fragment_for_prepared_instruction`, text module assembly, prepared data-object assembly, relocation handling, or ELF writing.
- Do not infer missing prepared facts from BIR, target text, object output, or testcase shape.
- Do not change gcc_torture expectations, unsupported markers, pass/fail accounting, or RV64 capability support.

## Working Model

- `object_emission.cpp` should remain the compatibility surface for public object emission entrypoints.
- `prepared_module_emit.*` may own the narrow admission shell and diagnostic assembly helpers when that reduces entrypoint coupling.
- Existing conversion and object assembly helpers should remain in their current owner unless a later source idea owns them.

## Execution Rules

- Work in small behavior-preserving steps.
- Prefer moving coherent helper groups over broad rewrites.
- Keep compatibility wrappers if call sites or public headers still need them.
- For each code-changing step, run at least:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_object_consumer_contract|riscv_object_emission|object_model_records|cli_riscv64_variadic_entry_missing_contract_obj|cli_riscv64_unsupported_global_diagnostic_obj)'`
- Escalate to broader validation if the diff touches public entrypoint behavior, diagnostic contracts, or object assembly beyond the admission shell.

## Ordered Steps

### Step 1: Map Admission And Diagnostic Boundaries

Goal: Identify the exact admission and diagnostic construction logic that can move without changing conversion, assembly, relocation, or ELF writing.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Inspect `build_rv64_prepared_text_object_module_with_diagnostics` and `write_rv64_prepared_relocatable_elf_object_with_diagnostics`.
- List helper calls that construct diagnostics or validate prepared module admission.
- Mark helpers that must stay parked because they perform function conversion, text module construction, prepared data-object assembly, relocation handling, or ELF writing.
- Record the proposed move set and parked set in `todo.md`.

Completion check:

- `todo.md` identifies the owned admission/diagnostic helper set and the explicit non-moved assembly/conversion set.

### Step 2: Extract The Prepared Module Admission Shell

Goal: Move the narrow admission and diagnostic helper set into `prepared_module_emit.*` while preserving public entrypoints.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.hpp`

Actions:

- Add or extend `prepared_module_emit.*` declarations for the selected admission shell helpers.
- Move implementation details only for admission and diagnostic construction.
- Leave high-risk conversion, text module assembly, data-object assembly, relocation handling, and ELF writing in their existing ownership.
- Keep wrapper signatures and public behavior stable.

Completion check:

- The focused validation command passes.
- Diagnostics and public entrypoint behavior remain unchanged.

### Step 3: Prune Or Park Remaining Wrappers

Goal: Remove only now-dead forwarding wrappers and leave live compatibility wrappers intentionally parked.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Inspect call sites after Step 2.
- Delete wrappers that no longer have call sites or compatibility purpose.
- Keep wrappers that still bridge public object emission entrypoints, relocation/ELF writing, conversion, or object assembly.
- Update `todo.md` with the rationale for kept wrappers.

Completion check:

- The focused validation command passes.
- Any remaining wrapper is either called or documented in `todo.md` as intentionally parked.

### Step 4: Close-Readiness Review

Goal: Confirm the extracted admission shell satisfies the source idea without hiding the same monolithic coupling behind a new file.

Actions:

- Compare the final diff against the source idea and this runbook.
- Verify no diagnostic categories, rejection text meaning, unsupported markers, expectation files, or pass/fail accounting changed.
- Verify `prepared_function_to_object_function`, `fragment_for_prepared_instruction`, text module assembly, prepared data-object assembly, relocation handling, and ELF writing were not moved.
- Record final focused proof and any residual parked-wrapper notes in `todo.md`.

Completion check:

- The runbook is ready for supervisor or reviewer closure evaluation, with focused proof recorded in `test_after.log` or explained in `todo.md`.
