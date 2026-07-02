Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Current RV64 Representative Rows

# Current Packet

## Just Finished

Step 4 - Prove Current RV64 Representative Rows ran the supervisor-selected
five-row proof after the Step 3 producer repair and refreshed the matching RV64
backend object five-row scan. The delegated `llvm_gcc_c_torture` CTest subset
passes all five representatives. The backend-object scan still passed 0/5
rows, failed 5/5 rows, and recorded the same current semantic-admission
families: `src/20000314-1.c` remains a load local-memory admission failure in
`main`, `src/20000717-4.c` remains a gep local-memory admission failure in `x`,
`src/20001026-1.c` remains a store local-memory admission failure in
`build_real_from_int_cst_1`, `src/20000519-1.c` remains a scalar/local-memory
admission failure in `foo`, and `src/20050604-1.c` remains an alloca
local-memory admission failure in `foo`.

## Suggested Next

Continue this source idea with a follow-up producer/admission packet rather than
closing it. The next coherent packet should target the remaining local-memory
admission gaps exposed by these rows, starting with one family narrow enough to
prove with the corresponding RV64 backend object case log.

## Watchouts

- This proof packet did not touch implementation files, expectations,
  unsupported markers, allowlists, runtime comparison behavior, or baseline
  logs.
- The proof record now includes both the passing `llvm_gcc_c_torture` CTest
  subset and the refreshed RV64 backend object runner that writes
  `build/rv64_gcc_c_torture_backend/src_*/case.log`. The backend-object scan is
  intentionally not green yet: it passed 0/5 and failed 5/5 representative rows.
- The source idea should continue. It should not close until representative
  RV64 backend object rows move from semantic-admission failure to runtime
  comparison pass, or until the plan is explicitly split/retired by the plan
  owner.

## Proof

Ran exactly `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(llvm_gcc_c_torture_src_20000314_1_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20001026_1_c|llvm_gcc_c_torture_src_20000519_1_c|llvm_gcc_c_torture_src_20050604_1_c)$'`,
with combined output recorded in `test_after.log`. Result: build succeeded and
all five delegated CTest rows passed.

Supervisor also refreshed `test_after.log` with
`cmake --build --preset default && ALLOWLIST=build/agent_state/557_step4_representatives.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
Result: build succeeded, the RV64 backend object five-row scan passed 0/5 rows
and failed 5/5 rows, and the current `case.log` files still show the same
local-memory semantic admission families:
`build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log` reports load
local-memory in `main`;
`build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log` reports gep
local-memory in `x`;
`build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log` reports store
local-memory in `build_real_from_int_cst_1`;
`build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log` reports
scalar/local-memory in `foo`; and
`build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log` reports alloca
local-memory in `foo`.
