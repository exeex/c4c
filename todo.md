Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Helper-Style .insn.d Integration

# Current Packet

## Just Finished

Step 5 proved helper-style RV64 `.insn.d` integration across the available
folded-template and object-emission surfaces: frontend HIR now checks that a
helper-style string-literal `+` `.insn.d` template folds to the same final
template text and metadata as the literal form, and the RV64 object unit now
checks that a helper-built `.insn.d` template emits the same expected EV bytes
as the literal route.

## Suggested Next

Delegate Step 6 broad proof and closure preparation: run the supervisor-selected
broader validation, check that the completed inline-asm child stages were not
weakened, and summarize the accepted string surface plus known leftovers for
plan-owner close review.

## Watchouts

- The accepted helper-style surface remains narrow: string literal leaves joined
  by binary `+` before `NK_ASM`; this still does not accept runtime strings,
  general `consteval` string returns, or fixed-string objects.
- A direct public source-to-RV64-object test with vector typed operands is still
  outside current semantic-BIR admission: vector signatures/locals fail before
  inline asm object emission, while scalar operands reach BIR but do not provide
  compatible vector register homes for `VRM2`.
- No EV mnemonic, intrinsic, named-case shortcut, runtime string acceptance, or
  expectation downgrade was added.

## Proof

Ran exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^frontend_hir_tests$|^frontend_cxx_|cpp_parse_gnu_asm_adjacent_string_template_dump|cpp_negative_tests_bad_inline_asm_runtime_template|backend_riscv_object_emission|backend_codegen_route_x86_64_inline_asm_|inline_asm_aarch64_simple)') > test_after.log 2>&1`

Result: passed, 15/15 selected tests green. Proof log: `test_after.log`.
