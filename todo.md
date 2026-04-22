# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed another step-1.5 duplicate-ownership cleanup packet by promoting the
canonical i32 ABI register-narrowing helper into
`src/backend/mir/x86/codegen/abi/x86_target_abi.*`, deleting the copied
helper logic from `module/module_emit.cpp` and the handoff-boundary backend
tests, and rebinding those consumers to the reviewed `abi/` seam while
leaving the remaining broad `x86_codegen.hpp` dependencies in place only for
real helper declarations such as stack-memory operand rendering and
multi-defined dispatch support.

## Suggested Next

Continue step 1.5 by auditing the remaining
`src/backend/mir/x86/codegen/x86_codegen.hpp` dependencies that are still real
helper contracts, especially `render_prepared_stack_memory_operand(...)` and
`PreparedModuleMultiDefinedDispatchState`, to decide whether they should stay
as explicit compatibility holdouts until the reviewed `prepared/` seams land
or whether any existing reviewed owner can take them without changing the
stage-3 contract.

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
- Do not invent new replacement headers for prepared helpers during step 1.5;
  if a remaining helper has no reviewed owner yet, keep it classified as a
  compatibility dependency until the planned `prepared/` conversion work.

## Proof

Step 1.5 transitional-forwarding audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
