Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Attempted `plan.md` Step 3 ("Implement One Honest Capability Lane") by adding
bounded semantic integer-switch lowering in `lir_to_bir` plus backend-test
fixtures for a minimal i32 parameter switch lane. That moved the route past the
earlier scalar-control-flow lowering failure, but the delegated proof stopped
at deeper x86 prepared-module gates: `00158` still requires looped control flow
plus direct variadic runtime calls inside one function, and `00193` still
requires a nontrivial multi-defined helper-function route with parameterized
switch control flow plus direct variadic runtime calls. This packet therefore
stops as a truthful blocker instead of widening into a broader x86 CFG/runtime
renderer.

## Suggested Next

Use the repaired `plan.md` Step 1 route and re-choose the first c-testsuite-
credible scalar-control-flow lane: either a bounded single-function loop-plus-
runtime prepared-module route or a bounded multi-defined helper-function route.
Keep the current integer-switch backend work as probe scaffolding only unless a
same-mechanism c-testsuite cluster is also proven.

## Watchouts

- Do not reopen idea `55`; the memory ownership split is closed.
- Treat idea `56` as a separate initiative; the current Step 1 baseline did
  not show renderer de-headerization as the immediate blocker.
- Do not weaken `x86_backend` expectations or add testcase-shaped shortcuts.
- The in-progress diff currently adds bounded integer-switch lowering and
  backend fixtures, but it does not make the delegated c-tests pass yet.
- `00158` is not a pure switch lane: it includes a counted loop and direct
  `printf` calls around the switch body.
- `00193` is not a single-function lane: it includes a nontrivial helper
  function `fred(int)` with switch arms that call `printf`, plus a `main`
  caller.
- Accepting this packet as progress would overstate the route unless the plan
  explicitly widens to those x86 emitter capabilities.

## Proof

Executor packet proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_(lir_to_bir_notes|x86_handoff_boundary)|c_testsuite_x86_backend_src_(00158|00193)_c)$" |& tee test_after.log'`

Result: `backend_lir_to_bir_notes` passed, but `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00158_c`, and
`c_testsuite_x86_backend_src_00193_c` failed in `test_after.log`. The current
blocking messages are x86 prepared-module route gates, not the earlier
semantic `lir_to_bir` scalar-control-flow family failure.
