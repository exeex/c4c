# Prepared Authority Step 6 Reconciliation

Source plan: `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`

Step 6 re-ran the deduplicated 43-row prepared-authority queue after Steps
2-5. The queue now has no unexplained prepared-authority gaps.

## Inputs

- Step 1 queue:
  `build/agent_state/552_step1_prepared_authority_queue/queue.tsv`
- Step 6 allowlist:
  `build/agent_state/552_step6_prepared_authority_reconciliation.allowlist`
- Step 6 row status:
  `build/agent_state/552_step6_prepared_authority_reconciliation/row_status.tsv`
- Step 6 current results:
  `build/agent_state/552_step6_prepared_authority_reconciliation/current_results.tsv`
- Proof log: `test_after.log`

## Reconciliation Counts

| Bucket | Rows |
| --- | ---: |
| Repaired/pass | 7 |
| Advanced to later RV64 diagnostic or lowering owner | 24 |
| Rerouted to earlier semantic producer or explicit multi-source authority | 9 |
| Runtime mismatch | 3 |
| Still unexplained prepared-authority gap | 0 |
| Other owner | 0 |
| Total | 43 |

Focused scan result:

```text
[rv64-gcc-torture] total=43 passed=7 failed=36
```

## Residual Owners

Later RV64 diagnostic or lowering owners:

- RV64 instruction-fragment lowering: 14 rows.
- RV64 global-data emission: 4 rows.
- RV64 local-memory access: 2 rows.
- RV64 return pointer stack-source lowering: 2 rows.
- RV64 select stack-publication lowering: 2 rows.

Earlier semantic producer or explicit classifier owners:

- Ambiguous non-parallel multi-source stack-destination authority: 7 rows.
- Conversion-adjacent stack-source stack-destination classification: 2 rows.

Runtime owners:

- Runtime mismatch: 3 rows.

Passing repaired rows:

- `src/20000717-3.c`
- `src/20011219-1.c`
- `src/20070212-1.c`
- `src/20100316-1.c`
- `src/20170419-1.c`
- `src/cmpsi-1.c`
- `src/vrp-6.c`

## Lifecycle Recommendation

Close or split the prepared-authority plan. Steps 2-5 repaired the prepared
publication facts that were safe to repair here, and Step 6 found no remaining
unexplained prepared-authority row. The residual failures have explicit owners
outside this source idea: later RV64 lowering policy, earlier semantic
producer/classifier authority, or runtime mismatch investigation.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step6_prepared_authority_reconciliation.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Initial `git status --short` was clean.
- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 6 proof scanned all 43 rows and reported `7` passed,
  `36` failed.
- The `36` failures all reconcile to non-prepared residual owners.
