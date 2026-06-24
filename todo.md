# Current Packet

Status: Active
Source Idea Path: ideas/open/335_textual_assembler_object_route_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Textual Assembler Need And Seams

## Just Finished

- Activated idea 335 as the next focused child after closing object-route scan
  and default-readiness idea 334.

## Suggested Next

- Execute Step 1 by inspecting current direct object/asm route coexistence and
  deciding whether a scoped textual assembler route is needed now or whether
  this child should close with a no-work-needed decision.

## Watchouts

- Do not make direct `--codegen obj` depend on textual assembly parsing.
- Do not build broad GNU assembler compatibility.
- Preserve existing asm-route and direct-object tests.
- Keep any implementation scoped to c4c-emitted assembly syntax and existing
  target encoder/object model integration.

## Proof

- Lifecycle activation only; no build required.
