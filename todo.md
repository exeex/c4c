# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Idea-60 Ownership And Confirm The Next Scalar Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Step 1 ownership refresh complete: `backend_cli_dump_mir_00204_match_rejection`
and `backend_cli_trace_mir_00204_match_rejection` still pass, so idea 67 stays
parked, while `c_testsuite_x86_backend_src_00204_c` still fails inside the
idea-60 x86 scalar/prepared emitter contract. The current seam remains the
`match` compare-driven-entry handoff in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, where the prepared
shape reaches the x86 lane with `params=2`, `non-variadic i32 params=0`,
`non-i32 or varargs params=2`, and `function variadic=no`, but the emitter
still only accepts that route when the function exposes exactly one
non-variadic `i32` parameter.

## Suggested Next

Take one idea-60 repair packet on the prepared-module compare-driven-entry
gate for `match`, extending or rerouting that scalar/control-flow seam
generically enough to consume the current two-non-`i32` parameter shape
without reopening idea-67 observability work.

## Watchouts

- Do not reopen idea 67 unless a fresh supported-CLI scan shows the route has
  regressed back to an opaque rejection that needs more observability work.
- The nearest existing backend protection is still the delegated pair
  `backend_cli_dump_mir_00204_match_rejection` and
  `backend_cli_trace_mir_00204_match_rejection`; within
  `backend_x86_handoff_boundary`, the closest compare-driven-entry guard is the
  multi-parameter rejection lane in
  `backend_x86_handoff_boundary_compare_branch_test.cpp`, but that nearby
  coverage currently protects `params=2`, `non-variadic i32 params=1`,
  `non-i32 or varargs params=1` rather than the exact `00204` two-non-`i32`
  shape.
- Reject x86-only matcher growth for one named compare, branch, or return
  spelling; prefer generic prepared compare-driven-entry or terminator
  consumption.
- Keep routing explicit if the current seam graduates into idea 61, idea 65,
  or another downstream leaf after scalar emission succeeds.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_mir_00204_match_rejection|backend_cli_trace_mir_00204_match_rejection|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
Result: build completed, `backend_cli_dump_mir_00204_match_rejection`,
`backend_cli_trace_mir_00204_match_rejection`, and `backend_x86_handoff_boundary`
passed, and `c_testsuite_x86_backend_src_00204_c` failed with the current x86
emitter contract message about the bounded minimal return / equality-guard /
compare-against-zero branch family. Proof log: `test_after.log`.
