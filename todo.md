Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Fail-Closed Behavior

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: added focused fail-closed proof for the
select-carrier alias source freshness contract.

Test coverage added in `check_select_carrier_alias_authority_contract()`:

- Accepted carrier-alias authority now asserts selected
  `SelectCarrierAliasSource` freshness with exact source value id/name,
  `SelectCarrierAlias` source kind, `SelectCarrierAliasAuthority` proof,
  `SelectCarrierAlias` rank, and source producer block/inst reference.
- `prepared_select_carrier_alias_source_freshness_available(...)` is asserted
  to accept the good authority record and publication.
- Alias-only authority is proven insufficient by clearing selected freshness
  while leaving the alias authority otherwise available.
- Fail-closed copies reject ambiguous freshness, stale/wrong reference, wrong
  value, wrong use, wrong source kind, wrong proof kind, and wrong rank.
- Existing rejected source-use closure and prepared dump assertions remain in
  place for unavailable carrier-alias evidence.

Files changed:

- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`

## Suggested Next

Step 5: Handoff And Broader Validation.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, diagnostics-only changes, or target-local shape
  checks.
- Prepared dumps still expose carrier-alias authority status, candidates,
  aliases, and source-use closure, but do not print the selected
  `SelectCarrierAliasSource` freshness fields. Adding dump output was left out
  of this test-only packet to avoid broadening production output.
- Step 5 should record that direct contract tests, not dump assertions, prove
  missing, ambiguous, stale/wrong-reference, wrong-value, wrong-use,
  wrong-source, wrong-proof, wrong-rank, and alias-only fail-closed behavior.
- Keep destination fan-in, predecessor-edge suppression, pointer/address
  follow-ups, target migration, and Prepared MIR view design separate.

## Proof

Ran focused precheck:
`cmake --build --preset default --target backend_prepare_stack_layout_test && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'`;
passed.

Ran
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`;
passed with 346/346 backend tests passing. Proof log: `test_after.log`.

Ran `git diff --check`; passed.
