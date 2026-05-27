Status: Active
Source Idea Path: ideas/open/50_aarch64_memory_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Validation And Closure Check

# Current Packet

## Just Finished

Step 6 final validation and closure audit completed for active idea 50.
The corrected broad validation command rebuilt the default preset, then split
backend and named AArch64 c-testsuite CTest runs because the original combined
regex under-selected backend tests. Build was already current. The backend
subset reported 165 passed and 2 failed out of 167; the failures are the known
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`. The
named AArch64 c-testsuite subset passed `00176`, `00181`, and `00214`; the
expected known failures remained: `00196` failed with the existing runtime
mismatch, and `00207` timed out.

Step 6 audit found no current worktree source/test diff to review; the active
idea implementation is in `HEAD`. Audited the Step 1-5 implementation in
`src/backend/mir/aarch64/codegen/memory.cpp`,
`src/backend/prealloc/publication_plans.*`, and
`tests/backend/mir/backend_store_source_publication_plan_test.cpp`. No
expectation downgrades, unsupported rewrites, testcase-shaped matching, raw
move-bundle scans in `memory.cpp`, or AArch64-local store-global duplicate or
pending authority in `memory.cpp` were found. The remaining compatibility
parameters in `memory.cpp` are ignored at the relevant store-global call sites,
and pending/duplicate store-global publication state is provided by prepared
publication planning.

## Suggested Next

Supervisor can decide whether to close the active plan or request reviewer or
plan-owner closure handling. No executor implementation packet is suggested
from this Step 6 validation.

## Watchouts

The original combined regex selected only the five named
`c_testsuite_aarch64_backend` tests despite the `backend_` alternation, so the
supervisor split the run into `^backend_` and named c-testsuite invocations.
The corrected split run is the intended Step 6 broad validation. The known
backend failures, `00196` runtime mismatch, and `00207` timeout remain
unchanged blockers for a fully green broad subset.

## Proof

Supervisor reran Step 6 final validation with the corrected split command and
wrote `test_after.log`:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^backend_" 2>&1 | tee -a test_after.log; ctest --test-dir build -j --output-on-failure -R "^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c|c_testsuite_aarch64_backend_src_00214_c)$" 2>&1 | tee -a test_after.log'`

Build completed with no work. Backend CTest result: 165 passed, 2 known
failures out of 167
(`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`).
Named AArch64 c-testsuite result: `00176`, `00181`, and `00214` passed;
`00196` failed with the known runtime mismatch and `00207` timed out. The
overall command remains nonzero because the intended broad subset still
contains those known failures.
