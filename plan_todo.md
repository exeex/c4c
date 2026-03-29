# Built-in AArch64 Assembler Todo

Status: Active
Source Idea: ideas/open/06_backend_builtin_assembler_aarch64_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Inventory The Minimal Object-Emission Slice

## In Progress

- Identify the smallest current backend-emitted AArch64 case that can move from staged text to real object emission without requiring broad parser or relocation work.
- Compare the existing backend object-contract fixtures against the mirrored parser, encoder, and ELF-writer stubs to pick the first slice.

## Next Slice

- Prefer a backend-emitted case with minimal directives and no relocations if that is enough to prove real ELF object emission; otherwise choose the smallest relocation-bearing case already locked by current tests.

## Remaining Items

- [ ] Step 1: Inventory The Minimal Object-Emission Slice
- [ ] Step 2: Connect The Boundary To Real Assembler State
- [ ] Step 3: Emit And Compare One Real Object Slice

## Completed

- [x] Closed `ideas/open/05_backend_builtin_assembler_boundary_plan.md` after finishing the compile-integration boundary work and moved it to `ideas/closed/`.
- [x] Activated `ideas/open/06_backend_builtin_assembler_aarch64_plan.md` into the active `plan.md` runbook.
- [x] Created `plan_todo.md` aligned to the same source idea.

## Blockers

- None yet.

## Resume Notes

- The active assembler boundary is text-first: `emit_module(module)` or `assemble_module(module, output_path)` feeds `assemble(AssembleRequest{asm_text, output_path})`.
- Current full-suite baseline remains four unrelated failures outside this new runbook:
  - `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  - `cpp_positive_sema_decltype_bf16_builtin_cpp`
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_llvm_initializer_list_runtime_materialization`
- Keep this runbook focused on the first AArch64 object-emission slice, not general assembler completeness or linker work.
