Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Emit RV64 .insn Bytes Through Object Route

# Current Packet

## Just Finished

Completed Plan Step 4 - Emit RV64 `.insn` Bytes Through Object Route.

RV64 object emission now treats structured `InlineAsmInsnRMetadata` as the
primary authority for scalar `.insn r` opcode/funct fields and rd/rs1/rs2
operand indices when metadata is present, resolving only the prepared GPR
carrier homes at emission time. The legacy string parser remains as a
compatibility fallback for text-only inline asm.

Focused object tests now prove exact bytes for structured metadata emission
without text reparse, tied-input emission, direct `+r` read-write emission, and
malformed structured operand metadata rejection. Existing fail-closed coverage
for incomplete carriers, named operands, template modifiers, clobbers,
unsupported constraints, raw register tokens, vector homes, and text-only
malformed forms remains in place.

## Suggested Next

Execute Step 5: preserve and verify side-effect handling for the structured
scalar `.insn r` route, including `asm volatile`, memory/clobber diagnostics,
unsupported variants, and broader validation selected by the supervisor.

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Do not prove progress by matching the umbrella's exact sample string.
- Do not rely on an external assembler as the primary object-byte proof.
- Structured `.insn r` object emission still requires complete prepared
  carriers; do not bypass prealloc when tightening Step 5 diagnostics.
- Current `.insn r` object path rejects clobbers, named operands, template
  modifiers, raw register names, vector homes, and incomplete carriers. Preserve
  those rejection contracts.
- The text parser remains only for compatibility fallback when no structured
  metadata exists; new scalar `.insn r` progress should prefer metadata.

## Proof

Command:

```sh
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|backend_lir_to_bir_notes|backend_prealloc_inline_asm|backend_riscv_object_emission)$" >> test_after.log 2>&1'
```

Result: passed, `100% tests passed, 0 tests failed out of 4`.
Log: `test_after.log`

Supervisor acceptance checks:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed. Regression guard reported no passed/failed/total delta for the
canonical four-test proof logs; direct RV64 object-emission test passed 1/1.
