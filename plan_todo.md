# Built-in Assembler Boundary Todo

Status: Active
Source Idea: ideas/open/05_backend_builtin_assembler_boundary_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Inventory The Current Assembler Entry Shape

## In Progress

- Audit the existing `src/backend/aarch64/assembler/` headers and stubbed modules against the ref text-first assembler flow.
- Confirm which Step 1 observations are now locked in tests and which still need a tighter code-facing inventory note or helper extraction before Step 2.

## Next Slice

- Decide whether Step 2 should keep growing the text-first assembler entry header or instead extract explicit shared helper declarations for preprocess and expression seams first.

## Remaining Items

- [ ] Step 1: Inventory The Current Assembler Entry Shape
- [ ] Step 2: Decide And Declare The Boundary
- [ ] Step 3: Prove Compile Integration

## Completed

- [x] Activated `ideas/open/05_backend_builtin_assembler_boundary_plan.md` into the active `plan.md` runbook.
- [x] Created `plan_todo.md` aligned to the same source idea and queued the first execution slice.
- [x] Locked the current text-first assembler entry reality in repo tests and `src/backend/aarch64/BINARY_UTILS_CONTRACT.md`.
- [x] Exposed the existing stub `assembler::assemble` entry in `src/backend/aarch64/assembler/mod.hpp` so the staged boundary matches the current compile-linked implementation.
- [x] Verified the slice with `backend_lir_adapter_tests`; a full `ctest --test-dir build -j8 --output-on-failure` run still ends with four unrelated suite failures outside this plan slice.

## Blockers

- Full-suite failures observed outside this slice:
  - `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  - `cpp_positive_sema_decltype_bf16_builtin_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_llvm_initializer_list_runtime_materialization`

## Resume Notes

- Keep this runbook focused on assembler boundary selection and compile integration, not full object emission.
- Use the staged contract from `src/backend/aarch64/BINARY_UTILS_CONTRACT.md` as the compatibility baseline.
- Split out any linker-facing or broad target-generic IR work instead of mutating this plan.
- Current inventory evidence points to a text-first path today: `codegen/emit.cpp` produces GNU-style assembly text, `assembler::parse_asm` preserves raw text as a single placeholder statement, and the mirrored shared helper files are compile-linked stubs.
- The current repo-visible assembler boundary is now explicitly include-reachable as `parse_asm(raw text)` plus `assemble(raw text, output path)`; Step 2 still needs to decide whether that header grows or whether shared helper seams become the next named contract surface.
