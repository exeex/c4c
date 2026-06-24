Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement First Real Compile-Time String Folding Surface

# Current Packet

## Just Finished

Step 4 implemented the first real compile-time inline-asm template folding
surface: a parser expression tree made only of string literals joined by binary
`+` now folds to final string-literal text before the existing `NK_ASM` path,
HIR template rewrite, operand metadata, constraint handling, clobbers, and
runtime-string rejection consume it.

## Suggested Next

Delegate Step 5 to prove a helper-style `.insn.d` integration using the folded
template route from Step 4, without adding EV intrinsics or bypassing the
normal inline asm/object path.

## Watchouts

- The accepted Step 4 surface is narrow: parenthesized inline-asm template
  expressions composed exclusively of `NK_STR_LIT` leaves and binary `+`.
- Runtime/non-literal templates remain rejected by the same unsupported-template
  diagnostic; this is not helper-style `consteval` or fixed-string support yet.
- The fold deliberately happens before `NK_ASM` construction so literal,
  adjacent-literal, empty-asm, HIR metadata, and backend routes stay shared.

## Proof

Ran exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^frontend_hir_tests$|^frontend_cxx_|cpp_parse_gnu_asm_adjacent_string_template_dump|cpp_negative_tests_bad_inline_asm_runtime_template|backend_riscv_object_emission|backend_codegen_route_x86_64_inline_asm_|inline_asm_aarch64_simple)') > test_after.log 2>&1`

Result: passed, 15/15 selected tests green. Proof log: `test_after.log`.
