Status: Active
Source Idea Path: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Harden Proof And Boundaries

# Current Packet

## Just Finished

Step 4 hardened the RV64 standard `.insn r` object proof and boundaries. The
focused object test now covers the existing positive byte proof for plain
`=r,r,r` and tied `=r,0,r` scalar carriers plus fail-closed coverage for
missing and extra fields, out-of-range numeric fields, raw register tokens,
named operands, template modifiers, clobbers, unsupported operand kinds,
unsupported constraint text, vector homes, incomplete carriers, non-`.insn r`
templates, and EV `.insn.d`. `object_emission.cpp` now also rejects contradictory
complete-carrier metadata whose operand kind is register-like but whose
constraint string is outside Stage 1 scalar `r`/`=r`/`+r`/tied-index forms.

## Suggested Next

No focused Step 4 implementation work remains from this packet. The runbook
appears complete from the executor side; next action should be supervisor-side
review or lifecycle routing rather than widening into vector constraints, EV
`.insn.d`, named operands, consteval asm strings, or external assembler proof.

## Watchouts

- The Stage 1 object path is intentionally narrow: numeric `.insn r` fields and
  positional `%N`/internal `$N` operands that resolve through complete scalar
  GPR prepared carriers only.
- Vector constraints, EV `.insn.d`, named operands, consteval asm strings, and
  external assembler proof remain separate child scopes, not Step 4 follow-up
  work.
- Do not weaken the new negative tests to accept non-scalar prepared metadata;
  later children should add distinct semantic encoders or carrier support.

## Proof

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > test_after.log 2>&1`

Ran the exact delegated proof command. It exited nonzero with the known baseline
failure still present and unchanged: 315/316 backend tests passed, and the only
failing test was `backend_codegen_route_riscv64_pointer_typed_select_publication`.
The hardened `backend_riscv_object_emission` test passed in the full subset.
Focused preflight also passed:
`cmake --build --preset default --target backend_riscv_object_emission_test &&
ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`.
Proof log path: `test_after.log`.
