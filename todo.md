Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by updating the x86 prepared-module boundary
contract and handoff coverage to describe the landed `00210` lane truthfully as
one bounded multi-defined-function `main`-entry route with same-module symbol
calls and direct variadic runtime calls, while keeping the adjacent
`00189`-style global-function-pointer plus indirect variadic-runtime boundary
explicitly unsupported.

## Suggested Next

Execute `plan.md` Step 5 by re-running the bounded prepared-module proving
cluster, including the explicit `00189`-style out-of-scope boundary check, and
use that result to decide whether the runbook is ready for close/switch review
or still needs another bounded prepared-module packet.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not generalize this slice into opaque indirect-call support; the admitted
  provenance remains bounded to same-module symbols, not loaded global function
  pointers.
- Keep `00189` explicit as the adjacent indirect/global-function-pointer plus
  indirect variadic-runtime neighbor even though direct variadic runtime calls
  inside the admitted `00210` lane are now documented as supported.
- Keep `00057` and `00124` out of the next packet; they remain emitter and
  scalar-control-flow families unrelated to this prepared-module lane.

## Proof

Step 4 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00131_c|c_testsuite_x86_backend_src_00211_c|c_testsuite_x86_backend_src_00210_c)$' > test_after.log 2>&1`

Result: `backend_lir_to_bir_notes`, `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00131_c`,
`c_testsuite_x86_backend_src_00211_c`, and
`c_testsuite_x86_backend_src_00210_c` all pass after the Step 4 contract-note
update, and `backend_x86_handoff_boundary` now also keeps the adjacent
multi-defined global-function-pointer plus indirect variadic-runtime boundary
explicitly rejected. Proof log path: `test_after.log`.
