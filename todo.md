Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve Side Effects And Close Diagnostics

# Current Packet

## Just Finished

Completed Plan Step 5 - Preserve Side Effects And Close Diagnostics.

Structured scalar RV64 `.insn r` side-effect metadata is now explicitly covered
at both HIR and LIR: the volatile `.insn r` fixture asserts that ordinary inline
asm side-effect bits survive alongside the structured metadata.

The RV64 object-emission tests now also prove that the structured metadata path
keeps the same closed diagnostic surface for named operands, template modifiers,
and clobbers as the text fallback path. Existing coverage continues to prove
unsupported `.insn` variants, malformed operand metadata, raw register tokens,
unsupported constraints, vector homes, incomplete carriers, ordinary inline asm
behavior, x86_64 route behavior, adjacent string templates, runtime-template
diagnostics, and `asm volatile` semantics.

## Suggested Next

The plan steps are now exhausted for this child. Ask the plan owner to decide
whether `ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
is ready to close or should spawn follow-up child ideas.

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Structured `.insn r` object emission still requires complete prepared
  carriers; do not bypass prealloc when tightening Step 5 diagnostics.
- Current `.insn r` object path rejects clobbers, named operands, template
  modifiers, raw register names, vector homes, and incomplete carriers. Preserve
  those rejection contracts.
- The text parser remains only for compatibility fallback when no structured
  metadata exists; new scalar `.insn r` progress should prefer metadata.
- Potential follow-up child ideas, if needed: vector constraints, EV `.insn.d`,
  consteval/template strings, and broader GNU assembler compatibility.

## Proof

Command:

```sh
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|backend_lir_to_bir_notes|backend_prealloc_inline_asm|backend_prepared_printer|backend_riscv_object_emission|backend_codegen_route_x86_64_inline_asm_.*|cpp_parse_gnu_asm_adjacent_string_template_dump|cpp_negative_tests_bad_inline_asm_runtime_template|positive_sema_linux_stage2_repro_03_asm_volatile_c)$" >> test_after.log 2>&1'
```

Result: passed, `100% tests passed, 0 tests failed out of 17`.
Log: `test_after.log`

Supervisor acceptance checks:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|backend_riscv_object_emission)$'
```

Result: passed. Regression guard reported no passed/failed/total delta for the
17-test broader proof logs; direct frontend HIR and RV64 object-emission checks
passed 2/2.
