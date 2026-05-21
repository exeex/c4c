Status: Active
Source Idea Path: ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 3/4 acceptance
Current Step Title: Post-commit regression fix for idea 381 Step 3/4 acceptance

# Current Packet

## Just Finished

Step 3/4 post-commit regression fix is complete for the prepared AArch64
call-boundary scalability route.

- Reproduced `c_testsuite_aarch64_backend_src_00181_c` as a runtime segfault.
- Traced the bad fact to `Hanoi` reloading stack-preserved live-through-call
  values (`%p.n` and `%p.spare`) from frame slots on a sibling CFG path where
  the new first-stack-preserved cache had skipped the required publication.
- Repaired the cache construction in AArch64 traversal so first stack-preserved
  publications are skipped only when a prior stack publication reaches the
  current call through same-block order or prepared CFG dominance.
- Confirmed the generated `Hanoi` non-base path now stores `w0` and `x3` to
  their preserved stack homes before the first recursive call.
- Preserved idea 381 `00200` advancement and the focused prepared
  call-boundary scalability path.

## Suggested Next

Return to the supervisor for review/commit handling of this regression-fix
slice.

## Watchouts

- The repaired path is general CFG-aware stack-preservation publication; it does
  not special-case `00181`, `00200`, a test name, expectations, unsupported
  lists, runners, or timeout policy.
- The focused pre-proof also passed `00181`, `00200`, and
  `backend_codegen_route_aarch64_prepared_call_boundary_scalability`.
- Do not work on parked idea 382 unless the supervisor switches lifecycle
  state.

## Proof

Delegated proof preserved in `test_after.log`:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00200_c$|c_testsuite_aarch64_backend_src_00181_c$)' ; } > test_after.log 2>&1`

Outcome: build completed; CTest ran 150 matching tests. `00181` passed,
`00200` passed, the focused prepared call-boundary scalability test passed, and
all backend tests in the delegated subset passed.
