Status: Active
Source Idea Path: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconcile Runtime Intrinsic Memory Representatives

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: reconciled the runtime intrinsic memory
representatives with a fresh RV64 allowlist proof for `src/20000703-1.c` and
`src/20041218-1.c`.

Both rows have moved off runtime intrinsic memory producer admission:
`src/20000703-1.c` no longer fails in `memcpy runtime family` or `memset
runtime family`, and `src/20041218-1.c` no longer fails in `memset runtime
family`.

Both rows now fail only in downstream RV64 object-route ownership with
`unsupported_global_data: RV64 object route requires supported prepared global
memory facts`. No implementation files, expectations, unsupported markers,
runtime comparison files, lifecycle source files, or review artifacts were
changed.

## Suggested Next

Ask the plan owner to close the active runtime/intrinsic memory producer
admission idea. If downstream RV64 global-data object-route support should be
tracked, split that into a separate source idea instead of extending this
producer-admission runbook.

## Watchouts

- The proof command exits `1` because both representative rows still fail, but
  the remaining failure family is downstream `unsupported_global_data`, not
  runtime intrinsic producer admission.
- Do not fold RV64 prepared-global-data/object-route support into this source
  idea unless the plan owner deliberately creates a new lifecycle scope for it.

## Proof

- `printf '%s\n' src/20000703-1.c src/20041218-1.c > build/agent_state/559_step4_intrinsic_memory_reconcile.allowlist && ALLOWLIST=build/agent_state/559_step4_intrinsic_memory_reconcile.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/559_step4_intrinsic_memory_reconcile.log 2>&1`
  exited `1`, acceptable for this reconciliation packet because both rows now
  fail only in downstream `unsupported_global_data` ownership. Proof log:
  `build/agent_state/559_step4_intrinsic_memory_reconcile.log`.
