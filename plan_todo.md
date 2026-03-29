# Built-in AArch64 Assembler Todo

Status: Active
Source Idea: ideas/open/06_backend_builtin_assembler_aarch64_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Emit And Compare One Real Object Slice

## In Progress

- Run the full regression guard after the new direct return-immediate slice landed through the built-in parser, encoder, and ELF writer.
- Decide whether the next bounded slice should tighten built-in-versus-external object comparison for `return_add` or move to the smallest relocation-bearing case.

## Next Slice

- Keep the first built-in slice bounded to `.text` plus one global function symbol with `mov w0, #imm` and `ret`, then extend comparison coverage before taking on AArch64 relocations.

## Remaining Items

- [x] Step 1: Inventory The Minimal Object-Emission Slice
- [x] Step 2: Connect The Boundary To Real Assembler State
- [ ] Step 3: Emit And Compare One Real Object Slice

## Completed

- [x] Closed `ideas/open/05_backend_builtin_assembler_boundary_plan.md` after finishing the compile-integration boundary work and moved it to `ideas/closed/`.
- [x] Activated `ideas/open/06_backend_builtin_assembler_aarch64_plan.md` into the active `plan.md` runbook.
- [x] Created `plan_todo.md` aligned to the same source idea.
- [x] Picked `backend_contract_aarch64_return_add_object` as the minimal first slice because it needs only `.text`, `.globl`, `.type`, one function label, `mov w0, #imm`, and `ret`, with no relocations.
- [x] Replaced the parser placeholder with line-oriented statement parsing for the minimal assembler slice.
- [x] Wired the active assembler seam to a real minimal path: parse backend-emitted text, encode `mov wN, #imm` and `ret`, and write an ELF64 AArch64 relocatable object containing `.text`, `.symtab`, `.strtab`, and `.shstrtab`.
- [x] Tightened backend adapter tests so the named assembler seam and compatibility overload both emit a real object for the direct return-immediate slice.
- [x] Rebuilt from scratch and reran the full `ctest` suite; the repo remained monotonic at the same four known unrelated failures recorded in the baseline.

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
- The current built-in assembler slice is intentionally narrow: `.text` only, one global `%function` symbol, and `mov wN, #imm` plus `ret`; relocation-bearing cases still fall back to `object_emitted = false`.
