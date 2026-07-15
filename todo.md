# Current Packet

Status: Active
Source Idea Path: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one native body-parameter use row

## Just Finished

- 734 Step 7.37 is accepted in `88e930ee1`; its source completion gate was
  reassessed and rejected because no next body-parameter receiver row is yet
  authorized.

## Suggested Next

- Execute Step 1: trace and select exactly one native body-parameter use row.

## Watchouts

- Do not modify Raw-BIR/importer code or select semantic authority from text,
  names, signatures, rendered operands, printer output, or testcase shape.

## Proof

- Step 1 is selection-only; define its focused producer proof only after a
  native structured row is selected.
