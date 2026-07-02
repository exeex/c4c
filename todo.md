Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family completed the
scalar/local-memory producer repair for immediate memcpy between local scalar
pointer slots.

Implemented:

- `try_lower_immediate_local_memcpy` now handles scalar local-slot source to
  scalar local-slot target copies when both slots have the same scalar type and
  the requested immediate copy covers the source slot.
- The producer emits a `LoadLocalInst` from the source slot and a
  `StoreLocalInst` to the target slot.
- Both emitted accesses carry `LocalSlot` `MemoryAddress` facts with requested
  range/provenance for the copied pointer-sized bytes.
- Added focused BIR coverage:
  `expect_local_scalar_pointer_memcpy_copies_between_local_slots`.

RV64 representative result:

- `src/20000519-1.c` moved off semantic `scalar/local-memory` admission.
- The row now fails downstream in the RV64 object route with
  `unsupported_local_memory_access: RV64 object route requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing`.

## Suggested Next

Recommended next packet: continue the source idea with the remaining semantic
local-memory representative, likely `src/20050604-1.c` / `alloca local-memory`,
or have the supervisor route downstream RV64 object-preparation failures
separately if desired.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, `src/20000717-4.c`, and now
`src/20000519-1.c` have moved off semantic local-memory admission and should
not be absorbed back into this source idea without supervisor/lifecycle
direction.

## Proof

Proof log: `test_after.log`.

Backend proof:

- `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
- Result: passed, `345/345` backend tests.

RV64 representative proof appended to `test_after.log`:

- `ALLOWLIST=build/agent_state/557_step13_20000519.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Result: nonzero, `0/1`; row moved off semantic `scalar/local-memory`
  admission to downstream RV64 object-route `unsupported_local_memory_access`.
- Case log:
  `build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log`.
