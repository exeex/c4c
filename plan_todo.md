# Built-in x86 Assembler Todo

Status: Active
Source Idea: ideas/open/23_backend_builtin_assembler_x86_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 3: Port the minimum encoder surface

## Planned Items

- [ ] Step 4: Port ELF object writing support
- [ ] Step 5: Wire one backend-emitted function through the built-in assembler path
- [ ] Step 6: Validate against external assembler output

## Completed Items

- [x] Activated `ideas/open/23_backend_builtin_assembler_x86_plan.md` into the active runbook
- [x] Step 1: Inventory current x86 backend output and choose the first bounded assembler slice
- [x] Step 2: Port the minimum parser surface

## Next Intended Slice

- Port the first encoder entry points for `mov eax, imm32` and `ret`.
- Keep the encoder surface aligned with the parser’s current single-function `.text`-only slice.
- Add focused byte-level tests before widening beyond the relocation-free return-immediate path.

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
- Regression guard status for this slice: `before passed=2320 failed=19`, `after passed=2320 failed=19`, `new failing tests=0`.
