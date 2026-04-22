# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed a step-1.5 transitional-forwarding audit packet by rebinding
`src/backend/backend.cpp` to the reviewed
`src/backend/mir/x86/codegen/api/x86_codegen_api.hpp` emit surface and a new
explicit `src/backend/mir/x86/codegen/route_debug.hpp` declaration seam,
removing stale route-debug and legacy public-entry declarations from the broad
`x86_codegen.hpp` compatibility header, and splitting
`api/x86_codegen_api.cpp` so the emit-facing api object no longer drags the
x86 assembler ownership graph into ordinary backend links while
`x86_codegen_api_assemble.cpp` retains the target-aware assemble contract.

## Suggested Next

Continue step 1.5 by auditing whether the remaining consumers of
`src/backend/mir/x86/codegen/x86_codegen.hpp`, especially module-facing
headers and backend tests, still rely on broad compatibility reach-through for
types or declarations that should move into narrower reviewed seams.

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

## Proof

Step 1.5 transitional-forwarding audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
