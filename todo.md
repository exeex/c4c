# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Completed a step-1.5 compatibility-classification packet by marking
`render_prepared_stack_memory_operand(...)` and
`PreparedModuleMultiDefinedDispatchState` /
`build_prepared_module_multi_defined_dispatch_state(...)` as explicit prepared
compatibility holdouts in `x86_codegen.hpp`, and by annotating the reviewed
`module/module_emit.cpp` and `route_debug.cpp` consumers so those remaining
`x86_codegen.hpp` includes are clearly intentional until the reviewed
`prepared/*` seams land.

## Suggested Next

Continue step 1.5 by auditing the other reviewed-seam consumers that still
include `src/backend/mir/x86/codegen/x86_codegen.hpp`, and separate true
prepared compatibility holdouts from any API/core/abi/module declarations that
already have a narrower owner so the remaining broad-header usage is limited
to honest staged dependencies only.

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

Step 1.5 compatibility-holdout classification packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Proof log path: `test_after.log`
Regression guard script result:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
reported no new failures but rejected the pair because passed count stayed
`106 -> 106`, so the canonical baseline was not rolled forward.
