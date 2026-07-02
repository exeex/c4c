Status: Active
Source Idea Path: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconcile Runtime Intrinsic Memory Representatives

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: repaired the remaining representative
runtime intrinsic memory producer admission failures exposed by
`src/20000703-1.c`.

The `memcpy` runtime path now admits the representative `foo` shape:
an `i32` immediate copy count, a source pointer produced by a global GEP into a
linear-addressable string global, and a destination pointer produced from a
formal-parameter aggregate GEP. The repair reuses `global_pointer_slots_` and
preserves the source global byte offset when publishing `LoadGlobalInst`
memory facts.

After `foo` moved, the same row reached `bar` and exposed a pointer-value
`memset` destination loaded from a local pointer slot. The `memset` runtime
path now publishes pointer-destination `StoreLocalInst` memory facts using the
existing `pointer_value_addresses_` provenance, while preserving the prior
direct-global `memset` path.

Focused BIR coverage was extended with raw LIR fixtures for the global-GEP
`memcpy` source shape and loaded-pointer `memset` destination shape. No
expectations, unsupported markers, runtime comparison files, or RV64 object
consumers were changed.

## Suggested Next

Run Step 4 reconciliation from `plan.md`: re-run the representative RV64
allowlist for `src/20000703-1.c` and `src/20041218-1.c`, compare the results
against the Step 1 runtime/intrinsic producer-admission diagnostics, and
decide whether to close this source idea, split downstream RV64 global-data
ownership into separate work, or continue the runbook.

## Watchouts

- `src/20000703-1.c` and `src/20041218-1.c` now both fail only with
  `unsupported_global_data: RV64 object route requires supported prepared
  global memory facts`.
- The loaded-pointer `memset` repair publishes pointer-value memory facts from
  the loaded pointer identity; it does not claim the local pointer load
  preserves formal-parameter spelling through the intrinsic boundary.
- Broader arbitrary runtime intrinsic memory combinations remain outside this
  packet unless a future representative exposes them.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed; `test_after.log` is the proof log path.
- `printf '%s\n' src/20000703-1.c src/20041218-1.c > build/agent_state/559_step3_intrinsic_memory_after.allowlist && ALLOWLIST=build/agent_state/559_step3_intrinsic_memory_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/559_step3_intrinsic_memory_after.log 2>&1`
  exited `1` only because both rows now fail in downstream RV64 object-route
  unsupported ownership. Proof log:
  `build/agent_state/559_step3_intrinsic_memory_after.log`.
