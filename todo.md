Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind Scalar Constraints And Substitute GPR Operands

# Current Packet

## Just Finished

Completed Plan Step 3 - Bind Scalar Constraints And Substitute GPR Operands.

BIR inline asm metadata now classifies scalar `+r` as a read-write register
operand with both input and output authority, and structured RV64 `.insn r`
metadata now publishes unsupported facts when any rd/rs1/rs2 field does not
bind to a scalar GPR-capable operand (`r`, `=r`, `+r`, or a supported tied
operand). This connects the Step 2 `.insn r` shape to the existing scalar
prepared-carrier path without adding a new allocator or object encoder route.

Focused tests now cover clean untied scalar `.insn r` metadata, direct `+r`
read-write metadata, invalid immediate constraints in register fields, prepared
carrier binding for structured `+r`, existing untied/tied GPR carriers, and the
unchanged RV64 object-emission route.

## Suggested Next

Execute Step 4: route the structured scalar `.insn r` fields into RV64 object
encoding byte proof, preferably by consuming `InlineAsmInsnRMetadata` plus
prepared GPR carrier homes instead of re-parsing the asm text as the primary
authority. Keep existing string parser behavior only as compatibility/fallback
where needed.

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Do not prove progress by matching the umbrella's exact sample string.
- Do not rely on an external assembler as the primary object-byte proof.
- Existing object-side `.insn r` support is still late and string-based; Step 4
  should make structured metadata the documented authority for byte emission
  without bypassing prepared carriers.
- Current `.insn r` object path rejects clobbers, named operands, template
  modifiers, raw register names, vector homes, and incomplete carriers. Preserve
  those rejection contracts.
- `backend_prealloc_inline_asm` is now a CTest alias for the prepared-printer
  carrier executable so the delegated Step 3 proof exercises the prealloc
  inline-asm carrier assertions directly.
- Some semantic-prealloc fixtures may spill result homes under pressure; avoid
  treating that as scalar constraint failure unless the prepared carrier itself
  is expected to be complete.

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
ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_prealloc_inline_asm)$'
```

Result: passed. Regression guard reported one additional passing test from the
new `backend_prealloc_inline_asm` alias and no new failures; prepared-printer
and prealloc alias checks passed 2/2.
