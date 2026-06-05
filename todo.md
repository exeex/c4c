Status: Active
Source Idea Path: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Refresh Baseline Confidence

# Current Packet

## Just Finished

Step 5 repaired the three broader AArch64 HFA/f128 call-boundary blockers
without weakening expected output, unsupported markings, or baseline logs.

Scalar HFA global payload call arguments now bypass the stale already-emitted
FPR-source shortcut when the source value traces through prepared frame stores
back to a global payload, so `s0`/`s1` and `d0`/`d1` are loaded directly from
the global source at the call boundary.

Full-width f128 HFA carriers whose prepared register spelling must be re-viewed
as a q-register now materialize the selected call-boundary move as a byte-lane
`mov v*.16b, v*.16b` assembler copy. Plain q-register f128 moves, such as the
instruction-dispatch `q2 -> q0` case, remain selected call-boundary move
records.

Passing repair targets:
- PASS `backend_codegen_route_aarch64_hfa_global_payload_call_boundary`
- PASS `backend_codegen_route_aarch64_hfa_result_home_publication_contract`
- PASS `backend_aarch64_call_boundary_owner`
- PASS `backend_aarch64_instruction_dispatch`
- PASS `c_testsuite_aarch64_backend_src_00172_c`
- PASS `c_testsuite_aarch64_backend_src_00180_c`
- PASS `c_testsuite_aarch64_backend_src_00216_c`
- PASS `c_testsuite_aarch64_backend_src_00220_c`
- PASS `c_testsuite_aarch64_backend_src_00204_c`

## Suggested Next

Supervisor should review and commit this completed Step 5 regression-repair
slice, then decide whether the active runbook needs broader AArch64 validation
or plan-owner lifecycle handling.

## Watchouts

The f128 assembler materialization is intentionally limited to complete
full-width carriers whose prepared register spelling is not already a q-register
view. Keep ordinary q-register f128 moves as selected records so
`backend_aarch64_instruction_dispatch` continues to prove selected-move
ownership. No expected-output files, unsupported markings, `plan.md`, source
idea, or `test_baseline.log` were touched.

## Proof

Ran:
`cmake --build --preset default; ctest --test-dir build -j1 --output-on-failure -R '^(backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_hfa_result_home_publication_contract|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1`

Build rebuilt the touched AArch64 backend objects and linked dependent tests.
CTest result: 9/9 passed. Proof log: `test_after.log`.
