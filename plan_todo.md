# Built-in x86 Assembler Todo

Status: Active
Source Idea: ideas/open/23_backend_builtin_assembler_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 5: Wire one backend-emitted function through the built-in assembler path
- Iteration slice: add the first production x86 backend helper that emits
  `return_zero` / `return_add` through `x86::assembler::assemble(...)` so one
  backend-owned object path uses the built-in assembler instead of staging raw
  assembly text only.

## Planned Items

- [ ] Step 5: Wire one backend-emitted function through the built-in assembler path
- [ ] Step 6: Validate against external assembler output

## Completed Items

- [x] Activated `ideas/open/23_backend_builtin_assembler_x86_plan.md` into the active runbook
- [x] Step 1: Inventory current x86 backend output and choose the first bounded assembler slice
- [x] Step 2: Port the minimum parser surface
- [x] Step 3: Port the minimum encoder surface
- [x] Step 4: Port the first ELF64 relocatable object writer slice for the single-function
  `.text` / `.symtab` / `.strtab` return-immediate path

## Next Intended Slice

- Expose the production x86 backend handoff seam so one minimal backend-emitted
  function can request object emission through the built-in assembler.
- Preserve the current bounded slice: single global function, `.text` only, no
  relocations, and no multi-function/data-section support yet.
- Keep relocations, data sections, and wider parser/encoder coverage deferred
  until later Step 5/6 follow-on slices.

## Blockers

- None recorded.

## Resume Notes

- Keep the active plan scoped to the built-in assembler only.
- Do not activate `ideas/open/24_backend_builtin_linker_x86_plan.md` until this assembler runbook is complete.
- Step 1 inventory is recorded in `ideas/open/23_backend_builtin_assembler_x86_plan.md`.
- The current x86 runtime surface splits into 13 pure-assembly cases and 19 LLVM-fallback cases; do not broaden Step 2 to absorb fallback cases.
- The first object-emission slice is the relocation-free `return_zero` / `return_add` shape only.
- Step 2 now exposes `src/backend/x86/assembler/parser.hpp` and compiles `parser.cpp` into both the main binary and backend test target.
- The Step 2 parser deliberately accepts only `.intel_syntax noprefix`, `.text`, one `.globl`, one matching function label, `mov eax, imm32`, and `ret`.
- Step 3 now exposes `src/backend/x86/assembler/encoder/mod.hpp` with a bounded encoder surface that accepts only instruction statements from the parser's single-function slice.
- The first Step 3 encoding contract is byte-exact `mov eax, imm32` as opcode `0xB8` plus little-endian `imm32`, followed by `ret` as `0xC3`.
- Step 4 now wires `src/backend/x86/assembler/elf_writer.cpp` into both build targets and emits a minimal ELF64 relocatable object with `.text`, `.symtab`, `.strtab`, and `.shstrtab` for the bounded single-function slice.
- Step 4 validation now includes an x86 external-baseline test that compares `.text` size, relocation absence, symbol-table surface, and disassembly against `clang --target=x86_64-unknown-linux-gnu -c`.
- Regression guard status for this slice: `before passed=2320 failed=19`, `after passed=2320 failed=19`, `new failing tests=0`.
