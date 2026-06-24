Status: Active
Source Idea Path: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement RV64 `.insn` Substitution And Encoding

# Current Packet

## Just Finished

Step 3 added the first target-owned RV64 standard `.insn r` object-byte path
from complete prepared inline-asm carriers. `object_emission.cpp` now recognizes
numeric `.insn r opcode, funct3, funct7, rd, rs1, rs2` templates on
`llvm.inline_asm`, resolves positional `%N` operands through complete prepared
carrier register homes, accepts the Stage 1 `%N` positional placeholder
spelling directly in `asm_text`, encodes the R-type word directly into the RV64
object fragment, and fails closed for missing carriers, unsupported templates,
named operands, template modifiers, clobbers, non-GPR homes, or malformed
numeric fields. The focused RV64 object test now proves both plain `=r,r,r` and
normalized tied `=r,0,r` carrier forms emit expected 32-bit instruction bytes
using `%0`, `%1`, and `%2` placeholders.

## Suggested Next

Delegate the next hardening packet for Step 3 follow-up or Step 4, limited to
additional standard scalar `.insn` forms or integration through the prepared
route selected by the supervisor. Keep vector constraints, EV `.insn.d`, named
operands, consteval asm strings, and external-assembler proof out of that
packet unless explicitly routed.

## Watchouts

- Keep the active route limited to standard RV64 `.insn` scalar constraints.
- Do not absorb vector constraints, EV `.insn.d`, named operands, or consteval
  asm string work into this child.
- Treat external assembler proof, testcase-shaped string matching, or
  expectation downgrades as route drift.
- The generic BIR classifier still normalizes scalar read-write `+r` into
  output `=r` plus tied input `0`; Step 3 should consume that prepared form.
- RV64 prealloc now proves scalar GPR identity only. FPR, vector, named operand,
  and EV constraints remain intentionally unsupported for this child route.
- RV64 object emission now treats inline asm as an in-place instruction
  fragment rather than a real call for call-frame setup; ordinary non-asm calls
  still drive the existing call-frame path.
- The `.insn r` parser is intentionally narrow: numeric opcode/funct fields and
  positional `%N` register operands only. `$N` remains accepted as a secondary
  internal alias, but `%N` is the Stage 1 user-surface route. Broader `.insn`
  shapes should be separate semantic encoders, not string shortcuts.

## Proof

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > test_after.log 2>&1`

Ran the exact delegated proof command. It exited nonzero with the known baseline
failure still present and unchanged: 315/316 backend tests passed, and the only
failing test was `backend_codegen_route_riscv64_pointer_typed_select_publication`.
Focused preflight also passed:
`cmake --build --preset default --target backend_riscv_object_emission_test &&
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`.
Proof log path: `test_after.log`.
