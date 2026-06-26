Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Iterate Or Split Remaining Families

# Current Packet

## Just Finished

Step 4 - Iterate Or Split Remaining Families re-probed `src/20000403-1.c`
after the udiv/immediate-loader slice and repaired one reusable RV64 object
call-lowering family: prepared direct-call GPR arguments whose source storage
is an integer immediate now use the existing recursive RV64 immediate
materializer instead of rejecting values outside the signed-12-bit `addi`
range. This is a semantic call-argument materialization change in
`fragment_for_prepared_call()`, not a filename- or constant-specific shortcut.

The fresh proof moved `src/20000403-1.c` past the old
`unsupported_instruction_fragment` blocker. The current blocker is now
`unsupported_param_home: RV64 object route requires all parameters in supported
GPR or prepared FPR register homes`, so no second implementation family was
added in this packet.

## Suggested Next

Delegate the next packet to classify and repair the reusable
`unsupported_param_home` family exposed by `src/20000403-1.c`, or choose a
different representative if the supervisor wants to defer parameter-home
support.

## Watchouts

- The delegated proof for `src/20000403-1.c` is still red because of the new
  parameter-home blocker; the immediate call-argument fragment itself compiled
  far enough to expose that next root cause.
- Do not continue expanding this packet into parameter-home support; that is a
  separate reusable family from direct-call immediate argument materialization.
- Do not mix `20000622-1.c` into the udiv packet; its frame-slot call arguments
  are explicitly marked with missing publication facts.
- Keep aggregate/sret local-frame-address handling from `20000917-1.c` and
  broader large-literal comparison pressure from `20001221-1.c` as separate
  packets unless a later proof shows they collapse into the same producer/target
  fact.

## Proof

Ran the delegated RV64 proof exactly. Result: `[rv64-gcc-torture] total=1
passed=0 failed=1`, with `fail	src/20000403-1.c`.

Delegated command:
`cmake --build --preset default --target c4cll && printf '%s\n' 'src/20000403-1.c' > build/agent_state/395_step4_next.allowlist.txt && { ALLOWLIST=build/agent_state/395_step4_next.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log`

One-case proof log copied before supervisor validation:
`build/agent_state/395_step4_20000403_immediate_arg_probe.log`. Current
per-case blocker:
`build/rv64_gcc_c_torture_backend/src_20000403-1.c/case.log` reports
`unsupported_param_home: RV64 object route requires all parameters in supported
GPR or prepared FPR register homes`.

Supervisor validation:
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed after
  the slice: 326 passed, 0 failed.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  passed against the existing backend-subset `test_before.log`.
