Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Reclassify Aggregate Stack-Home Rows After Residual Repair

# Current Packet

## Just Finished

Step 11 reran the aggregate stack-home residual probe after the Step 10 sret
extent repair and classified the remaining rows.

The refreshed probe stayed at `total=35 passed=3 failed=32`, but the Step 9
primary sret rows moved past the aggregate stack-home local-memory owner:
`src/20020215-1.c`, `src/950628-1.c`, and `src/pr30185.c` now fail at
`unsupported_call_abi` instead of `unsupported_local_memory_access`.
`src/pr60017.c` also moved from local memory to `unsupported_call_abi`.
`src/pr38969.c` and `src/struct-ret-1.c` remain past stack-home local memory at
later call owners from Step 8.

`src/941110-1.c` did not move. Fresh prepared evidence shows ordinary
local-slot/frame-slot pointer publication with an unknown-layout 8-byte pointer
store, not a byval/sret stack-home authority shape, so it should not extend
idea 633.

Remaining failures bucket as:
- later call ABI or call-instruction owners: `src/20020215-1.c`,
  `src/921117-1.c`, `src/950628-1.c`, `src/pr30185.c`, `src/pr38969.c`,
  `src/pr58984.c`, `src/pr60017.c`, `src/struct-ret-1.c`
- idea 637 move-bundle destination fan-in: `src/20011109-2.c`,
  `src/20021204-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, `src/ptr-arith-1.c`
- F128/16-byte local-memory width policy: `src/20010605-2.c`,
  `src/20040208-1.c`, `src/ieee/inf-1.c`
- idea 639 pointer-loaded-from-global: `src/pr46309.c`
- idea 634 large selected pointer-offset: `src/ipa-sra-2.c`, `src/pr60822.c`
- idea 635 branch stack-load clobber-safety: `src/pr52129.c`
- idea 641 aggregate global-object materialization: `src/complex-7.c`,
  `src/pr49073.c`, `src/pr66556.c`, `src/pr88739.c`
- idea 640 mixed local/global publication: `src/pr57861.c`,
  `src/pr58431.c`, `src/pr68185.c`, `src/pr68321.c`, `src/pr70005.c`
- idea 642 runtime/global residual mismatch: `src/20000722-1.c`
- new residual candidate if split is desired: `src/941110-1.c`, ordinary
  frame-slot pointer/local-slot publication rather than stack-home authority

## Suggested Next

Ask the plan owner to close or exhaust idea 633 as complete for aggregate
byval/sret stack-home local-memory policy, with an optional split for
`src/941110-1.c` as a separate ordinary frame-slot pointer-publication
residual if the supervisor wants durable tracking.

## Watchouts

The raw pass/fail count did not change because the moved rows now expose later
backend owners, not full row passes. Treat count stability as owner movement,
not lack of Step 10 effect.

Do not fold `src/941110-1.c` back into idea 633 without new evidence tying it
to byval/sret stack-home authority; the current dump shows ordinary local-slot
pointer publication.

## Proof

Delegated proof command:
`cmake --build --preset default && ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/633_step11_aggregate_stack_home.log 2>&1`

Result: build succeeded; focused residual probe returned nonzero for remaining
failures with `total=35 passed=3 failed=32`.

Evidence:
- `build/agent_state/633_step11_aggregate_stack_home.allowlist`
- `build/agent_state/633_step11_aggregate_stack_home.log`
- `build/agent_state/633_step11_current_diagnostics.tsv`
- `build/agent_state/633_step11_failure_diagnostics.tsv`
- `build/agent_state/633_step11_movement.tsv`
- `build/agent_state/633_step11_classification.md`
- focused prepared extracts under `build/agent_state/633_step11_*`

Extra check: `git diff --check` passed.
