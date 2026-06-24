Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Stage 3 And Record Remaining Children

# Current Packet

## Just Finished

Step 5 validated RV64 EV `.insn.d` Stage 3 coverage against the requested
checklist and recorded remaining child scope without changing `plan.md` or the
source idea.

- Malformed syntax coverage is present for missing and extra `.insn.d`
  positional fields.
- Unsupported operand coverage is present for register slots backed by an
  unsupported operand kind and for register operands used in immediate slots.
- Non-constant or missing immediate coverage is present through rejection of
  literal immediate fields and prepared `i` operands without an
  `immediate_value`.
- Invalid width/range coverage is present for namespace, operation, and dtype
  overflow, including object-path rejection for an invalid dtype.
- Relocation-like/no-relocation behavior is covered by the accepted `.insn.d`
  object path and mixed `.insn r` plus `.insn.d` object path both asserting no
  relocations are emitted; relocation-bearing operands remain outside the
  supported Stage 3 shape.
- Named operands and `%c` template modifiers are explicitly rejected for
  `.insn.d`.
- Mixed `.insn`/`.insn.d` coverage is present and verifies scalar `.insn r`
  bytes, EV `.insn.d` bytes `0x0000030b10620a0a`, return move, and `ret` in
  order.

## Suggested Next

Supervisor should review and commit the Stage 3 slice if the known unrelated
backend baseline failure remains acceptable.

## Watchouts

- No additional implementation or test edits were needed for Step 5 validation.
- Future child work, if desired, should be scoped separately for consteval or
  template-built asm strings, GNU named operands, `%c[...]` modifiers, masks,
  relocation-bearing operands, or broader EV operation semantics.
- The backend proof still shows the known unrelated baseline failure
  `backend_codegen_route_riscv64_pointer_typed_select_publication`.

## Proof

Ran exactly:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded and the backend subset ran. `test_after.log` contains
one failing test, the known unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`;
`backend_riscv_object_emission` passed, including the Stage 3 `.insn.d`
classifier, encoder, object-byte, negative, no-relocation, and mixed
`.insn r` plus `.insn.d` coverage.
