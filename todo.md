# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics split the dirty
post-review worktree from `review/step4_global_short_circuit_slice_review.md`.
The rejected local-slot short-circuit renderer route was removed from
`src/backend/mir/x86/module/module.cpp`, including its local-slot frame helper,
fixture-shaped renderer helpers, dispatch entry, and branch-condition target
validator exception.

The semantic same-module global and pointer-root ownership validation pieces for
compare-join rendering remain in the dirty implementation. The short-circuit
execution-site clarification in
`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp` also
remains.

## Suggested Next

Repair the prepared short-circuit branch-condition target identity mismatch
semantically before reintroducing any renderer path. The current selected proof
now fails because the prepared-module consumer rejects the minimal local-slot
short-circuit or-guard handoff with `prepared branch condition targets drifted
from prepared block targets`.

## Watchouts

Do not revive the removed local-slot short-circuit renderer as-is. Any future
short-circuit renderer needs a clearly defined semantic prepared lowering class
with nearby negative coverage for missing, drifted, and ambiguous prepared
identity.

The pointer-backed global compare-join route still validates the prepared
pointer-root global but does not complete semantic pointer dereference lowering;
the data emitter still publishes deferred initializer comments rather than
`.quad` symbol initializers. Keep that boundary explicit if the global
compare-join portion is accepted separately.

## Proof

Delegated proof command was run and preserved in `test_after.log`:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed on the current Step 4 blocker:
`minimal local-slot short-circuit or-guard route: x86 prepared-module consumer
rejected the prepared handoff with exception: canonical prepared-module handoff
rejected x86 control-flow label authority: prepared branch condition targets
drifted from prepared block targets`.
