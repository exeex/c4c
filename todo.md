Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Rejected the probe-only `plan.md` Step 3 ("Implement One Honest Capability
Lane") attempt by removing the bounded integer-switch lowering/test scaffolding
that only advanced backend-local probing. The c-testsuite blockers remain the
same truthful boundary: `00158` still needs loop-plus-direct-runtime support in
one function, and `00193` still needs a nontrivial multi-defined helper route
with parameterized switch control flow plus direct variadic runtime calls.

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
- `00158` is not a pure switch lane: it includes a counted loop and direct
  `printf` calls around the switch body.
- `00193` is not a single-function lane: it includes a nontrivial helper
  function `fred(int)` with switch arms that call `printf`, plus a `main`
  caller.
- Accepting this packet as progress would overstate the route unless the plan
  explicitly widens to those x86 emitter capabilities.

## Proof

Cleanup proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_(lir_to_bir_notes|x86_handoff_boundary))$" |& tee test_after.log'`

Result: both backend-owned proof tests passed after removing the rejected
integer-switch probe slice. `test_after.log` is the cleanup proof log.
