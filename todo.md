Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Complete selected global object-data authority

# Current Packet

## Just Finished

- Lifecycle review parked Step 3 and advanced execution to Step 4. Step 3
  evidence showed the visible prepared-field experiment could change prepared
  dumps but did not move any row past the exact direct base-plus-offset
  diagnostic, so it is a blocker note rather than accepted progress.

## Suggested Next

- Execute Step 4 evidence-first for selected global object-data authority.
- Capture diagnostics and prepared object-data facts for representative
  `src/20010924-1.c` plus at least one neighboring selected object-data row.
- Edit only `PreparedGlobalObjectData` production or prepared contract
  verification if the evidence shows a missing 608-owned object-data fact:
  label, identity, extent, alignment, emitted bytes, zero-fill, relocation, or
  unsupported-marker state.
- Stop and return evidence if the remaining issue is RV64 byte emission,
  symbol materialization, access-width support, or another `609` consumer
  condition.

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

## Proof

- Lifecycle-only route repair. Validation: run `git diff --check`.
