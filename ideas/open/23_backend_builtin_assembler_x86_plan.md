# Built-in x86 Assembler Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/15_backend_x86_port_plan.md`
- `ideas/closed/04_backend_binary_utils_contract_plan.md`

Should precede:

- `ideas/open/24_backend_builtin_linker_x86_plan.md`

## Goal

Implement the first built-in assembler slice for x86 so the compiler can emit working ELF objects without relying on an external assembler for the supported x86 backend subset.

## Why This Is Separate

- codegen bring-up and assembler object emission are distinct seams
- x86 instruction encoding coverage is broad enough that the first built-in assembler slice must stay tightly bounded
- the built-in assembler should follow one explicit backend-owned x86 assembly subset instead of driving codegen scope

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/x86/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/x86/assembler/mod.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/parser.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/elf_writer.rs`
- `ref/claudes-c-compiler/src/backend/x86/assembler/encoder/`

## Scope

- make the x86 assembler subtree under `src/backend/x86/assembler/` compile together with the shared ELF support it depends on
- encode the minimum x86 instruction subset already exercised by current backend tests
- write ELF relocatable objects for that supported subset
- validate emitted objects against external assembler output for representative cases

## Explicit Non-Goals

- linker implementation
- full x86 instruction-set coverage
- assembler support beyond the already-supported x86 backend subset

## Suggested Execution Order

1. port only enough parser surface to cover the current x86 codegen output subset
2. port the smallest encoder slices matching that subset
3. port the object-writing layer around shared ELF helpers
4. compare resulting `.o` files against external assembly on sections, symbols, relocations, and disassembly
5. widen parser and encoder coverage only when new x86 backend slices require it

## Validation

- the x86 assembler subtree compiles and is wired to the chosen assembler entry boundary
- emitted x86 object files disassemble correctly for the bounded subset
- object metadata and relocations are comparable to externally assembled output for covered cases

## Good First Patch

Make parser, encoder, and ELF-writer pieces compile together, then route one minimal x86 backend-emitted function through that path.

## Step 1 Inventory

Captured from current `x86_64-unknown-linux-gnu` backend output on 2026-03-29.

### Current x86 Runtime Cases Already Emitting Pure Assembly

- `return_zero`
- `return_add`
- `local_temp`
- `local_array`
- `branch_if_lt`
- `branch_if_le`
- `branch_if_gt`
- `branch_if_ge`
- `extern_global_array`
- `string_literal_char`
- `global_char_pointer_diff`
- `global_int_pointer_diff`
- `global_int_pointer_roundtrip`

### Current x86 Runtime Cases Still Falling Back To LLVM IR Text

- `branch_if_eq`
- `branch_if_ne`
- `branch_if_ult`
- `branch_if_ule`
- `branch_if_ugt`
- `branch_if_uge`
- `call_helper`
- `param_slot`
- `two_arg_helper`
- `two_arg_local_arg`
- `two_arg_second_local_arg`
- `two_arg_second_local_rewrite`
- `two_arg_first_local_rewrite`
- `two_arg_both_local_arg`
- `two_arg_both_local_first_rewrite`
- `two_arg_both_local_second_rewrite`
- `two_arg_both_local_double_rewrite`
- `local_arg_call`
- `global_load`

### Current Pure-Assembly Surface

Observed directives and section forms:

- `.intel_syntax noprefix`
- `.text`
- `.globl <symbol>`
- `<symbol>:`
- local labels such as `.Lblock_1:` and `.Lblock_2:`
- `.section .rodata`
- `.data`
- `.bss`
- `.asciz "hi"`
- `.long <imm>`
- `.zero <size>`

Observed instruction forms:

- `mov eax, imm32`
- `mov eax, dword ptr [reg]`
- `mov dword ptr [reg], imm32`
- `movsx eax, byte ptr [reg + imm]`
- `movzx eax, al`
- `lea reg, symbol[rip]`
- `lea reg, [base + imm]`
- `add eax, dword ptr [reg + imm]`
- `sub reg, reg`
- `sar reg, imm`
- `cmp reg, imm`
- `jg`, `jge`, `jl`, `jle`
- `sete al`
- `push rbp`
- `pop rbp`
- `ret`

### External-Assembler Object Surface For The Smallest Slice

For `return_zero` and `return_add`, assembling the current backend text with
`clang --target=x86_64-unknown-linux-gnu -c` produces:

- ELF64 relocatable object format
- `.text`, `.symtab`, and `.strtab`
- one global function symbol: `main`
- no `.data`, `.bss`, `.rodata`, or relocation sections
- no relocations
- `.text` bytes matching `mov eax, imm32; ret`

### External-Assembler Relocation Surface In Supported Follow-On Cases

The first pure-assembly cases that need relocations all use one RIP-relative
address materialization shape:

- `lea rax, symbol[rip]`
- relocation section: `.rela.text`
- relocation type: `R_X86_64_PC32`

Observed symbols/sections behind that relocation:

- `.rodata` local label from `string_literal_char`
- global `.data` symbol `g_value`
- global `.bss` symbols `g_bytes` and `g_words`
- undefined external symbol `ext_arr`

### Step 1 Decision

Keep the first built-in assembler slice narrower than the full current pure
assembly surface:

- parser target: `.intel_syntax noprefix`, `.text`, `.globl`, one global label,
  `mov eax, imm32`, and `ret`
- encoder target: `mov r32, imm32` and `ret`
- object-writer target: ELF64 relocatable object with `.text`, `.symtab`,
  `.strtab`, and one global function symbol
- explicitly defer local labels, conditional branches, frame setup/teardown,
  data sections, string/data directives, RIP-relative relocations, and
  multi-function output until later slices
