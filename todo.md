Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation And Close Readiness

# Current Packet

## Just Finished

Step 5 completed close-readiness validation for the RV64 call-adjacent scalar
and inline-asm plan. Fresh representative object-route probes were captured
under `build/agent_state/428_step5_probe/` for `pr40657`, `pr45695`,
`pr56982`, `pr38533`, and `pr49279`.

Completed work:

- `pr38533` still exits object compile with
  `unsupported_instruction_fragment`; the prepared carrier facts show repeated
  `missing_operand0_home` for tied `=r,0` inline-asm calls. This is an in-scope
  scalar inline-asm row only after a producer/carrier packet supplies operand
  homes; it is not currently coherent RV64 object-lowering work.
- `pr45695` still exits object compile with
  `unsupported_instruction_fragment`; prepared facts show
  `tied_input_output_home_mismatch`. This requires prepared inline-asm tied
  operand/home repair before object lowering can consume it.
- `pr49279` still exits object compile with
  `unsupported_instruction_fragment`; prepared facts also show
  `tied_input_output_home_mismatch`. This is the same producer/carrier
  follow-up class as `pr45695`.
- `pr40657` still exits object compile with
  `unsupported_instruction_fragment`; prepared facts show
  `unsupported_constraint0:=*m` and `unsupported_operand_constraint0`. This is
  memory-constraint inline-asm work, outside the scalar GPR input/output
  lowering packets completed by this plan.
- `pr56982` still exits object compile with
  `unsupported_instruction_fragment`; prepared facts show a clobber-only
  `~{memory}` carrier with `result_home=no`, no scalar operands, and no scalar
  publication work. This is out of scope for scalar call-result, call-argument,
  or scalar inline-asm input/output materialization.
- Existing focused backend tests cover the completed coherent scalar packets:
  prepared GPR call-result publication, frame-slot scalar value to GPR
  call-argument publication, and complete-carrier `.insn r` / `.insn d`
  inline-asm materialization. The representative full-row failures that remain
  are not expectation/accounting progress gaps and were not modified.
- Close-readiness result: the implemented scalar object-lowering work is ready
  for supervisor review, but the source idea is not ready for lifecycle close
  if the remaining representative rows must pass. The required follow-up is a
  prepared inline-asm producer/carrier idea for `missing_operand0_home` and
  `tied_input_output_home_mismatch`, with separate handling for memory
  constraints/clobber-only carriers if those rows are still desired.

## Suggested Next

Supervisor close-readiness review: accept the completed scalar object-lowering
packets as validated, then route the remaining representative rows to a
follow-up prepared inline-asm producer/carrier idea before further RV64 object
lowering.

## Watchouts

- Step 5 did not change implementation, expectations, allowlists, unsupported
  markers, runtime comparison, or pass/fail accounting.
- The fresh object-route probes still report generic
  `unsupported_instruction_fragment`; the useful classification comes from the
  paired prepared-carrier facts in `build/agent_state/428_step5_probe/`.
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
