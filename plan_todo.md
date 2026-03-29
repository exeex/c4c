# Built-in x86 Assembler Todo

Status: Active
Source Idea: ideas/open/23_backend_builtin_assembler_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 2: Port the minimum parser surface

## Planned Items

- [ ] Step 3: Port the minimum encoder surface
- [ ] Step 4: Port ELF object writing support
- [ ] Step 5: Wire one backend-emitted function through the built-in assembler path
- [ ] Step 6: Validate against external assembler output

## Completed Items

- [x] Activated `ideas/open/23_backend_builtin_assembler_x86_plan.md` into the active runbook
- [x] Step 1: Inventory current x86 backend output and choose the first bounded assembler slice

## Next Intended Slice

- Add focused parser tests for the first bounded `.text`-only return-immediate path.
- Accept only `.intel_syntax noprefix`, `.text`, `.globl`, one symbol label, `mov eax, imm32`, and `ret`.
- Keep every wider directive, operand form, and relocation-producing case explicitly unsupported for now.

## Blockers

- None recorded.

## Resume Notes

- Keep the active plan scoped to the built-in assembler only.
- Do not activate `ideas/open/24_backend_builtin_linker_x86_plan.md` until this assembler runbook is complete.
- Step 1 inventory is recorded in `ideas/open/23_backend_builtin_assembler_x86_plan.md`.
- The current x86 runtime surface splits into 13 pure-assembly cases and 19 LLVM-fallback cases; do not broaden Step 2 to absorb fallback cases.
- The first object-emission slice is the relocation-free `return_zero` / `return_add` shape only.
