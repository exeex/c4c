Status: Active
Source Idea Path: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Intrinsic Memory Producer Coverage

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: added focused BIR coverage for runtime
intrinsic non-local destination memory-effect publication.

The new coverage uses raw LIR fixture modules, not gcc_torture filenames:

- `runtime_memcpy_pointer_destination` calls `@memcpy` with a destination GEP
  derived from formal pointer `%p.buf` and a declared global byte source. It
  asserts explicit `GlobalSymbol` source load facts and `PointerValue`
  destination store facts with provenance rooted at the formal parameter.
- `runtime_memset_global_destination` calls `@memset` with a declared global
  destination and asserts `StoreGlobalInst` publication with explicit
  `GlobalSymbol` `MemoryAddress` provenance and `LinkNameId` identity.

The tests required the minimal producer repair in
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp`. The runtime intrinsic call
path now admits non-local destination stores for the covered shapes:
`memcpy` from a linear-addressable global source to a pointer-value
destination, and `memset` to a linear-addressable global destination. The
repair reuses existing pointer/global provenance and scratch-slot publication
patterns and does not touch expectations, unsupported markers, runtime
comparisons, or RV64 object consumers.

## Suggested Next

Proceed to Step 3 for the remaining `memcpy` producer repair. The
supervisor-run representative proof shows `src/20041218-1.c` moved off
`memset runtime family` to downstream RV64 global-data ownership, while
`src/20000703-1.c` still fails in function `foo` in `memcpy runtime family`.
The next packet should repair the actual representative `memcpy` shape,
probably GEP-derived global-source/pointer provenance rather than only the
direct global-source fixture covered in Step 2.

## Watchouts

- `src/20000703-1.c` first fails before reaching the later `bar` `memset` and
  additional `memcpy` calls. A representative move for `foo` should not be
  overstated as whole-row completion until the allowlist log is inspected.
- The repair intentionally covers the required pointer-value `memcpy`
  destination and global-symbol `memset` destination shapes. Broader arbitrary
  non-local intrinsic combinations remain separate unless the representative
  proof requires them.
- The new `memcpy` path reads from declared linear-addressable globals. It does
  not add string-pool or named-case shortcuts.

## Proof

- `./build/tests/backend/bir/backend_lir_to_bir_notes_test` passed.
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  passed; `test_after.log` is the proof log path.
- Supervisor representative proof:
  `printf '%s\n' src/20000703-1.c src/20041218-1.c > build/agent_state/559_step2_intrinsic_memory_after.allowlist && ALLOWLIST=build/agent_state/559_step2_intrinsic_memory_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/559_step2_intrinsic_memory_after.log 2>&1`
  exited `1`. `src/20041218-1.c` moved to
  `unsupported_global_data: RV64 object route requires supported prepared global
  memory facts`; `src/20000703-1.c` remains in `memcpy runtime family` in
  function `foo`. Proof log:
  `build/agent_state/559_step2_intrinsic_memory_after.log`.
