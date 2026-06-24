Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove RV64 `.insn.d` Helper Integration

# Current Packet

## Just Finished

Step 3 added a positive RV64 `.insn.d` integration proof in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`.

The new object-emission test builds a prepared `.insn.d` carrier whose template
text is assembled from accepted adjacent literal fragments:

`.insn.d ` + `%4, %5, ` + `%0, %1, ` + `%2, %3, %6`

It then verifies that the carrier reaches the same prepared object route as the
literal `.insn.d` case: `.text`/`main` are published, the text size is 12 bytes,
the first eight bytes decode to `0x0000030b10620a0a`, the following word is the
RV64 return instruction, and no relocations are emitted. No runtime string or
consteval helper support was added or claimed.

## Suggested Next

Delegate the next packet for Step 4: define the narrowest first consteval/helper
surface or request plan review if Step 4 needs to split string-valued folding
from backend object integration.

## Watchouts

- Runtime strings remain intentionally rejected at the parser gate.
- The current positive object proof covers only literal/adjacent-literal final
  template text flowing through the prepared `.insn.d` object route.
- `const char*`, constexpr arrays, fixed-string operators, `std::array`,
  `std::string_view`, and consteval string helpers remain unsupported until a
  real string-valued folding representation exists.

## Proof

Proof command:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|frontend_cxx_.*|cpp_parse_gnu_asm_adjacent_string_template_dump|cpp_negative_tests_.*|backend_riscv_object_emission)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains 53/53 selected tests passing,
including `cpp_parse_gnu_asm_adjacent_string_template_dump`,
`cpp_negative_tests_bad_inline_asm_runtime_template`, `frontend_hir_tests`, and
`backend_riscv_object_emission`.

Focused pre-check also passed with:

`cmake --build --preset default --target backend_riscv_object_emission_test && ./build/tests/backend/mir/backend_riscv_object_emission_test`
