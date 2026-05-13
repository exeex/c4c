Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Scalar Inputs And Existing Record Owners

# Current Packet

## Just Finished

Lifecycle activation initialized this active plan from
`ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md`.

## Suggested Next

Execute Step 1 from `plan.md`: confirm scalar inputs, existing target record
owners, and the first concrete ALU/cast record slice.

## Watchouts

- Keep this slice record-only; do not add assembly emission, encoding, object
  output, memory lowering, calls, or returns.
- Use structured BIR and prepared facts as authority; do not parse rendered
  names or printed BIR.
- Keep target record ownership under `src/backend/mir/aarch64/codegen/`.

## Proof

No validation run; lifecycle-only activation.
