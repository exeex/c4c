Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Parse And Represent Supported .insn r

# Current Packet

## Just Finished

Completed Plan Step 2 - Parse And Represent Supported `.insn r`.

Added structured optional `.insn r` metadata for the exact scalar RV64 shape
`.insn r opcode, funct3, funct7, rd, rs1, rs2` after GCC placeholder rewrite:
HIR `InlineAsmStmt`, LIR `LirInlineAsmOp`, and BIR `InlineAsmMetadata` now carry
opcode/funct3/funct7 plus rd/rs1/rs2 operand indices. HIR lowering diagnoses
malformed `.insn r` attempts early for field count, numeric range, and
non-positional register fields while leaving ordinary non-`.insn` inline asm
and other `.insn` forms as text-only templates.

Added dump visibility and a LIR verifier guard for `.insn r` metadata operand
indices. Focused tests now prove HIR recognition and diagnostics, HIR-to-LIR
metadata preservation, BIR metadata preservation, ordinary inline asm
non-recognition, and unchanged backend RV64 object-emission behavior.

## Suggested Next

Execute Step 3: bind scalar `r`, `=r`, `+r`, and supported tied operands for
the structured `.insn r` route to the existing GPR carrier path, reject
incompatible constraints at source/BIR metadata boundaries, and add focused
fixtures for untied, tied, read-write, and invalid scalar operands.

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Do not prove progress by matching the umbrella's exact sample string.
- Do not rely on an external assembler as the primary object-byte proof.
- Existing object-side `.insn r` support is still late and string-based; Step 3
  should reuse prepared carriers and must not introduce a parallel byte encoder
  or object-route bypass.
- Current `.insn r` object path rejects clobbers, named operands, template
  modifiers, raw register names, vector homes, and incomplete carriers. Preserve
  those rejection contracts.
- The delegated proof regex as written executes `frontend_hir_tests`,
  `backend_lir_to_bir_notes`, and `backend_riscv_object_emission`; the
  `frontend_lir_` alternative is exact-name anchored and does not match longer
  frontend LIR test names.

## Proof

Command:

```sh
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|frontend_lir_|backend_lir_to_bir_notes|backend_riscv_object_emission)$" >> test_after.log 2>&1'
```

Result: passed, `100% tests passed, 0 tests failed out of 3`.
Log: `test_after.log`

Supervisor acceptance checks:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^frontend_lir_'
```

Result: passed. Regression guard reported no passed/failed/total delta for the
canonical three-test proof logs; explicit frontend LIR subset passed 4/4.
