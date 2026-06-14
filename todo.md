Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Completed Step 4: audited and extended fail-closed proof rows for the `names`
semantic resolver candidate.

Step 3 already covered:

- Positive prepared function-name, block-label, and value-name rows.
- Absent prepared names, immediate values, invalid `kInvalidBlockLabel`,
  raw-id spelling drift, corrupted prepared-table round-trip mismatches, direct
  `names.*.find(...)` compatibility, and non-interning table-size preservation.

Added focused Step 4 rows for:

- Empty function, block-label, and value-name spellings.
- Out-of-range raw BIR block-label ids.
- Raw-only block-label ids that have no prepared id.
- Explicit prepared/BIR block-label spelling drift.
- Repeated missing-name fail-closed queries that must not intern names.

## Suggested Next

Execute Step 5: broader validation and closure readiness for the semantic
resolver helper candidate.

Suggested Step 5 packet:

- Owned files: `todo.md`.
- Run the focused proof again and any supervisor-selected broader prepared or
  backend subset needed for a helper shared by three resolver families.
- Record proof commands, pass/fail status, and residual out-of-scope rows.
- If complete, request plan-owner closure review rather than expanding into
  another idea 260 candidate.
- Preserve route-debug, target-output, same-block lookup, value-home lookup,
  control-flow, store-source, printer/debug, and backend lowering behavior.

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

## Proof

Ran
`(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`;
passed. `test_after.log` shows `100% tests passed, 0 tests failed out of 1`.
