Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Reconcile The Five Representative Families

# Current Packet

## Just Finished

Step 9 - Reconcile the five representative families completed as a proof and
log-inspection packet.

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

Current lifecycle judgment from this packet: the source idea should continue.
It should not close or deactivate because four representative local-memory
semantic admission families remain. A source-idea split is not needed for those
four rows. The moved `src/20001026-1.c` downstream object-lowering failure is
outside the local-memory semantic producer-admission route and should be handled
only if the supervisor chooses a separate downstream RV64 object packet.

## Suggested Next

Continue this source idea with one focused producer/admission packet for one of
the four still-failing semantic local-memory representative families: load
`src/20000314-1.c`, GEP `src/20000717-4.c`, scalar/local-memory
`src/20000519-1.c`, or alloca `src/20050604-1.c`.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

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
