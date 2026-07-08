Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Represent mixed object-data bytes plus relocation slots

# Current Packet

## Just Finished

- Lifecycle review kept `608` active after Step 5 instead of closing it.
- Step 5 is accepted as a handoff proof: `src/921110-1.c` moved past the
  prepared selected object-data contract stop to the `609` RV64
  relocation-record consumer diagnostic, while Step 2 and Step 3 remain parked
  as non-moving evidence.
- The source idea is still open because mixed selected object-data rows remain
  precise prepared contract stops until ordinary emitted bytes plus relocation
  slots can be represented coherently in prepared facts.

## Suggested Next

- Execute Step 6 evidence-first for mixed selected object-data authority.
- Capture current prepared object-data facts for `src/20010924-1.c` plus at
  least one neighboring selected object-data row that still stops at the
  prepared contract diagnostic.
- Edit only prepared object-data production or prepared contract verification
  if evidence shows a missing 608-owned representation for emitted bytes plus
  relocation slots. Stop and return evidence if the remaining issue requires
  RV64 byte emission, relocation records, symbol materialization, or a separate
  prepared-fact design idea.

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
- Parked Step 2 and Step 3 evidence should stay as route notes: helper-only
  `ByteStorageAggregate` publication changed prepared dumps but did not move
  the exact prepared-memory or direct base-plus-offset diagnostics.
- Do not mark mixed aggregate object data coherent unless both ordinary
  emitted bytes and relocation slots are represented as prepared facts.
- Do not move RV64 relocation-record production or byte emission into this
  plan.

## Proof

- Latest accepted Step 4 proof:
  `{ cmake --build --preset default && ALLOWLIST=build/agent_state/608_step4_selected_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`
- Result: build succeeded; allowlist still failed overall with `0/5` passed and
  `5/5` failed.
- Diagnostic movement: `src/921110-1.c` moved from `prepared selected
  object-data contract status=unsupported_but_coherent` to the RV64
  relocation-record diagnostic. `src/20010924-1.c`, `src/pr61517.c`,
  `src/pr57877.c`, and `src/pr57860.c` retained the prepared contract stop.
- Final Step 5 proof re-ran the same command into `test_after.log`; result
  remained build succeeded, allowlist `0/5`. Current diagnostics:
  - `src/921110-1.c`: `RV64 object route cannot emit prepared relocation
    object data without relocation records`, owned by `609`.
  - `src/20010924-1.c`, `src/pr61517.c`, `src/pr57877.c`,
    `src/pr57860.c`: precise prepared selected object-data contract stops.
- Lifecycle validation for advancing to Step 6 is `git diff --check`.
