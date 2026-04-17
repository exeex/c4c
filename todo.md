Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Rejected the attempted `plan.md` Step 4 `00050` probe as route drift: the case
depends on aggregate-backed global reads, which widens into the global/data
family instead of the active local-memory lane. Reverted the unaccepted shared
lowering change and revalidated the accepted `00042`/`00046` local-memory
sentinel.

## Suggested Next

Stay on `plan.md` Step 4, keep `00042`/`00046` as the local-memory sentinel,
and probe a true adjacent stack/object case such as `00043` or `00053`, where
the testcase shape stays inside straight-line local-memory work instead of
aggregate-backed global-data semantics.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- `00050` is out of scope for the active slice because its blocker lives in the
  global/data family, not the chosen first local-memory lane.
- Keep the next packet on adjacent same-family local-memory probes and honest
  pass-count movement; do not widen into scalar-cast, global-data, or broader
  multi-block control-flow routes.
- `00037` is still an adjacent scalar-cast-family failure and remains out of
  scope for this packet.

## Proof

Supervisor-chosen Step 4 proof command for this packet:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00042_c|c_testsuite_x86_backend_src_00046_c)$' >> test_after.log 2>&1`.
The build completed. `backend_lir_to_bir_notes`,
`backend_x86_handoff_boundary`, `c_testsuite_x86_backend_src_00042_c`, and
`c_testsuite_x86_backend_src_00046_c` all passed after reverting the rejected
`00050` drift.
Proof log:
`test_after.log`.
