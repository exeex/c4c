Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Steps 1-2 by re-baselining the next frontier around the
delegated `00057` / `00124` / `00189` evidence subset and naming one bounded
next family. The honest next lane is the single-function prepared-module
boundary at the x86 prepared handoff/emitter edge, with
`c_testsuite_x86_backend_src_00189_c` as the proving anchor because both
backend boundary tests already pass and `00189` fails only on the emitter's
current `functions.size() != 1` gate. Nearby neighbors remain out of scope for
this lane: `00057` is still a minimal single-block emitter/control-flow family
and `00124` is still a scalar-control-flow semantic family.

## Suggested Next

Implement the smallest shared multi-function prepared-module lane that admits
`00189` through the canonical prepared x86 handoff without widening into
generic control flow, bootstrap-global work, or runtime-heavy multi-function
programs. The first implementation packet should stay on prepared-module
ownership and direct-call / global-function-pointer plumbing, then prove the
lane with `00189` plus any immediately adjacent same-family probe that fails
for the same prepared-module reason.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not treat `00057` or `00124` as regressions in this lane; they are the
  neighboring emitter/control-flow and scalar-control-flow semantic families
  that Step 1 separated on purpose.
- Do not satisfy the next packet by merely deleting the
  `functions.size() != 1` rejection. The route must keep prepared-module
  ownership honest for declarations, helper functions, and the one function
  whose body is actually emitted.
- Keep bootstrap globals, generic multi-block control flow, and broad
  multi-function/runtime surfaces out of scope unless the next packet proves
  they are required by the chosen prepared-module lane.

## Proof

Baseline capture before the route-selection packet:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00057_c|c_testsuite_x86_backend_src_00124_c|c_testsuite_x86_backend_src_00189_c)$' | tee test_before.log`

Delegated proof rerun for this packet:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00057_c|c_testsuite_x86_backend_src_00124_c|c_testsuite_x86_backend_src_00189_c)$' | tee test_after.log`

The delegated subset is sufficient for Steps 1-2 route evidence: both backend
boundary tests pass, while `00057`, `00124`, and `00189` fail in three
different families, which makes `00189` the cleanest next bounded
single-function prepared-module probe. Proof log path: `test_after.log`.
