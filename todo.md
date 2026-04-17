Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 4 `Prove Nearby Same-Family Cases` on top of the
completed Step 2 and Step 3 same-module global lane work. The honest proving
cluster remains `00047` and `00048`; both now pass alongside
`backend_x86_handoff_boundary` and `backend_lir_to_bir_notes`, while adjacent
`00049` remains explicitly unsupported because it adds pointer-indirect global
addressing. The `x86_backend` checkpoint on the current tree now reports
`54/220` passing with `166/220` still failing.

## Suggested Next

This runbook packet is now proven. Route the next turn through lifecycle
decision-making: either close this runbook as exhausted or retarget a new
bounded capability-family packet. If a follow-on packet is activated, keep
`00045`, `00049`, and `00051` out of scope unless the route is explicitly
widened to an honest neighboring family.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep this route at the prepared x86 handoff boundary; do not reopen semantic
  local-memory work as routine follow-up.
- The accepted lane is fixed-offset i32 loads from one same-module defined
  global symbol feeding equality-immediate guard blocks with immediate-return
  leaves.
- `00045` fails earlier in bootstrap global-data lowering, not in the same
  emitter family.
- `00049` still fails with the x86 emitter note because it adds pointer-
  indirect global addressing (`load_global ptr @s, offset 8`) on top of the
  admitted same-module lane; pulling it in would silently widen the route.
- `00051` fails in semantic scalar-control-flow lowering and must remain out of
  scope for this packet.

## Proof

Executed the Step 4 proof command:
`cmake --build --preset default > test_after.log 2>&1 && (ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00047_c|c_testsuite_x86_backend_src_00048_c|c_testsuite_x86_backend_src_00049_c)$' >> test_after.log 2>&1 || true) && ctest --test-dir build -j --output-on-failure -L x86_backend >> test_after.log 2>&1`.
`backend_lir_to_bir_notes`, `backend_x86_handoff_boundary`,
`c_testsuite_x86_backend_src_00047_c`, and
`c_testsuite_x86_backend_src_00048_c` all passed. Adjacent
`c_testsuite_x86_backend_src_00049_c` still failed with the updated x86
emitter boundary message because it adds pointer-indirect global addressing and
remains out of scope. The full `x86_backend` checkpoint then reported
`54/220` passing with `166/220` still failing. Proof log: `test_after.log`.
