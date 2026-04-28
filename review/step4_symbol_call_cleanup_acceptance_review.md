# Step 4 Symbol-Call Cleanup Acceptance Review

## Scope

Objective: re-review the dirty Step 4 same-module symbol-call renderer slice after cleanup addressed `review/step4_symbol_call_acceptance_review.md`.

Focus files:
- `src/backend/mir/x86/module/module.cpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `todo.md`

No clang-tools were used; the raw diff was not ambiguous.

## Review Base

Chosen lifecycle base: `f8280a1c [plan] reset step 4 loop-countdown route`.

Why: this is still the latest lifecycle-tagged `plan.md` checkpoint that reset the active idea-121 Step 4 route. The requested dirty review target is the unstaged cleanup after `HEAD` commit `07f92093 recover prepared guard chain rendering`, but the reviewer workflow base remains the active Step 4 plan checkpoint.

Commits since base: 2.

History ambiguity: none.

## Prior Blocking Findings Rechecked

1. Resolved: prepared local-slot memory operands are now emitted.

   `append_prepared_symbol_call_local_i32_function` routes store and load memory through `render_local_slot_memory(...)`, then emits the returned operand directly for both stores and loads (`src/backend/mir/x86/module/module.cpp:3549`, `src/backend/mir/x86/module/module.cpp:3558`, `src/backend/mir/x86/module/module.cpp:3583`, `src/backend/mir/x86/module/module.cpp:3594`). The helper validates result/stored value identity and returns the operand rendered from the prepared frame-slot access (`src/backend/mir/x86/module/module.cpp:3875`, `src/backend/mir/x86/module/module.cpp:3883`, `src/backend/mir/x86/module/module.cpp:3890`, `src/backend/mir/x86/module/module.cpp:3895`).

2. Resolved: load result homes are honored.

   The load path now requires the prepared i32 value home, narrows that prepared register, emits the load into that register, and records the same register for later argument movement (`src/backend/mir/x86/module/module.cpp:3576`, `src/backend/mir/x86/module/module.cpp:3592`, `src/backend/mir/x86/module/module.cpp:3594`, `src/backend/mir/x86/module/module.cpp:3595`).

3. Resolved: the route-shaped text-header switch is gone.

   I found no remaining `module_has_same_module_symbol_call_local_i32_route` detector. The repeated `.intel_syntax noprefix` / `.text` emission is now a general per-defined-function policy after the first emitted function, not a same-module symbol-call route gate (`src/backend/mir/x86/module/module.cpp:5809`, `src/backend/mir/x86/module/module.cpp:5815`).

4. Resolved: focused mutation proof covers the cleanup hazards.

   The test now mutates prepared frame-access addresses and expects the rendered stores/loads to follow those prepared operands (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3430`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3451`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3462`). It also mutates a load result home to `r10` and expects both the load and later printf move to consume that prepared home (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3469`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3484`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3498`). Both checks run before the later helper-prefix boundary (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:4304`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:4308`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:4311`).

## Remaining Boundary

The later red `bounded multi-defined helper same-module local byval call route` is recorded in `todo.md` as the next helper-prefix renderer boundary (`todo.md:25`, `todo.md:31`, `todo.md:54`). The current `test_after.log` shows configure/build passed, `backend_x86_prepared_handoff_label_authority` passed, and `backend_x86_handoff_boundary` advanced to that later helper-prefix failure. That later failure does not block committing this cleaned bounded symbol-call slice.

## Findings

No blocking findings.

The cleaned dirty slice is acceptable bounded Step 4 progress. It remains a narrow same-module symbol-call/local-slot route, but it now consumes the prepared authorities called out in the prior review and adds focused mutation proof for the exact frame-access and load-home drift risks.

## Judgments

Plan alignment: `on track`.

Idea alignment: `matches source idea`.

Technical-debt judgment: `acceptable`.

Validation sufficiency: `narrow proof sufficient`.

Reviewer recommendation: `continue current route`.

Commit recommendation: acceptable to commit the dirty slice with the known later local-byval helper-prefix blocker recorded in `todo.md`.
