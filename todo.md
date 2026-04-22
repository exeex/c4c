# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5.3
Current Step Title: Audit Remaining Broad-Header Holdouts Before Lowering Migration
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 1.5.3 by auditing the remaining backend handoff boundary tests
that still include `src/backend/mir/x86/codegen/x86_codegen.hpp`: kept
`backend_x86_handoff_boundary_scalar_smoke_test.cpp` and
`backend_x86_handoff_boundary_compare_branch_test.cpp` as explicit
compatibility holdouts for `render_prepared_stack_memory_operand(...)`, kept
`backend_x86_handoff_boundary_multi_defined_call_test.cpp` as a prepared
helper/render holdout where no narrower reviewed owner exists yet, and
removed the reviewed register-narrowing reach-through in that test by using
`x86::abi::narrow_i32_register_name(...)` instead of the broad-header helper.

## Suggested Next

Move to the next step-1.5.3 consumer packet outside these backend handoff
boundary tests and apply the same rule: remove only declaration reach-throughs
that already have a reviewed owner, while leaving helper/prepared compatibility
holdouts on `x86_codegen.hpp` until their real seams land.

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
- `render_prepared_stack_memory_operand(...)`,
  `render_prepared_trivial_defined_function_if_supported(...)`,
  `render_prepared_bounded_same_module_helper_prefix_if_supported(...)`, and
  the prepared call-argument ABI selectors still have no narrower reviewed
  owner, so forcing these three tests off `x86_codegen.hpp` would be fake
  narrowing rather than progress.
- Do not force helper-heavy tests off `x86_codegen.hpp` unless their actual
  helper declarations have a real narrower owner; API-only include churn is
  fine, but fake narrowing that breaks helper users is not progress.
- Do not invent new replacement headers for prepared helpers during step 1.5;
  if a remaining helper has no reviewed owner yet, keep it classified as a
  compatibility dependency until the planned `prepared/` conversion work.

## Proof

Step 1.5.3 backend handoff broad-header audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Proof log path: `test_after.log`
