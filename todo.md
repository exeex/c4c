Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Encode EV 64-Bit Fields

# Current Packet

## Just Finished

Step 3 encoded the classified positional RV64 EV `.insn.d` shape into
target-owned 64-bit instruction bytes and wired the prepared inline-asm object
path to emit them.

- Added `encode_rv64_ev_insn_d_inline_asm` with field-width validation:
  7-bit EV namespace opcode, 8-bit EV operation, 16-bit dtype/policy, and
  5-bit allocated register fields for destination/lhs/rhs/accumulator.
- Added little-endian 64-bit byte append support for the object fragment path.
- The first supported layout keeps minor `funct3`, `funct2`, and high
  `opcode8` fields fixed at zero while encoding grouped vector operands by
  their allocated base register.
- Added object-emission tests proving `.insn.d %4, %5, %0, %1, %2, %3, %6`
  emits `0x0000030b10620a0a` as eight little-endian text bytes and rejects
  out-of-range immediate fields.

## Suggested Next

Delegate Step 4 to add any desired mixed `.insn`/`.insn.d` prepared object
coverage while preserving existing scalar `.insn` and vector substitution
behavior.

## Watchouts

- The selected Step 3 layout maps the first immediate (`EV64`) to low
  `opcode7`, the second immediate (`EVADD`) to bits 39:32, and dtype/policy to
  bits 55:40; `funct3`, `funct2`, and high `opcode8` remain fixed zero for the
  first shape.
- Named operands, `%c[...]`, masks, consteval strings, relocations, and broad
  EV operation semantics remain rejected or out of scope.
- The proof still shows the known unrelated baseline failure
  `backend_codegen_route_riscv64_pointer_typed_select_publication`.

## Proof

Ran exactly:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded and the backend subset ran. `test_after.log` contains
one failing test, the known unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`; the new
`backend_riscv_object_emission` `.insn.d` coverage passed.
