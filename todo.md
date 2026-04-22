# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed a follow-on step-1.5 transitional-forwarding audit packet by moving
most backend handoff-boundary tests from the broad
`src/backend/mir/x86/codegen/x86_codegen.hpp` compatibility header to the
reviewed `src/backend/mir/x86/codegen/api/x86_codegen_api.hpp` emit surface
and `src/backend/mir/x86/codegen/route_debug.hpp`, while narrowing
`module/module_emit.hpp` and `module/module_data_emit.hpp` to explicit
prepared-module dependencies and making `module/module_emit.cpp` own the
remaining broad implementation include directly. The only tests left on
`x86_codegen.hpp` in this slice are the ones that still exercise real helper
declarations such as stack-memory operand and multi-defined helper rendering
support.

## Suggested Next

Continue step 1.5 by auditing the remaining real consumers of
`src/backend/mir/x86/codegen/x86_codegen.hpp`, especially helper-heavy backend
tests and codegen implementation files, to decide which declarations should
move into narrower reviewed helper seams versus which remaining broad includes
are still honest implementation dependencies for now.

## Watchouts

- Keep `emit.cpp` and `prepared_module_emit.cpp` wrapper-thin; step 1.5 is
  still classifying compatibility entrypoints, not handing ownership back to
  legacy files.
- Keep `route_debug.hpp` declaration-only; route-debug implementation should
  stay in `route_debug.cpp` and not recreate another broad mixed-owner header.
- Keep `x86_codegen_api.cpp` limited to emit-facing entry logic and
  `x86_codegen_api_assemble.cpp` limited to assemble ownership so ordinary
  backend links do not silently depend on assembler implementation files.
- Preserve the legacy `x86::emit_module(...)`, `x86::assemble_module(...)`,
  and `x86::emit_prepared_module(...)` symbols until the supervisor retires
  those compatibility surfaces explicitly.
- Backend tests that exercise route-debug reporting now need
  `route_debug.hpp`; do not rely on `x86_codegen.hpp` to leak those
  declarations back in.
- Do not force helper-heavy tests off `x86_codegen.hpp` unless their actual
  helper declarations have a real narrower owner; API-only include churn is
  fine, but fake narrowing that breaks helper users is not progress.

## Proof

Step 1.5 transitional-forwarding audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
