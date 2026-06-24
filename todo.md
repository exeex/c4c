Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define And Test The Positional `.insn.d` Shape

# Current Packet

## Just Finished

Step 2 defined and tested the first target-owned positional RV64 EV `.insn.d`
shape without enabling object byte emission.

- Added `RiscvInsnDInlineAsmShape`/register classifier records and
  `classify_prepared_rv64_insn_d_inline_asm` in
  `src/backend/mir/riscv/codegen/object_emission.*`.
- The accepted shape is positional and structured:
  `.insn.d %imm_major, %imm_op, %dst, %lhs, %rhs, %acc, %imm_dtype`, currently
  represented by seven GCC-style numeric placeholders such as
  `.insn.d %4, %5, %0, %1, %2, %3, %6`.
- Immediate fields are accepted only from prepared `i` operands with
  `PreparedInlineAsmOperand::immediate_value`; raw literal fields and missing
  immediate values are rejected.
- Register fields are accepted only from prepared RV64 target register
  identities and preserve scalar/vector bank, physical index, and group width.
- Named operands, `%c` template modifiers, malformed field counts, and
  unsupported operand kinds reject before encoding.
- Existing `.insn r` object emission and Stage 2 vector substitution tests were
  kept intact. `rejects_prepared_inline_asm_insn_d_object` remains fail-closed
  because Step 3 still owns 64-bit byte emission.

## Suggested Next

Delegate Step 3 to encode the classified `.insn.d` shape into deterministic
64-bit EV instruction bytes, add field-width/range validation, and wire the
encoded fragment into prepared RV64 object emission.

## Watchouts

- Step 3 owns byte emission and field-range checks; Step 2 intentionally does
  not claim any `.insn.d` object bytes.
- Keep `rejects_prepared_inline_asm_insn_d_object` fail-closed until Step 3
  adds `append_le64`/equivalent and expected-byte coverage.
- The classifier is shape-based by placeholder positions and prepared operand
  facts; avoid replacing it with exact sample-string matching.
- Continue rejecting named operands, `%c[...]`, masks, consteval strings,
  relocations, and broad EV semantics unless the supervisor opens a separate
  packet.

## Proof

Ran exactly:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded and backend subset ran. The only failing test in
`test_after.log` is the supervisor-accepted unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`; the new
`backend_riscv_object_emission` coverage passed.
