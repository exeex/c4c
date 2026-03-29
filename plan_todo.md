# Built-in x86 Assembler Todo

Status: Active
Source Idea: ideas/open/23_backend_builtin_assembler_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 4: Port ELF object writing support
- Iteration slice: wrap the Step 3 encoder byte stream in the first ELF64
  relocatable object path for the `.text`-only, one-symbol return-immediate
  slice.

## Planned Items

- [ ] Step 4: Port ELF object writing support
- [ ] Step 5: Wire one backend-emitted function through the built-in assembler path
- [ ] Step 6: Validate against external assembler output

## Completed Items

- [x] Activated `ideas/open/23_backend_builtin_assembler_x86_plan.md` into the active runbook
- [x] Step 1: Inventory current x86 backend output and choose the first bounded assembler slice
- [x] Step 2: Port the minimum parser surface
- [x] Step 3: Port the minimum encoder surface

## Next Intended Slice

- Emit an ELF64 relocatable object containing `.text`, `.symtab`, and `.strtab`
  for the single `main` symbol and the relocation-free encoder output.
- Compare the object surface for `return_zero` and `return_add` against an
  external `clang -c` baseline before widening the writer.
- Keep relocations, data sections, and multi-function output deferred until a
  later Step 4 slice.

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
- Regression guard status for this slice: `before passed=2319 failed=20`, `after passed=2320 failed=19`, `new failing tests=0`.
