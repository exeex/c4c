Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Accepted Template Expressions Before Inline Asm Lowering

# Current Packet

## Just Finished

Step 2 implemented the first asm-template folding gate in
`src/frontend/parser/impl/statements.cpp`.

Literal and adjacent-literal asm templates still fold through
`consume_adjacent_string_literal(parser)` into `NK_STR_LIT` and continue through
the existing `NK_ASM` construction path. Parsed non-literal template
expressions now stop before `NK_ASM` construction, mark the parser errored,
return `NK_INVALID_STMT`, and emit:

`unsupported inline asm template expression; expected string literal or adjacent string literals`

Test coverage added:

- `tests/cpp/internal/negative_case/bad_inline_asm_runtime_template.cpp`
  verifies a runtime `const char*` template is rejected instead of silently
  becoming `NK_EMPTY`.
- `tests/cpp/internal/negative_case/CMakeLists.txt` pins the diagnostic
  substring for that case.
- `tests/c/internal/cmake/run_negative_case.cmake` now optionally checks
  `EXPECT_ERROR_SUBSTRING`; existing negative cases are unchanged unless they
  opt in.

The accepted literal path was not otherwise changed, so constraints, operands,
clobbers, volatility, memory metadata, and `%0`/positional placeholder handling
remain on the existing path.

## Suggested Next

Delegate Step 3 to `c4c-executor`: add the first positive integration proof for
an RV64 `.insn.d` asm template assembled from accepted adjacent literal
fragments, and compare it against the literal `.insn.d` object route without
adding any runtime string support.

## Watchouts

- Runtime strings are intentionally rejected at the parser gate.
- The first accepted surface is still only literal/adjacent-literal template
  text. `const char*`, constexpr arrays, fixed-string operators, `std::array`,
  `std::string_view`, and consteval string helpers remain unsupported until a
  real string-valued folding representation exists.
- The Step 3 proof should not claim consteval/helper support yet; it should
  prove the currently accepted fragment-folding route and preserve the existing
  operand/constraint/clobber behavior.

## Proof

Proof command:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|frontend_cxx_.*|cpp_parse_gnu_asm_adjacent_string_template_dump|cpp_negative_tests_.*|backend_riscv_object_emission)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains 53/53 selected tests passing,
including `cpp_parse_gnu_asm_adjacent_string_template_dump`,
`cpp_negative_tests_bad_inline_asm_runtime_template`, `frontend_hir_tests`, and
`backend_riscv_object_emission`.
