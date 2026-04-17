Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 `Name The First Same-Module Global Lane` by
confirming that `00047` and `00048` form the first honest proving cluster:
both are same-module defined-global reads feeding straight-line equality
guards, and both currently stop at the same x86 prepared-module emitter gate.
The nearby cases remain explicitly out of scope: `00045` is still bootstrap
scalar-global setup, `00049` adds pointer-indirect global addressing on top of
the same emitted guard shape, and `00051` is still multi-block scalar control
flow in semantic `lir_to_bir`.

## Suggested Next

Start `plan.md` Step 2 by widening the prepared x86 same-module defined-global
lane only far enough for fixed-offset reads from defined globals feeding
equality-immediate guards and early returns. Use `00047` and `00048` as the
proving cluster and keep `00049` out until pointer-indirect global addressing
is explicitly routed.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep this route at the prepared x86 handoff boundary; do not reopen semantic
  local-memory work as routine follow-up.
- `00047` and `00048` currently fail with the same emitter-side minimal
  prepared-module guard note, so the route should widen shared x86 handoff or
  codegen support rather than case-specific test handling.
- `00045` fails earlier in bootstrap global-data lowering, not in the same
  emitter family.
- `00049` shares the emitter note but requires pointer-indirect global
  addressing (`&x` stored in a global aggregate), so pulling it in now would
  silently widen the route.
- `00051` fails in semantic scalar-control-flow lowering and must remain out of
  scope for this packet.

## Proof

Executed the Step 1 proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00045_c|c_testsuite_x86_backend_src_00047_c|c_testsuite_x86_backend_src_00048_c|c_testsuite_x86_backend_src_00049_c|c_testsuite_x86_backend_src_00051_c)$' >> test_after.log 2>&1`.
`backend_lir_to_bir_notes` and `backend_x86_handoff_boundary` passed.
`00047` and `00048` failed with the same x86 prepared-module emitter-family
note, `00045` failed in bootstrap global-data lowering, `00049` failed with
the same emitter-family note but remains out of scope because it adds
pointer-indirect global addressing, and `00051` failed in semantic
scalar-control-flow lowering. Proof log: `test_after.log`.
