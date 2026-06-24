Status: Active
Source Idea Path: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve And Classify Stage 1 Constraints

# Current Packet

## Just Finished

Step 2 added the RV64 scalar prealloc prerequisite for Stage 1 inline asm:
`src/backend/prealloc/inline_asm.cpp` now maps RV64 ABI GPR spellings and
`x0`-`x31` names to concrete `PreparedTargetRegisterIdentity` values, allowing
`r`, `=r`, and numeric tied operands to validate as RV64 general-register
carriers. `tests/backend/bir/backend_prepared_printer_test.cpp` now has a
focused RV64 carrier fixture proving complete `r`, `=r`, and tied `"0"`
metadata, including tied shared-home authority, without adding `.insn` object
encoding.

## Suggested Next

Delegate Step 3 to implement the first target-owned RV64 `.insn r` substitution
and object-byte fragment path from complete prepared inline-asm carriers. Keep
the packet limited to standard scalar `.insn r` parsing, positional operands,
RV64 GPR register-number extraction, and byte-level object proof.

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
- RV64 object emission currently rejects unsupported call shapes by returning
  `nullopt`; adding `.insn` support likely means handling complete
  `call.inline_asm` carriers before ordinary call-fragment lowering.

## Proof

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > test_after.log 2>&1`

Ran the exact delegated proof command. It exited nonzero with the known baseline
failure still present and unchanged: 315/316 backend tests passed, and the only
failing test was `backend_codegen_route_riscv64_pointer_typed_select_publication`.
Proof log path: `test_after.log`.
