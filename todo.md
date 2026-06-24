Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map `.insn.d` Extension Surfaces

# Current Packet

## Just Finished

Step 1 mapped the Stage 1/Stage 2 inline asm surfaces that Stage 3 `.insn.d`
must extend.

- Frontend/LIR source flow:
  `src/frontend/parser/impl/statements.cpp` fills inline asm template,
  constraint, output, input, and clobber AST fields; `src/frontend/hir/impl/stmt/stmt.cpp`
  preserves them in HIR; `src/codegen/lir/hir_to_lir/stmt.cpp` builds
  `lir::LirInlineAsmOp`; `src/codegen/lir/ir.hpp` owns the LIR carrier fields
  `asm_text`, `constraints`, `side_effects`, `args_str`, and `clobbers`.
- BIR classification/carrier:
  `src/backend/bir/lir_to_bir/calling.cpp` owns constraint splitting and
  classification via `split_inline_asm_constraint_tokens`,
  `classify_inline_asm_vector_constraint`, `inline_asm_template_modifier_facts`,
  `make_inline_asm_metadata`, and `lower_inline_asm_call`; BIR metadata lives in
  `src/backend/bir/bir.hpp` as `InlineAsmMetadata` and
  `InlineAsmOperandMetadata`.
- Immediate carrier:
  `make_inline_asm_metadata` classifies `i` and `I` as
  `InlineAsmOperandKind::IntegerImmediateInput`; `src/backend/prealloc/inline_asm.cpp`
  then computes `PreparedInlineAsmOperand::immediate_value` through
  `inline_asm_integer_immediate_value`, `make_prepared_inline_asm_operand`, and
  validates it in `validate_inline_asm_carrier`.
- Register substitution:
  Stage 2 vector constraints are classified in `classify_inline_asm_vector_constraint`
  for `VR`, `VRM2`, and `VRM4`; `src/backend/prealloc/inline_asm.cpp`
  publishes group-width overrides through
  `populate_inline_asm_register_group_overrides`,
  `publish_inline_asm_metadata_register_group_overrides`, and
  `append_inline_asm_register_group_override`, attaches RV64 GPR/vector target
  identities through `rv64_inline_asm_register_identity` and
  `populate_inline_asm_home_identity`, and validates class compatibility in
  `validate_inline_asm_carrier`. RV64 text substitution is in
  `src/backend/mir/riscv/codegen/object_emission.cpp` via
  `substitution_home_for_operand`, `register_name_for_inline_asm_substitution_home`,
  `substitute_positional_riscv_inline_asm_operands`, and
  `substitute_prepared_riscv_inline_asm_operands`.
- `.insn` classification and object emission:
  `src/backend/mir/riscv/codegen/object_emission.cpp` owns the Stage 1 object
  path through `parse_rv64_insn_u32`, `rv64_register_number_for_inline_asm_operand`,
  `fragment_for_rv64_insn_r_inline_asm`, `fragment_for_prepared_call`,
  `prepared_function_to_object_function`, `build_rv64_prepared_text_object_module`,
  and `write_rv64_prepared_relocatable_elf_object`; public declarations are in
  `src/backend/mir/riscv/codegen/object_emission.hpp`.
- 64-bit object byte surface:
  the existing object fragment carrier `RiscvEncodedFragment` stores arbitrary
  byte vectors, and `.insn r` currently appends 32-bit little-endian bytes with
  `append_le32`. Stage 3 likely needs a local `append_le64`/equivalent and a
  new `.insn.d` fragment builder that appends eight little-endian bytes without
  relocations.
- Stage 3 carrier gaps:
  no frontend/HIR contract rewrite appears required for positional `.insn.d`
  when immediate fields are supplied through existing `i` operands. The backend
  gap is in RV64 object emission: `fragment_for_rv64_insn_r_inline_asm` parses
  only `.insn r`, accepts only literal numeric opcode/funct fields plus GPR
  placeholders, rejects `IntegerImmediateInput`, rejects vector homes, and the
  current `rv64_register_number_for_inline_asm_operand` cannot return vector
  physical indexes. `.insn.d` needs a separate parser/extractor that can read
  immediate operands from `PreparedInlineAsmOperand::immediate_value` and read
  scalar/vector physical indexes from prepared homes, including grouped vector
  base registers.
- Likely focused tests:
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` owns Stage 1
  `.insn r` object byte tests, vector substitution tests, and negative
  `.insn.d` rejection coverage; this is the primary Stage 3 proof surface.
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` covers vector
  constraint classification, `tests/backend/bir/backend_prepared_printer_test.cpp`
  covers RV64 scalar/vector inline asm carriers and dump visibility, and
  `tests/backend/bir/backend_prepare_liveness_test.cpp` covers vector group
  allocation/override behavior.

## Suggested Next

Delegate Step 2 to implement the target-owned positional `.insn.d` parser in
`src/backend/mir/riscv/codegen/object_emission.cpp`, starting beside
`fragment_for_rv64_insn_r_inline_asm`, with focused positive/negative coverage
in `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

## Watchouts

- Do not weaken `rejects_prepared_inline_asm_insn_d_object`; convert it only
  when Stage 3 owns structured `.insn.d` parsing and byte emission.
- Keep named operands, `%c[...]` modifiers, clobbers, memory/address operands,
  relocations, consteval strings, masks, and EV semantic operations out of this
  child.
- Reuse the existing prepared inline asm carrier instead of adding a parallel
  frontend path. Immediate values should come from
  `PreparedInlineAsmOperand::immediate_value`; scalar/vector registers should
  come from prepared homes/target identities.
- Avoid extending the Stage 1 GPR-only helper in a way that silently accepts
  vector homes for `.insn r`; Stage 3 should add a distinct `.insn.d` path.

## Proof

Discovery-only packet. No build, CTest, or proof log was required or run.
