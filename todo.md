Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove prepared authority handoff and preserve the split

# Current Packet

## Just Finished

- Step 4 is accepted as complete enough to advance: relocation-only one-slot
  pointer object data published prepared relocation-required/relocation-present
  authority and moved `src/921110-1.c` past the prepared selected object-data
  contract stop.
- `src/20010924-1.c`, `src/pr61517.c`, `src/pr57877.c`, and
  `src/pr57860.c` remain precise fail-closed prepared contract stops. The
  mixed aggregate case is parked until prepared emitted bytes plus relocation
  slots can be represented safely.
- Remaining RV64 relocation-record consumption belongs to
  `ideas/open/609_rv64_global_data_consumer.md`, not this plan.

## Suggested Next

- Execute Step 5 handoff proof. Re-run the selected object-data allowlist and
  record the Step 4 diagnostic movement plus the still-parked prepared stops.
- Include the Step 2 and Step 3 parked evidence in the handoff summary: both
  generic prepared global memory and direct base-plus-offset helper-only
  `ByteStorageAggregate` publication experiments changed prepared dumps but did
  not move the exact object-route diagnostics.
- Recommended proof command:
  `{ cmake --build --preset default && ALLOWLIST=build/agent_state/608_step4_selected_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`

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
- Do not repeat the aggregate `ByteStorageAggregate` publication helper as
  accepted Step 3 progress unless a future packet can explain why the exact
  direct base-plus-offset diagnostic would move. Current evidence says the
  diagnostic is not controlled solely by that prepared layout field.
- For Step 4, avoid moving byte emission into RV64 as a substitute for prepared
  object-data authority.
- Preserve explicit unsupported or invalid-prepared diagnostics when
  initializer, layout, relocation, or object-data facts are absent or out of
  scope.
- The remaining failure for moved row `src/921110-1.c` is now an RV64
  relocation-record consumer stop: `RV64 object route cannot emit prepared
  relocation object data without relocation records`. That is outside this
  executor packet and should stay with
  `ideas/open/609_rv64_global_data_consumer.md`.
- Do not mark mixed aggregate object data coherent until both ordinary emitted
  bytes and relocation slots can be represented as prepared facts.
- Step 5 is a proof and handoff packet unless evidence reveals a small
  608-owned prepared-authority gap that can move a row without touching RV64
  consumer/emission policy.

## Proof

- Latest accepted Step 4 proof:
  `{ cmake --build --preset default && ALLOWLIST=build/agent_state/608_step4_selected_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`
- Result: build succeeded; allowlist still failed overall with `0/5` passed and
  `5/5` failed.
- Diagnostic movement: `src/921110-1.c` moved from `prepared selected
  object-data contract status=unsupported_but_coherent` to the RV64
  relocation-record diagnostic. `src/20010924-1.c`, `src/pr61517.c`,
  `src/pr57877.c`, and `src/pr57860.c` retained the prepared contract stop.
