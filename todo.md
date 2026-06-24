Status: Active
Source Idea Path: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Inline Asm And Object Surfaces

# Current Packet

## Just Finished

Completed Plan Step 1 - Map Existing Inline Asm And Object Surfaces.

Current frontend surface:
- Parser/HIR lowering reaches `NK_ASM` in `src/frontend/hir/impl/stmt/stmt.cpp`
  and builds `hir::InlineAsmStmt` from decoded asm text, output/input exprs,
  comma-joined constraints, clobbers, and `has_side_effects=true`.
- `rewrite_gcc_asm_template` rewrites GCC placeholders into `${N}` form before
  HIR storage. No `.insn r` structure is currently preserved in
  `hir::InlineAsmStmt`; the template remains string metadata.
- Existing HIR coverage in `tests/frontend/frontend_hir_tests.cpp` proves
  inline asm string literal folding and `.insn.d` folded-template preservation,
  but there is no scalar `.insn r` structured frontend fixture yet.

Current LIR/BIR surface:
- `src/codegen/lir/hir_to_lir/stmt.cpp` lowers `InlineAsmStmt` to
  `lir::LirInlineAsmOp`, rewriting constraints, splitting output/input
  operands, rendering `=r`, `0`, `r`, memory, immediate, clobber tokens, and
  preserving asm text as escaped string data.
- `src/backend/bir/lir_to_bir/calling.cpp` lowers `LirInlineAsmOp` to
  `bir::CallInst` with `InlineAsmMetadata`; it classifies `r`, `=r`, decimal
  tied inputs, `i`/`I`, `m`, `p`, clobbers, named-operand references, template
  modifiers, and unsupported facts. It does not classify `.insn r` shape.
- BIR coverage exists in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
  for inline asm metadata, tied operands, immediates, clobbers, modifiers, RV64
  vector-looking constraints, and VRM carrier metadata.

Current prealloc/regalloc surface:
- `src/backend/prealloc/inline_asm.cpp` builds `PreparedInlineAsmCarrier`
  records from BIR metadata and value homes. It publishes complete/incomplete
  carriers, RV64 GPR/vector target register identity, scalar `r` acceptance,
  tied output checks, immediate values, missing facts, and register-group
  overrides.
- RV64 scalar `.insn r` operands should reuse complete carriers with
  `InlineAsmRegisterClass::General`, `PreparedRegisterBank::Gpr`, and width 1;
  vector, named operand, template modifier, clobber, and unsupported operand
  facts should stay rejecting for this child.

Current RV64 object surface:
- `src/backend/mir/riscv/codegen/object_emission.cpp` already has late
  string-based `.insn r` support: `fragment_for_rv64_insn_r_inline_asm`
  accepts complete inline asm carriers, parses `.insn r opcode, funct3, funct7,
  rd, rs1, rs2`, resolves `%N` fields through prepared GPR homes, encodes with
  `encode_r_type`, and emits little-endian 32-bit bytes through
  `build_rv64_prepared_text_object_module`.
- The same file has `substitute_prepared_riscv_inline_asm_operands` for
  textual operand substitution and a separate EV `.insn.d` classifier/encoder;
  `.insn.d` and vector constraints remain out of scope for this child.
- Smallest byte-proof fixture is
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`:
  `builds_prepared_inline_asm_insn_r_object` expects `.insn r 0x33, 0, 0, %0,
  %1, %2` to emit `0x007302b3`, then move result to `a0`, then `ret`;
  negative fixtures cover incomplete carrier, non-`.insn r`, field count,
  numeric range, raw register tokens, named operands, template modifiers,
  clobbers, unsupported constraints, and vector homes.

Selected Step 2 route:
- Add a small structured `.insn r` recognizer at the frontend/HIR boundary,
  preferably as optional metadata on `hir::InlineAsmStmt`, for exactly
  `.insn r opcode, funct3, funct7, rd, rs1, rs2` after GCC placeholder rewrite.
- Carry that structure through `LirInlineAsmOp` and BIR inline asm metadata
  without changing register allocation yet. Keep the existing backend object
  parser as the byte-emission consumer for now, but make malformed
  supported-shape attempts reject earlier with source-level diagnostics.
- Step 2 should add positive and negative frontend/lowering tests before
  changing operand binding or byte emission.

## Suggested Next

Execute Step 2: parse and represent the supported scalar `.insn r` form by
adding structured metadata for the exact six-field shape, preserving ordinary
non-`.insn` inline asm behavior, and adding focused frontend/HIR/LIR/BIR tests
that prove recognition plus malformed-shape diagnostics.

Focused validation command for the next code packet:

```sh
bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|frontend_lir_|backend_lir_to_bir_notes|backend_riscv_object_emission)$" >> test_after.log 2>&1'
```

## Watchouts

- Keep this child limited to standard scalar RV64 `.insn`; vector constraints,
  EV `.insn.d`, and consteval/template asm strings belong to later child ideas.
- Do not prove progress by matching the umbrella's exact sample string.
- Do not rely on an external assembler as the primary object-byte proof.
- Existing object-side `.insn r` support is late and string-based; Step 2
  should not introduce a parallel byte encoder or bypass the prepared carrier
  route.
- Current `.insn r` object path rejects clobbers, named operands, template
  modifiers, raw register names, vector homes, and incomplete carriers. Preserve
  those rejection contracts.
- `src/codegen/lir/hir_to_lir/stmt.cpp` is marked as a draft-only split file,
  but it is currently part of the live inline asm lowering surface in this
  build. Verify wiring before editing.

## Proof

Command:

```sh
bash -lc 'cmake --build --preset default > test_after.log 2>&1'
```

Result: passed.
Log: `test_after.log`
