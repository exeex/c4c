Status: Active
Source Idea Path: ideas/open/04_aarch64_prior_preservation_baseline_drift.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prior-Preservation Source Selection

# Current Packet

## Just Finished

Step 1 reproduced `c_testsuite_aarch64_backend_src_00040_c` as a runtime
failure while `backend_aarch64_instruction_dispatch` still passed. Step 2
completed the narrow AArch64 repair: explicit callee-saved
`PriorPreservation` selections can now consume the matched prepared preserved
record, and after-call result moves are emitted before preservation
republication so restored prior values do not clobber call results.

## Suggested Next

Supervisor should decide whether to advance to Step 3 broader validation for
the remaining AArch64 c_testsuite regression family.

## Watchouts

The repair keeps incomplete explicit stack prior-preservation selections
fail-closed and does not rederive prior homes for unrelated explicit selection
kinds. The representative runtime also needed after-call explicit moves to be
lowered independently of preservation-effect ordering; otherwise the result
store was skipped and `x0` was republished from `x20` first.

## Proof

(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00040_c)$') > test_after.log 2>&1

Passed. Proof log: `test_after.log`.
