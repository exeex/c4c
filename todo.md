Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Select-Publication Register Move Coverage

# Current Packet

## Just Finished

Completed Step 5, `Add Select-Publication Register Move Coverage`.

Verified the remaining coherent
`phi_join_register_to_register/select_publication_immediate_to_register`
representative, `src/pr37924.c`, with no additional implementation edits.

The case no longer ends at generic move-bundle materialization failure or
select-publication immediate rejection. It now advances to a later
`unsupported_instruction_fragment` residual outside this Step 5 packet.

No implementation files, prepared-authority rows, expectation files,
unsupported markers, allowlists outside `build/agent_state`, or runtime
comparison code were changed.

## Suggested Next

Proceed to Step 6 reconciliation for the 151-row coherent lane. Recount fixed
rows, rows that now stop at later residuals, rows still failing for
move-bundle materialization, and any rows that should be rerouted to a
follow-up owner.

## Watchouts

- `src/pr37924.c` still fails the RV64 gcc torture backend scan, but the
  observed residual is `unsupported_instruction_fragment`, not the Step 5
  select-publication path.
- Step 6 should reconcile later residuals separately from move-bundle
  materialization coverage; do not count later unsupported instruction or
  terminator failures as Step 5 blockers.
- Prepared-authority-gap rows should stay with their follow-up idea unless
  reconciliation proves a row was misclassified.

## Proof

- Delegated Step 5 proof command was run exactly; full output is preserved in
  `test_after.log`.
- `cmake --build --preset default` completed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345 backend tests.
- The one-case RV64 gcc torture allowlist was `src/pr37924.c`.
- The final assertion reports `generic_move_bundle_failure_count=0` and
  `select_publication_failure_count=0`.
- The per-case status was `advanced_or_pass`; the current later residual is
  `unsupported_instruction_fragment`.
