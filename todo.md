# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed a step 4 backend-front-door classification packet by making
`src/backend/backend.cpp` stage x86 LIR assembly text through the reviewed
`api::emit_module(...)` seam instead of the generic backend helper path, while
preserving the existing bootstrap `BackendAssembleResult` contract above that
handoff. Added a focused x86 LIR boundary test proving the generic backend
assemble entry now stages through the canonical x86 API surface.

## Suggested Next

Audit the remaining step 4 public x86 dispatch and dump surfaces in
`src/backend/backend.cpp`, especially whether any semantic/prepared debug or
dump branches still hide target-local ownership behind generic helpers without
explicit classification.

## Watchouts

- `assemble_target_lir_module(...)` still preserves the generic backend
  bootstrap object-emission contract; this packet only makes the x86 staged-text
  handoff explicit and does not claim target-local object emission ownership at
  the backend front door.
- The real x86 object-emitting assemble path remains target-local; do not pull
  the assembler object-writing stack into `backend.cpp` unless step 4 or a
  follow-on plan explicitly widens that boundary.

## Proof

Step 4 backend-front-door assembly classification packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
