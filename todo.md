Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reconcile Call Metadata Representatives

# Current Packet

## Just Finished

Step 5 - Reconcile Call Metadata Representatives reran the direct-call and
call-return representative rows together and recorded their current ownership.

Representative outcomes:

- `src/20000412-2.c` no longer reports a direct-call semantic call metadata
  admission failure. It now reaches downstream RV64 object-route unsupported
  ownership: `unsupported_instruction_fragment: BIR instruction requires
  unsupported RV64 object lowering`.
- `src/20050121-1.c` no longer reports a call-return semantic call metadata
  admission failure. It now reaches downstream RV64 object-route unsupported
  ownership: `unsupported_instruction_fragment: BIR instruction requires
  unsupported RV64 object lowering`.

The Step 5 proof log contains no `semantic call family`, `semantic lir_to_bir`,
direct-call admission, or call-return admission diagnostic for either
representative row.

## Suggested Next

Ask plan-owner to close the source idea if it agrees the direct-call and
call-return representative semantic producer-admission objectives are complete.
If plan-owner wants to preserve downstream RV64 object-route unsupported work,
split that into a distinct source idea instead of extending this call metadata
runbook.

## Watchouts

Both rows still fail the progress script, but both failures are downstream RV64
object-route unsupported ownership, not producer-side call metadata admission.
Do not claim runtime/object lowering progress from this packet, and do not fold
RV64 object lowering, expectation changes, unsupported-marker changes,
runtime-comparison changes, or named-case shortcuts into this source idea.

## Proof

Proof logs:

- `build/agent_state/558_step5_call_metadata_reconcile.allowlist`
- `build/agent_state/558_step5_call_metadata_reconcile.log`
- `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20050121-1.c/case.log`

Command:

- `printf '%s\n' src/20000412-2.c src/20050121-1.c >
  build/agent_state/558_step5_call_metadata_reconcile.allowlist &&
  ALLOWLIST=build/agent_state/558_step5_call_metadata_reconcile.allowlist
  VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh >
  build/agent_state/558_step5_call_metadata_reconcile.log 2>&1` exited 1
  because both representatives moved to downstream RV64 object-route
  unsupported ownership.

Residual classification:

- Direct-call semantic admission: repaired for the representative row.
- Call-return semantic admission: repaired for the representative row.
- Current owner for both rows: downstream RV64 object lowering.
