Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reconcile The 151-Row Lane

# Current Packet

## Just Finished

Completed Step 6, `Reconcile The 151-Row Lane`.

Generated
`build/agent_state/551_step6_reconciliation/coherent_rv64_mir_materialization.allowlist`
from the Step 4 classification TSV, reran the 151-row RV64 gcc torture backend
subset, and recorded row-level reconciliation in
`docs/rv64_gcc_torture_post_contract/move_bundle_materialization_reconciliation.md`.

Fresh counts:

- 151 coherent-lane rows scanned.
- 45 rows now pass.
- 20 rows still contain
  `fragment_status=generic_move_bundle_materialization_failed`.
- 48 rows advanced to later explicit unsupported backend diagnostics.
- 10 rows advanced to runtime mismatch or abort comparison.
- 28 rows appear to need reroute to prepared authority, prepared classifier,
  or evidence-gap ownership.

No implementation files, `plan.md`, source ideas, expectation files,
unsupported markers, allowlists outside `build/agent_state`, or runtime
comparison code were changed.

## Suggested Next

Do not ask plan-owner to close or deactivate yet. Delegate one more
implementation packet for the 20 rows still failing at generic move-bundle
materialization, starting with the 17 remaining
`consumer_register_to_stack/register_to_stack_slot` rows and keeping the three
`rematerializable_immediate_to_stack_slot` rows separate unless inspection
shows they share the same semantic missing rule.

## Watchouts

- The 58 later-residual rows are not current move-bundle materialization
  blockers; they split across unsupported instruction, terminator, local
  memory, global data, and runtime mismatch categories.
- The 28 reroute rows should not be repaired by RV64 home inference. Current
  evidence points at prepared move-bundle authority/classification,
  select-publication source authority, or a move-bundle evidence gap.
- The row-level reconciliation TSV is derived under
  `build/agent_state/551_step6_reconciliation/reconciliation_rows.tsv`.

## Proof

- Delegated Step 6 docs/status proof was run; full output is preserved in
  `test_after.log`.
- The proof built `c4cll` through
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
- The allowlist was
  `build/agent_state/551_step6_reconciliation/coherent_rv64_mir_materialization.allowlist`.
- The subset reported `total=151 passed=45 failed=106`.
- `git diff --check -- todo.md docs/rv64_gcc_torture_post_contract/move_bundle_materialization_reconciliation.md`
  passed.
