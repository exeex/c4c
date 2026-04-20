# Execution State

Status: Active
Source Idea Path: ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate The Phi Legalization Route
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Classified the four `^backend_` checkpoint failures for Step 4 (`Validate The
Phi Legalization Route`) as pre-handoff semantic `lir_to_bir` backend-route
gaps outside the active phi-transfer scope. The failing cases all stop before
the canonical prepared-module handoff in `src/backend/backend.cpp` and
`src/backend/bir/lir_to_bir_module.cpp`, while the active idea and runbook are
scoped to authoritative phi-transfer work in `prealloc/legalize/regalloc`.
The targeted handoff proof surface still passed inside the broader run
(`backend_x86_handoff_boundary` stayed green), so the current Step 4 blocker
is validation-scope classification rather than an observed phi-route
regression.

## Suggested Next

Treat `backend_x86_handoff_boundary` plus the recorded out-of-scope
`lir_to_bir` classification as the honest Step 4 validation state, then hand
the lifecycle decision to the plan owner: either close the active phi-transfer
runbook as complete on its chosen validation scope or rewrite Step 4 only if
the source idea truly requires broader backend-route closure beyond the
prepared-module handoff.

## Watchouts

- Do not treat one additional phi testcase shape as success.
- Keep typed semantic ids as the public identity boundary.
- Do not silently fold branch/join ownership work from idea 62 back into this
  route.
- Step 3.2 inventory did not justify another packet in
  `backend_x86_handoff_boundary_joined_branch_test.cpp`.
- The remaining carrier-kind helper readers in prealloc/regalloc and the x86
  prepared-render callers are already represented by the current
  authoritative-handoff proof surfaces, especially
  `backend_x86_handoff_boundary_short_circuit`.
- The Step 4 broadened backend proof is currently blocked by semantic
  `lir_to_bir` route failures outside the narrow joined-branch/handoff subset;
  do not paper over that by rerunning only the narrow proof.
- `backend_x86_handoff_boundary` still passed inside the broadened run, so the
  current blocker is proof breadth, not an observed regression in the recent
  joined-branch contract-strict slices.
- The broader failures all die before `prepare_semantic_bir_pipeline(...)`
  reaches the authoritative phi-transfer surfaces, so they should not be
  reframed as `prealloc` or regalloc regressions without contrary proof.
- Keep Step 4 focused on validation-scope honesty and lifecycle disposition,
  not on inventing a new backend-route implementation initiative under idea 63.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
ran for this Step 4 checkpoint. `backend_x86_handoff_boundary` passed inside
that broader run, but the backend-focused validation failed overall with four
backend-route cases still failing before the prepared-module handoff in
semantic `lir_to_bir` lowering:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
