Status: Active
Source Idea Path: ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Coordinate Publication

# Current Packet

## Just Finished

Step 2 (`Repair Coordinate Publication`) generalized out-of-SSA parallel-copy
immediate publication for scalar integer immediates beyond `I32`.

`src/backend/prealloc/regalloc/phi_moves.cpp` now accepts immediate `I8`,
`I16`, `I32`, and `I64` parallel-copy sources when publishing
`phi_join_immediate_materialization` move resolutions. The published records
continue to use the existing parallel-copy step index, execution block,
predecessor label, successor label, authority, and block-entry coordinates.

`src/backend/mir/riscv/codegen/object_emission.cpp` now accepts the same
scalar integer immediate family when matching prepared out-of-SSA immediate
materialization against the parallel-copy source before emitting RV64 `li`.

Focused tests were added for an `i64 0` out-of-SSA phi incoming: the producer
test proves a value-location move bundle is published and addressable through
the right-to-join parallel-copy coordinates, and the RV64 object test proves
the object path accepts/materializes the `i64 0` move. Existing `I32`
immediate behavior remains covered by the existing tests.

## Suggested Next

Execute Step 3 by deciding whether this active route should close or record
the row advancement: `src/960209-1.c` no longer reports the audited
`prepared_consumer_category=missing_move_bundle` blocker and now stops at
`unsupported_local_memory_access` in the RV64 object route.

## Watchouts

- The one-row scan still exits `1`, but the first blocker advanced from
  missing move-bundle publication to `unsupported_local_memory_access: RV64
  object route requires prepared frame-slot or pointer-value base-plus-offset
  local memory addressing`.
- No expectations, unsupported markers, allowlists, or runtime comparison
  behavior were changed.
- The struct field remains named `source_immediate_i32` even though it already
  stores `std::int64_t`; this packet generalized the accepted producer/object
  semantics without widening that public surface.

## Proof

Delegated proof command was run exactly:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; echo "== ctest producer/object subset =="; ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_riscv_object_emission|backend_prepared_object_consumer_contract)$'; echo '== ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh =='; ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

Build succeeded. Focused CTest passed:
`backend_prepare_phi_materialize`,
`backend_riscv_object_emission`, and
`backend_prepared_object_consumer_contract`.

The focused allowlist scan exited `1` because `src/960209-1.c` still fails,
but the case log no longer contains `prepared_consumer_category=missing_move_bundle`.
The current diagnostic is `[RV64_C4C_OBJ_COMPILE_FAIL]` with
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot or pointer-value base-plus-offset local memory addressing`.
Canonical proof log: `test_after.log`; case log:
`build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`.

Supervisor acceptance also ran:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The backend bucket passed `345/345`.
