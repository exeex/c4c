Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation And Close Readiness

# Current Packet

## Just Finished

Step 4 classified the prepared scalar inline-asm evidence. No coherent new
scalar inline-asm input/output lowering packet exists in the active
call-adjacent evidence because the representative rows do not provide complete
prepared inline-asm carrier facts for RV64 object lowering.

Completed work:

- Accepted as already covered by existing object lowering: complete prepared
  `.insn r` and `.insn d` inline-asm carriers with scalar GPR operands. The
  object route already requires complete carriers and existing backend tests
  cover scalar input/output substitution, tied/readwrite operands, and
  fail-closed cases for missing carriers, unsupported operand metadata, named
  operands, template modifiers, clobbers, vector homes, and malformed text.
- Rejected for this packet: `pr38533`-class rows carry repeated
  `missing_operand0_home` facts for tied `=r,0` inline-asm calls. RV64 lowering
  cannot materialize scalar input/output operands without the producer assigning
  the operand home.
- Rejected for this packet: `pr45695`-class and `pr49279`-class rows carry
  `tied_input_output_home_mismatch`. The packet constraints forbid repairing
  tied input/output mismatches inside RV64 lowering.
- Rejected for this packet: `pr40657`-class rows carry
  `unsupported_constraint0:=*m` / `unsupported_operand_constraint0`, which is a
  memory-constraint carrier problem, not coherent scalar GPR input/output
  materialization.
- Out of scope for scalar input/output lowering: `pr56982`-class rows provide
  a clobber-only `~{memory}` carrier with `result_home=no`, no scalar operands,
  and no scalar publication work for the object route.
- No implementation files were changed. Completing Step 4 as lowering work
  requires a separate prepared inline-asm producer/carrier repair packet for
  missing operand homes or tied input/output home mismatches before RV64 object
  emission can consume those rows.

## Suggested Next

Execute Step 5: broader validation and close-readiness review for the completed
call-adjacent scalar packets. If inline-asm rows remain required for the source
idea, select a separate producer/carrier packet for `missing_operand0_home` and
`tied_input_output_home_mismatch` before attempting more RV64 object lowering.

## Watchouts

- Step 4 did not add new code because the active evidence lacks complete new
  scalar inline-asm carrier facts.
- Do not treat complete `.insn r` / `.insn d` inline-asm carriers as new Step 4
  evidence; they are already supported by the existing object route and tests.
- Register-to-register pointer call results were pre-existing supported
  movement and should not be used as evidence for pointer/address publication
  progress.
- Do not repair inline-asm `missing_operand0_home` or
  `tied_input_output_home_mismatch` inside RV64 lowering; those are producer or
  carrier blockers.
- Keep frame-slot address arguments, symbol-address arguments, and
  prior-preservation argument routing out of this completed scalar argument
  packet.
- Keep aggregate `sret`/`byval`, F128 helper-call behavior, missing prepared
  metadata reconstruction, pointer/address publication, and
  expectation/accounting changes out of this plan.
- Do not key the route to representative filenames such as `src/pr38533.c`.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
