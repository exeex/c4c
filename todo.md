Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Completed Step 5: broader validation and closure-readiness review for the
`names` semantic resolver candidate.

Completed proof surface:

- Positive prepared function-name, block-label, and value-name rows.
- Absent prepared names, immediate values, invalid `kInvalidBlockLabel`,
  raw-id spelling drift, corrupted prepared-table round-trip mismatches, direct
  `names.*.find(...)` compatibility, and non-interning table-size preservation.
- Empty function, block-label, and value-name spellings.
- Out-of-range raw BIR block-label ids.
- Raw-only block-label ids that have no prepared id.
- Explicit prepared/BIR block-label spelling drift.
- Repeated missing-name fail-closed queries that must not intern names.
- Broader backend validation passed after the proof rows; the selected semantic
  resolver candidate is ready for plan-owner closure review.

## Suggested Next

Request plan-owner closure review for this selected semantic resolver
candidate. Remaining idea 260 candidates should stay open for separate
one-candidate runbooks.

## Watchouts

- Keep this runbook limited to the selected `names` semantic resolver API
  candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, public aggregate compatibility, existing
  construction-time interning behavior, and current absent/null resolver
  behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  same-block lookup, value-home lookup, control-flow, store-source
  publication, or backend lowering behavior to claim progress.
- Do not use `prepared_named_value_id(...)` as proof of resolver safety; it
  intentionally mutates prepared value names during construction.
- Treat raw BIR ids and raw spelling fallback as compatibility inputs only.
  They become semantic resolver agreement only after the new helper validates
  spelling, id, and prepared-table round trip.
- BIR function and value rows currently expose spellings directly; raw-id
  validation is concrete for block labels through `bir::NameTables`. Do not
  invent new BIR ids for function or value names in this runbook.
- The agreement helper should fail closed, not downgrade tests or rewrite
  backend output expectations to claim progress.
- The Step 3 implementation left `resolve_prepared_*` wrappers unchanged.
  Step 4 should treat those wrappers as compatibility adapters unless the
  supervisor explicitly delegates behavior-preserving wiring.
- Step 4 found no helper behavior bug; remaining work is validation and
  lifecycle readiness, not another code packet.
- Residual out-of-scope rows remain same-block lookup, value-home lookup,
  control-flow dominance and branch-target packets, store-source-publication
  packets, route-debug, printer/debug, target-output, and broad
  `PreparedBirModule` retirement.

## Proof

Ran
`(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`;
passed. `test_after.log` shows `100% tests passed, 0 tests failed out of 1`.

Broader Step 5 proof:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^backend_' -j --output-on-failure) > test_before.log 2>&1
```

Result: passed before and after this closure-readiness update.
`test_before.log` and `test_after.log` both show 180/180 backend tests passed.
The regression guard comparison reported no new failures.
