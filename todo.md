Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Inspect Load Local-Memory Producer Boundary

# Current Packet

## Just Finished

Step 9 - Reconcile the five representative families completed as a proof and
log-inspection packet. The runbook is now extended and the source idea remains
active.

The five-row RV64 backend-object scan passed `0/5` and failed `5/5`, but the
store-family representative remains moved for the producer-owned reason from
Step 8:

- `src/20001026-1.c`: moved off semantic `store local-memory` admission and now
  fails downstream in the RV64 object route as
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

The remaining four representatives still fail in semantic local-memory
admission:

- `src/20000314-1.c`: `main` fails in `load local-memory`.
- `src/20000717-4.c`: `x` fails in `gep local-memory`.
- `src/20000519-1.c`: `foo` fails in `scalar/local-memory`.
- `src/20050604-1.c`: `foo` fails in `alloca local-memory`.

Lifecycle decision: keep the same source idea active. Do not close or
deactivate because four representative local-memory semantic admission families
remain. Do not split a new source idea for those four rows. The moved
`src/20001026-1.c` downstream object-lowering failure is outside the
local-memory semantic producer-admission route and should be handled only if
the supervisor chooses a separate downstream RV64 object packet.

## Suggested Next

Step 10 - Inspect Load Local-Memory Producer Boundary.

Next executor packet:

- Inspect `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log` and any
  available semantic/prepared BIR dumps for the failing `main` load.
- Trace the failing load through `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/`, especially local-slot address,
  requested range, layout authority, and load-source admission paths.
- Compare the failing shape with existing focused BIR coverage in
  `tests/backend/bir/`.
- Record the exact load-family boundary, the focused BIR test gap, and the
  RV64 representative proof command the supervisor should use after repair.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

Step 10 is inspection-first. Do not implement a load repair until the missing
producer fact and focused BIR coverage gap are named in `todo.md`.

No expectation, unsupported-marker, allowlist, runtime comparison, or semantic
admission weakening was performed in this packet.

## Proof

Proof log: `test_after.log`.

Command run:
- `cmake --build --preset default && ALLOWLIST=build/agent_state/557_step9_five_representatives.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

Result: build completed with no work to do; RV64 representative scan returned
nonzero with passed `0/5`, failed `5/5`. This is a valid Step 9 proof result
because the packet was a reconciliation packet and nonzero was expected unless
all rows passed.

Inspected case logs:
- `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log`
