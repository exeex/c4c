# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed a step-1.5 transitional-forwarding audit packet by making the legacy
`src/backend/mir/x86/codegen/emit.cpp` public entrypoints explicit
compatibility wrappers over `src/backend/mir/x86/codegen/api/`, narrowing
`api/x86_codegen_api.hpp` so it no longer republishes the broad
`x86_codegen.hpp` surface, and moving x86-local assemble ownership into the
reviewed api seam while preserving the legacy `x86::assemble_module(...)`
symbol as a thin forwarding layer.

## Suggested Next

Continue step 1.5 by auditing whether any remaining legacy declarations in
`src/backend/mir/x86/codegen/x86_codegen.hpp` still advertise mixed ownership
across `api/`, `module/`, and route-debug helpers, then shrink or reclassify
those declarations without widening the public seam.

## Watchouts

- Keep `emit.cpp` wrapper-thin; do not reintroduce lowering or assembler
  ownership there now that `api/x86_codegen_api.cpp` owns the x86-compatible
  entry logic for this seam.
- Preserve the legacy `x86::emit_module(...)`, `x86::assemble_module(...)`,
  and `x86::emit_prepared_module(...)` symbols until the supervisor retires
  them explicitly; step 1.5 only reclassifies them as compatibility surfaces.
- `api/x86_codegen_api.hpp` now stands on backend-facing types directly; avoid
  pulling `x86_codegen.hpp` back into that public api unless a concrete build
  dependency requires it.
- `backend.cpp` still includes `x86_codegen.hpp` and was out of scope here, so
  any broader public-entry retirement must be coordinated in a later packet.

## Proof

Step 1.5 transitional-forwarding audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
