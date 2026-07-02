Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Triage The 17 Residual Register-To-Stack Rows

# Current Packet

## Just Finished

Plan-owner reviewed the Step 6 reconciliation and extended the active runbook
because the source idea is not complete.

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

No implementation files, source ideas, expectation files, unsupported markers,
allowlists outside `build/agent_state`, or runtime comparison code were
changed. This lifecycle repair updated `plan.md`, `todo.md`, and local
plan-review state only.

## Suggested Next

Delegate Step 7: triage and implement the next general RV64/MIR materialization
rule for the 17 residual
`consumer_register_to_stack/register_to_stack_slot` rows that still report
`fragment_status=generic_move_bundle_materialization_failed`.

The Step 7 residual rows are:

- `src/20000717-3.c`
- `src/20020226-1.c`
- `src/20020508-1.c`
- `src/20020508-2.c`
- `src/20020508-3.c`
- `src/20020510-1.c`
- `src/20100316-1.c`
- `src/920908-2.c`
- `src/bf-pack-1.c`
- `src/loop-2d.c`
- `src/pr25125.c`
- `src/pr40386.c`
- `src/pr48197.c`
- `src/pr81281.c`
- `src/pr89195.c`
- `src/strcmp-1.c`
- `src/strncmp-1.c`

Keep the three residual
`rematerializable_immediate_to_stack_slot` rows (`src/920721-1.c`,
`src/pr82192.c`, and `src/usmul.c`) for Step 8 unless Step 7 inspection proves
they share the same semantic missing rule and the delegated proof covers both
families.

## Watchouts

- The 58 later-residual rows are not current move-bundle materialization
  blockers; they split across unsupported instruction, terminator, local
  memory, global data, and runtime mismatch categories.
- The 28 reroute rows should not be repaired by RV64 home inference. Current
  evidence points at prepared move-bundle authority/classification,
  select-publication source authority, or a move-bundle evidence gap.
- The row-level reconciliation TSV is derived under
  `build/agent_state/551_step6_reconciliation/reconciliation_rows.tsv`.
- The active runbook now has Step 7 for the 17 register-to-stack residual rows,
  Step 8 for the three immediate-to-stack residual rows, and Step 9 for final
  residual reconciliation.

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
- Plan-owner lifecycle repair should be validated with
  `git diff --check -- plan.md todo.md .plan_review_state.json`.
