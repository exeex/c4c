Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Repair Pointer-Addressed Aggregate Store Admission Gap

# Current Packet

## Just Finished

Step 8 follow-up - Repair pointer-addressed aggregate store admission gap
completed for `src/20001026-1.c`.

Implemented a semantic aggregate-store producer rule for stores whose source is
a local aggregate value and whose destination is an existing `PointerAddress`
fact. The store path now consumes the destination pointer/GEP provenance,
decomposes the aggregate source into scalar leaves, and emits producer-owned
leaf `StoreLocalInst` records with `MemoryAddress::BaseKind::PointerValue`.
Each leaf address uses the destination pointer base, the pointer/GEP byte offset
plus the aggregate leaf offset, and a leaf requested range.

Focused BIR coverage added:
`expect_pointer_addressed_aggregate_field_store_publishes_leaf_stores`.

The selected RV64 representative row moved: `src/20001026-1.c` no longer fails
at `store local-memory` semantic `lir_to_bir` admission. The recheck now reaches
the RV64 object route and fails downstream as
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`.

## Suggested Next

Ask the supervisor to choose the next route: either handle the downstream RV64
unsupported-instruction fragment for the moved `src/20001026-1.c` row, or return
to the remaining local-memory semantic representative families.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

Remaining neighboring representative families are still intentionally left for
later packets: load
`src/20000314-1.c`, GEP `src/20000717-4.c`, scalar/local-memory
`src/20000519-1.c`, and alloca `src/20050604-1.c`.

## Proof

Proof log: `test_after.log`.

Commands run:
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- `ALLOWLIST=build/agent_state/557_step8_20001026.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh` appended to `test_after.log`

Backend test result: passed, `345/345`.

RV64 representative result: script returned failed `0/1`, but the row moved
from semantic `store local-memory` admission to downstream
`unsupported_instruction_fragment` in
`build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log`.
