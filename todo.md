Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete direct global-symbol base-plus-offset authority

# Current Packet

## Just Finished

- Resolved the Step 2 route after an evidence-gated executor packet. The
  executor captured predicate inputs for `src/strlen-7.c` and
  `src/20000703-1.c`; the first visible missing field was
  `layout_authority=unknown`, while global-symbol base, symbol name,
  base-plus-offset eligibility, nonzero size/alignment, and in-bounds range
  verdict were already present.
- The executor's reverted prepared producer experiment could publish
  `ByteStorageAggregate` in representative prepared dumps, but the exact
  allowlist proof stayed `0/7` with unchanged
  `requires supported prepared global memory facts` diagnostics.
- Lifecycle decision: park Step 2 as blocked for now and advance to Step 3.
  The source idea remains active because direct global-symbol base-plus-offset
  authority and selected object-data authority are still 608-owned work.

## Suggested Next

- Execute Step 3 from `plan.md`: capture the current diagnostic and prepared
  address facts for `src/pr79737-2.c` plus at least one neighboring direct
  global-symbol base-plus-offset row. Edit only the prepared address /
  selected-address producer if the evidence shows a missing 608-owned fact.
- Stop and return evidence instead of editing if the prepared facts already
  prove direct symbol plus constant offset and the remaining stop is RV64
  materialization, relocation, or emission policy for `609`.

## Watchouts

- Keep RV64 global symbol emission and access-width lowering out of scope.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime behavior, accounting, or testcase-specific matching.
- Do not repeat the complete/global aggregate storage publication helper from
  the Step 2 experiment as accepted progress; it changed prepared dumps but did
  not move any allowlist row past `requires supported prepared global memory
  facts`.
- Preserve RV64/global consumer rows for `ideas/open/609_rv64_global_data_consumer.md`.
- `fragment_for_prepared_load_global()` and
  `prepared_global_access_is_supported()` do not currently consume
  `prepared_global_symbol_memory_has_publication_authority()` or layout
  authority, so the generic object-route diagnostic may be masking a later
  RV64 consumer/emission condition.
- For Step 3, do not treat relocation/materialization facts as pointer
  freshness authority and do not route into GOT/TLS or final target emission
  policy.

## Proof

- Step 2 evidence dumps:
  - `build/agent_state/608_step2_strlen_prepared_bir.txt`
  - `build/agent_state/608_step2_20000703_prepared_bir.txt`
  - `build/agent_state/608_step2_strlen_prepared_bir.after.txt`
  - `build/agent_state/608_step2_20000703_prepared_bir.after.txt`
- Step 2 delegated proof command:
  `{ cmake --build --preset default && ALLOWLIST=build/agent_state/608_step2_prepared_global_memory.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`
- Result: failed, `0/7` passed and `7/7` failed. The supported prepared
  global memory-facts rows in that allowlist still stop at
  `unsupported_global_data: RV64 object route requires supported prepared global
  memory facts`.
- No Step 3 proof has been run yet.
